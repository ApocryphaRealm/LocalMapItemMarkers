#include "PCH.h"

#include "DevBenchTool.h"

#include "Markers.h"
#include "DevBench/DevBenchAPI.h"
#include "Settings.h"
#include "utils/Logger.h"
#include "utils/Strings.h"

#include <cstdlib>
#include <format>
#include <string>
#include <string_view>

namespace DevBenchTool
{
	namespace
	{
		std::string EscapeJson(std::string_view a_in)
		{
			std::string out;
			out.reserve(a_in.size() + 8);
			for (const char c : a_in)
			{
				switch (c)
				{
				case '\\': out += "\\\\"; break;
				case '"': out += "\\\""; break;
				case '\n': out += "\\n"; break;
				default: out += c; break;
				}
			}
			return out;
		}

		void ControlTool(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			const std::string_view args = a_argsJson ? a_argsJson : "";
			if (args.find("\"reload\"") != std::string_view::npos)
			{
				const bool ok = settings::Reload();
				a_write(a_sink, std::format(R"({{"ok":{},"op":"reload"}})", ok ? "true" : "false").c_str());
				return;
			}
			if (const auto at = args.find("undelete:"); at != std::string_view::npos)
			{
				const std::string hex(args.substr(at + 9, 8));
				a_write(a_sink, itemmarkers::TestClearDeleted(static_cast<std::uint32_t>(std::strtoul(hex.c_str(), nullptr, 16))).c_str());
				return;
			}
			if (args.find("\"questprobe\"") != std::string_view::npos)
			{
				a_write(a_sink, itemmarkers::ProbeQuestItems().c_str());
				return;
			}
			if (args.find("\"strings\"") != std::string_view::npos)
			{
				a_write(a_sink, std::format(R"({{"ok":true,"op":"strings","strings":{}}})", strings::StatusJson()).c_str());
				return;
			}
			// op=<category>:<0|1> - flip a category live, so an in-game test does not need the
			// settings page or a restart. The next marker build picks it up.
			for (const auto& [key, target] : { std::pair{ "weapons:", &settings::markers::showWeapons },
											   std::pair{ "armour:", &settings::markers::showArmour },
											   std::pair{ "containers:", &settings::markers::showContainers },
											   std::pair{ "questitems:", &settings::markers::showQuestItems } })
			{
				const auto at = args.find(key);
				if (at != std::string_view::npos)
				{
					const auto tail = args.substr(at + std::string_view(key).size(), 8);
					*target = (tail.find('1') != std::string_view::npos) ||
							  (tail.find("true") != std::string_view::npos);
					a_write(a_sink, std::format(R"({{"ok":true,"op":"{}","value":{}}})", key,
											   *target ? "true" : "false").c_str());
					return;
				}
			}

			const auto s = itemmarkers::GetStats();
			const std::string json = std::format(
				"{{\"ok\":true,"
				"\"registeredWithLMU\":{},"
				"\"settings\":{{\"showWeapons\":{},\"showArmour\":{},\"showContainers\":{},"
				"\"showQuestItems\":{},\"radiusFeet\":{},\"maxMarkers\":{},\"logLevel\":{},\"iniPath\":\"{}\"}},"
				"\"lastBuild\":{{\"contributed\":{},\"scanned\":{},\"cappedAway\":{},\"micros\":{},"
				"\"weapons\":{},\"armour\":{},\"containers\":{},\"questItems\":{}}},"
				"\"builds\":{}}}",
				itemmarkers::IsRegistered() ? "true" : "false",
				settings::markers::showWeapons ? "true" : "false",
				settings::markers::showArmour ? "true" : "false",
				settings::markers::showContainers ? "true" : "false",
				settings::markers::showQuestItems ? "true" : "false",
				settings::markers::radiusFeet, settings::markers::maxMarkers,
				settings::debug::logLevel, EscapeJson(settings::GetIniPath()),
				s.contributed, s.scanned, s.cappedAway, s.lastScanMicros,
				s.weapons, s.armour, s.containers, s.questItems, s.builds);
			a_write(a_sink, json.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;
		if (registered) { return; }

		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();
		if (!devBench)
		{
			if (a_lastAttempt)
			{
				logger::info("DevBench not detected; skipping the \"lmim.status\" tool");
			}
			else
			{
				logger::debug("DevBench not detected yet; will retry at the next message");
			}
			return;
		}

		constexpr const char* descriptor =
			"{"
			"\"description\":\"Local Map Item Markers live state: whether it registered with Local "
			"Map Upgrade, the four category settings, and what the LAST marker build found - "
			"contributed/scanned/cappedAway, the scan time in microseconds, and the per-category "
			"counts. op=reload re-reads the INI; op=strings reports the active language; "
			"op=weapons:0|1, armour:, containers:, questitems: flip a category live for testing; op=questprobe lists the references in range that carry quest-alias data (and every runtime-created one); op=undelete:<ref> clears the Deleted flag on one reference, a test fixture for a quest's reserved alias item.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{\"op\":{\"type\":\"string\"}}},"
			"\"readOnly\":false"
			"}";

		if (devBench->RegisterTool("lmim.status", descriptor, &ControlTool, nullptr))
		{
			logger::info("Registered \"lmim.status\" with DevBench (build {})", devBench->GetBuildNumber());
			registered = true;
		}
	}
}
