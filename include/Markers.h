#pragma once

// Local Map Item Markers - the whole feature. Registers as Local Map Upgrade's extra-marker
// provider and, each time the local map builds its markers, contributes one marker per nearby
// weapon, armour piece, container or quest item.
//
// This plugin draws nothing and hooks nothing. Local Map Upgrade owns the marker array and the
// Scaleform side; all this does is answer "what is near the player, and which icon should it use".

#include <cstdint>
#include <string>

namespace itemmarkers
{
	// The icon indices this add-on contributes. They continue Local Map Upgrade's own enum
	// (0-5: enemy, hostile, guard, dead, teammate, neutral) and MUST match the order of the names
	// appended to LocalMap.ExtraMapMarker.IconTypes in the SWFs this add-on ships. Getting these
	// out of step is silent: the marker draws, with the wrong icon.
	enum class IconType : std::uint32_t
	{
		kWeapon = 6,
		kArmour = 7,
		kContainer = 8,
		kQuestItem = 9
	};

	// Registers the SKSE listener that waits for Local Map Upgrade's provider dispatch. Safe to
	// call before LMU has loaded - the registration is what waits, not us.
	void Install();

	// True once Local Map Upgrade has accepted this plugin as its provider. Until then this mod
	// does nothing at all, which is exactly what should happen when LMU is missing or too old.
	bool IsRegistered();

	// What the last marker build found, for the DevBench status tool. Read on devbench's listener
	// thread, written on the render thread, so every field is atomic and the snapshot is
    // deliberately cheap rather than consistent - it is a diagnostic, not a transaction.
	struct Stats
	{
		std::uint32_t weapons = 0;
		std::uint32_t armour = 0;
		std::uint32_t containers = 0;
		std::uint32_t questItems = 0;
		std::uint32_t contributed = 0;   // what was actually handed to LMU, after the cap
		std::uint32_t scanned = 0;       // references looked at
		std::uint32_t cappedAway = 0;    // matches dropped because uMaxMarkers was reached
		std::uint64_t lastScanMicros = 0;
		std::uint64_t builds = 0;
	};

	Stats GetStats();

	// Test aid for the DevBench tool (op=questprobe): every reference in range that carries quest-alias data,
	// with what the engine says about it - so a quest item can be found and judged without guessing which
	// cell holds one. Reads only; same radius setting as the marker build.
	std::string ProbeQuestItems();

	// Test fixture (op=undelete:<ref>): clears the engine's Deleted flag on one reference. A quest's
	// reserved alias item carries that flag until the quest really fills it, and the marker build rightly
	// skips deleted references; a test that has no way to run the quest to that point clears it by hand to
	// see the same item as the engine would present it once filled. Never called by the plugin itself.
	std::string TestClearDeleted(std::uint32_t a_formID);
}
