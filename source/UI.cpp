#include "PCH.h"

#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Markers.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Strings.h"
#include "utils/Toggle.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace UI
{
	namespace
	{
		std::string statusMessage;
		std::string selectedSlider;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
		constexpr const char* kLogLevelKeys[] = { "LMIM_LogLevel_Trace", "LMIM_LogLevel_Debug", "LMIM_LogLevel_Info",
													"LMIM_LogLevel_Warning", "LMIM_LogLevel_Error", "LMIM_LogLevel_Critical", "LMIM_LogLevel_Off" };
		constexpr int kLogLevelCount = 7;

		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface())
			{
				taskInterface->AddTask(std::move(a_task));
			}
		}

		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igCombo_Str_arr",
				"igSliderFloat",
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				"igGetCursorScreenPos",
				"igGetWindowDrawList",
				"igGetFrameHeight",
				"igInvisibleButton",
				"igPushID_Str",
				"igPopID",
				"ImDrawList_AddRectFilled",
				"ImDrawList_AddCircleFilled"
			};

			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("The menu framework does not export \"{}\"", name);
					return false;
				}
			}
			return true;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("%s", strings::TR("LMIM_HelpMark", "(?)"));
			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		bool NudgeableSlider(const char* a_label, float* a_value, float a_min, float a_max,
							 const char* a_format, float a_step)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max, a_format);
			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive()) { selectedSlider = a_label; }
			if (selectedSlider == a_label)
			{
				float nudge = 0.0F;
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow)) { nudge -= a_step; }
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow)) { nudge += a_step; }
				if (nudge != 0.0F)
				{
					*a_value = std::clamp(*a_value + nudge, a_min, a_max);
					changed = true;
				}
				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("%s", strings::TR("LMIM_NudgeArrows", "<-->"));
			}
			return changed;
		}

		void RenderGeneralSection()
		{
			using namespace settings;

			// Nothing this page changes needs applying: the next time the map draws its markers,
			// the provider reads these values afresh. So there is no Apply button here.
			ImGuiMCP::SeparatorText(strings::TR("LMIM_What", "What to mark"));

			ImGuiMCP::Toggle(strings::TR("LMIM_Weapons", "Weapons"), &markers::showWeapons);
			ImGuiMCP::Toggle(strings::TR("LMIM_Armour", "Armour"), &markers::showArmour);
			ImGuiMCP::Toggle(strings::TR("LMIM_Containers", "Containers"), &markers::showContainers);
			ImGuiMCP::Toggle(strings::TR("LMIM_QuestItems", "Quest items"), &markers::showQuestItems);
			HelpMarker(strings::TR("LMIM_HelpQuestItems", "A quest item is marked as a quest item even when it is also a weapon or a piece of armour, so it stands out from ordinary loot."));

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText(strings::TR("LMIM_Limits", "Limits"));

			float radius = static_cast<float>(markers::radiusFeet);
			if (NudgeableSlider(strings::TR("LMIM_Radius", "Radius (feet)"), &radius, 0.0F, 500.0F, "%.0f", 10.0F))
			{
				markers::radiusFeet = static_cast<std::uint32_t>(radius);
			}
			HelpMarker(strings::TR("LMIM_HelpRadius", "How far from you something may be and still be marked. 0 means no limit, which is not recommended in a crowded cell - everything in the loaded area is examined every time the map draws."));

			float cap = static_cast<float>(markers::maxMarkers);
			if (NudgeableSlider(strings::TR("LMIM_MaxMarkers", "Most markers at once"), &cap, 5.0F, 250.0F, "%.0f", 5.0F))
			{
				markers::maxMarkers = static_cast<std::uint32_t>(cap);
			}
			HelpMarker(strings::TR("LMIM_HelpMaxMarkers", "A ceiling on how many markers are added in one go. Every marker is drawn separately, so a very high number costs frame rate and turns the map into clutter."));

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText(strings::TR("LMIM_LastBuild", "Last time the map drew"));

			if (!::itemmarkers::IsRegistered())
			{
				ImGuiMCP::TextWrapped("%s", strings::TR("LMIM_NotRegistered", "Local Map Upgrade has not accepted this add-on, so no markers are being added. Check that Local Map Upgrade is installed and up to date."));
			}
			else
			{
				const auto st = ::itemmarkers::GetStats();
				ImGuiMCP::Text(strings::TR("LMIM_Counts", "%u marked of %u examined - %u weapons, %u armour, %u containers, %u quest items"),
							   st.contributed, st.scanned, st.weapons, st.armour, st.containers, st.questItems);
				if (st.cappedAway > 0)
				{
					ImGuiMCP::TextWrapped(strings::TR("LMIM_Capped", "%u more matched but were left out by the marker limit above."), st.cappedAway);
				}
				ImGuiMCP::TextDisabled(strings::TR("LMIM_ScanTime", "Last search took %llu microseconds."), st.lastScanMicros);
			}
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText(strings::TR("LMIM_Debug", "Debug"));

			int level = static_cast<int>(debug::logLevel);
			level = std::clamp(level, 0, kLogLevelCount - 1);
			// Rebuilt from TR'd entries every frame (plan 2.2); labelStore owns the translated
			// bytes for this call so the const char* pointers handed to Combo stay valid.
			std::vector<std::string> logLevelLabelStore;
			logLevelLabelStore.reserve(kLogLevelCount);
			for (int i = 0; i < kLogLevelCount; ++i)
			{
				logLevelLabelStore.push_back(strings::TR(kLogLevelKeys[i], kLogLevelNames[i]));
			}
			std::vector<const char*> logLevelLabels;
			logLevelLabels.reserve(logLevelLabelStore.size());
			for (const auto& s : logLevelLabelStore) { logLevelLabels.push_back(s.c_str()); }
			if (ImGuiMCP::Combo(strings::TR("LMIM_LogLevel", "Log level"), &level, logLevelLabels.data(), kLogLevelCount))
			{
				debug::logLevel = static_cast<std::uint32_t>(level);
				ApplyLogLevel();
			}
			HelpMarker(strings::TR("LMIM_HelpLogLevel", "Applies immediately. The log is at Documents\\My Games\\Skyrim Special Edition\\SKSE\\LocalMapItemMarkers.log."));
		}

		void RenderButtons()
		{
			ImGuiMCP::SeparatorText("");

			if (ImGuiMCP::Button(strings::TR("LMIM_SaveBtn", "Save")))
			{
				statusMessage = strings::TR("LMIM_StatusSaving", "Saving...");
				OnMainThread([]() {
					statusMessage = settings::Save() ? strings::TR("LMIM_StatusSaved", "Settings saved.")
													   : strings::TR("LMIM_StatusSaveFail", "Could not write the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("LMIM_HelpSave", "Writes every setting on this page to the plugin's INI so it survives a restart."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("LMIM_ReloadBtn", "Reload from INI")))
			{
				statusMessage = strings::TR("LMIM_StatusReloading", "Reloading...");
				OnMainThread([]() {
					statusMessage = settings::Reload() ? strings::TR("LMIM_StatusReloaded", "Settings reloaded from the INI.")
													   : strings::TR("LMIM_StatusReloadFail", "Could not read the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("LMIM_HelpReload", "Throws away any change made here since the last save and re-reads the INI from disk."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("LMIM_RestoreBtn", "Restore defaults")))
			{
				OnMainThread([]() {
					settings::RestoreDefaults();
					logger::debug("Restored default settings");
				});
				statusMessage = strings::TR("LMIM_StatusRestored", "Defaults restored. Press Save to keep them.");
			}
			HelpMarker(strings::TR("LMIM_HelpRestore", "Puts every setting back to its fresh-install value. Nothing is written until you press Save."));

			if (!statusMessage.empty())
			{
				ImGuiMCP::TextWrapped("%s", statusMessage.c_str());
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::Text("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::info("No menu framework is installed; settings will be read from the INI only");
			return;
		}
		if (!HasRequiredExports())
		{
			logger::warn("The installed menu framework is older than this plugin's settings "
						 "menu needs. Update it (Apocrypha Menu Framework, or SKSE Menu "
						 "Framework version 3 or newer).");
			return;
		}

		SKSEMenuFramework::SetSection("Local Map Item Markers");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);
		logger::info("Registered the settings page with the menu framework");
	}

	void __stdcall SettingsPanel::Render()
	{
		strings::Tick();

		ImGuiMCP::TextWrapped("%s", strings::TR("LMIM_Intro", "Changes apply as soon as you make them. Press Save to keep them for the next time you play."));
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		RenderGeneralSection();
		ImGuiMCP::Spacing();

		RenderDebugSection();
		ImGuiMCP::Spacing();

		ImGuiMCP::PopItemWidth();

		RenderButtons();
	}
}
