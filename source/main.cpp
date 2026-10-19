// Local Map Item Markers - own code, GPL-3.0-or-later (MIT up to 1.0.0). An optional add-on to Local Map Upgrade:
// marks nearby weapons, armour, containers and quest items on the local map - and therefore on
// Dragon's Eye Minimap too, since that renders the same map re-framed.
//
// It hooks nothing and draws nothing. Local Map Upgrade owns the marker array and the Scaleform
// side; this plugin registers as its marker provider and answers "what is near the player, and
// which icon should it use". With Local Map Upgrade absent, or too old to offer the seam, this
// plugin loads and does nothing at all - which is the whole point of shipping it separately.
//
// No co-save serialization: this mod holds no state belonging to a save. Everything it knows is
// recomputed from the world every time the map draws.
#include "PCH.h"

#include "DevBenchTool.h"
#include "Markers.h"
#include "Settings.h"
#include "UI.h"

#include "utils/AddressLibraryGuard.h"
#include "utils/Logger.h"
#include "utils/Strings.h"

namespace
{
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case SKSE::MessagingInterface::kPostLoad:
			DevBenchTool::Init(false);

			// Registered at kPostLoad so the listener exists before Local Map Upgrade dispatches
			// at kDataLoaded, whichever order SKSE happens to load the two plugins in.
			itemmarkers::Install();
			break;

		case SKSE::MessagingInterface::kDataLoaded:
			strings::Configure("LocalMapItemMarkers");
			UI::Register();
			DevBenchTool::Init(true);

			break;

		// The "did Local Map Upgrade answer?" check CANNOT live at kDataLoaded. SKSE delivers that
		// message to every plugin in load order, and Local Map Upgrade dispatches its seam from
		// inside ITS kDataLoaded - so when this plugin loads first, the check runs about a tenth of
		// a second before the registration it is testing for. The first run of this add-on logged
		// exactly that: a warning saying the seam was missing, followed immediately by "registered
		// with Local Map Upgrade". Same trap as the retry rule - a fixed moment during startup is a
		// guess about another plugin's ordering.
		//
		// So it is checked when the player is actually in a world, which is the first moment the
		// answer matters, and said once.
		case SKSE::MessagingInterface::kPostLoadGame:
		case SKSE::MessagingInterface::kNewGame:
		{
			static bool reported = false;

			if (!reported && !itemmarkers::IsRegistered())
			{
				reported = true;
				logger::warn("Local Map Upgrade has not offered its marker provider seam. Either it "
							 "is not installed, or it predates the version that added it. No item "
							 "markers will appear.");
			}
			break;
		}

		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	// Address Library pre-check (the guard every mod of ours carries), BEFORE SKSE::Init, which opens the
	// Address Library itself: a missing file gets a message naming it and the plugin loads inert.
	if (!AddressLibraryGuard::Guard("Local Map Item Markers"))
	{
		return true;
	}

	SKSE::Init(a_skse);
	SKSE::log::init("LocalMapItemMarkers");

	settings::Init("LocalMapItemMarkers.ini");
	settings::ApplyLogLevel();

	logger::info("Local Map Item Markers {} loading",
				 SKSE::PluginDeclaration::GetSingleton()->GetVersion().string("."));

	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

	return true;
}
