#pragma once

// Local Map Item Markers - settings. Plain-file INI (never the Win32 profile API, so
// PrivateProfileRedirector can neither serve stale values nor overwrite the file).

#include <cstdint>
#include <string>

namespace settings
{
	namespace debug
	{
		inline std::uint32_t logLevel = 2;  // uLogLevel:Debug - 2 = info (the shipped level)
	}

	namespace markers
	{
		// One switch per category, so a player who only wants containers is not paying for the
		// other three. Each is checked while classifying, not after, so a disabled category costs
		// nothing beyond the test.
		inline bool showWeapons = true;     // bShowWeapons:Markers
		inline bool showArmour = true;      // bShowArmour:Markers
		inline bool showContainers = true;  // bShowContainers:Markers
		inline bool showQuestItems = true;  // bShowQuestItems:Markers

		// How far from the player a thing may be and still be marked, in feet, matching how
		// Local Map Upgrade states its own actor radii. 0 means unlimited, which is NOT
		// recommended in a dense cell.
		inline std::uint32_t radiusFeet = 150;

		// A hard ceiling on markers contributed per build. A cluttered map is useless and every
		// marker is a movie clip attached in Scaleform, so this is a real cost, not tidiness.
		inline std::uint32_t maxMarkers = 60;
	}

	void Init(const std::string& a_iniFileName);
	bool Reload();
	bool Save();
	void RestoreDefaults();
	void ApplyLogLevel();
	const std::string& GetIniPath();
}
