#include "PCH.h"

#include "Markers.h"

#include "LMU/API.h"
#include "Settings.h"

#include "utils/Logger.h"

#include <atomic>
#include <chrono>

namespace itemmarkers
{
	namespace
	{
		std::atomic<bool> g_registered{ false };

		std::atomic<std::uint32_t> g_weapons{ 0 };
		std::atomic<std::uint32_t> g_armour{ 0 };
		std::atomic<std::uint32_t> g_containers{ 0 };
		std::atomic<std::uint32_t> g_questItems{ 0 };
		std::atomic<std::uint32_t> g_contributed{ 0 };
		std::atomic<std::uint32_t> g_scanned{ 0 };
		std::atomic<std::uint32_t> g_cappedAway{ 0 };
		std::atomic<std::uint64_t> g_lastScanMicros{ 0 };
		std::atomic<std::uint64_t> g_builds{ 0 };

		// Local Map Upgrade states its own radii in feet; match it rather than inventing a unit.
		constexpr float kFeetToUnits = 21.3333333F;

		// Decides which icon a reference deserves, or nothing at all.
		//
		// Quest items win over their own category on purpose: a quest sword should read as "the
		// thing the quest wants", not as one more weapon among twenty. The engine is asked
		// directly - HasQuestObjectAlias is the game's own answer, not a heuristic of ours.
		bool Classify(RE::TESObjectREFR* a_ref, IconType& a_out)
		{
			RE::TESBoundObject* base = a_ref->GetBaseObject();

			if (!base)
			{
				return false;
			}

			if (settings::markers::showQuestItems && a_ref->extraList.HasQuestObjectAlias())
			{
				a_out = IconType::kQuestItem;
				return true;
			}

			switch (base->GetFormType())
			{
			case RE::FormType::Weapon:
				if (settings::markers::showWeapons) { a_out = IconType::kWeapon; return true; }
				return false;

			case RE::FormType::Armor:
				if (settings::markers::showArmour) { a_out = IconType::kArmour; return true; }
				return false;

			case RE::FormType::Container:
				if (settings::markers::showContainers) { a_out = IconType::kContainer; return true; }
				return false;

			default:
				return false;
			}
		}

		// Called by Local Map Upgrade while it builds the local map's markers - on the render
		// thread, with the map open (or with Dragon's Eye Minimap driving it). Everything here is
		// bounded on purpose: a radius, a hard marker cap, and no allocation per reference.
		void Provider(void* a_frame, LMU::API::ExtraMarkerProviderMessage::AddMarkerFn a_add)
		{
			if (!a_frame || !a_add)
			{
				return;
			}

			const auto started = std::chrono::steady_clock::now();

			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			RE::TES* tes = RE::TES::GetSingleton();

			if (!player || !tes)
			{
				// Nothing to anchor a search on. Not an error worth shouting about every build.
				static bool loggedOnce = false;
				if (!loggedOnce)
				{
					loggedOnce = true;
					logger::warn("provider: no player or TES singleton yet - contributing nothing this build");
				}
				return;
			}

			const std::uint32_t cap = settings::markers::maxMarkers;
			const float radius = settings::markers::radiusFeet == 0 ?
									 std::numeric_limits<float>::max() :
									 static_cast<float>(settings::markers::radiusFeet) * kFeetToUnits;

			std::uint32_t weapons = 0, armour = 0, containers = 0, quest = 0;
			std::uint32_t contributed = 0, scanned = 0, capped = 0;

			tes->ForEachReferenceInRange(player, radius, [&](RE::TESObjectREFR* a_ref) {
				++scanned;

				if (!a_ref || a_ref->IsDisabled() || a_ref->IsDeleted() || a_ref->IsMarkedForDeletion())
				{
					return RE::BSContainer::ForEachResult::kContinue;
				}

				// Actors are Local Map Upgrade's own business - it already marks them, and marking
				// a dead bandit again as "a container" would double up on the same spot.
				if (a_ref->GetFormType() == RE::FormType::ActorCharacter)
				{
					return RE::BSContainer::ForEachResult::kContinue;
				}

				IconType icon{};

				if (!Classify(a_ref, icon))
				{
					return RE::BSContainer::ForEachResult::kContinue;
				}

				switch (icon)
				{
				case IconType::kWeapon:    ++weapons; break;
				case IconType::kArmour:    ++armour; break;
				case IconType::kContainer: ++containers; break;
				case IconType::kQuestItem: ++quest; break;
				}

				// The cap is counted, not silently swallowed: a status tool that says "60 markers"
				// without saying "and 400 more matched" would hide the reason the map looks wrong.
				if (contributed >= cap)
				{
					++capped;
					return RE::BSContainer::ForEachResult::kContinue;
				}

				// The handles were the first suspect when nothing drew (2026-09-09) and were
				// measured innocent - valid, real forms, correct icon indices. Nothing is logged
				// per marker here on purpose: this runs on every frame the map is drawn, and the
				// standing rule is that per-frame code logs transitions only.
				a_add(a_frame, a_ref->CreateRefHandle().native_handle(), a_ref->GetDisplayFullName(),
					  static_cast<std::uint32_t>(icon));
				++contributed;

				return RE::BSContainer::ForEachResult::kContinue;
			});

			const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
									std::chrono::steady_clock::now() - started).count();

			g_weapons.store(weapons);
			g_armour.store(armour);
			g_containers.store(containers);
			g_questItems.store(quest);
			g_contributed.store(contributed);
			g_scanned.store(scanned);
			g_cappedAway.store(capped);
			g_lastScanMicros.store(static_cast<std::uint64_t>(micros));
			g_builds.fetch_add(1);

			// This runs every time the map builds its markers, which with a minimap installed is
			// continuous - so it logs only when the answer CHANGES, per the standing rule about
			// per-frame code.
			static std::uint32_t lastLogged = static_cast<std::uint32_t>(-1);
			if (contributed != lastLogged)
			{
				lastLogged = contributed;
				logger::debug("provider: {} marker(s) from {} reference(s) in {} us "
							  "(weapons={} armour={} containers={} quest={}, {} over the cap)",
							  contributed, scanned, micros, weapons, armour, containers, quest, capped);
			}
		}

		void LMUMessageListener(SKSE::MessagingInterface::Message* a_msg)
		{
			if (!a_msg)
			{
				return;
			}

			// Local Map Upgrade dispatches this once, at kDataLoaded.
			if (auto* msg = LMU::API::TranslateAs<LMU::API::ExtraMarkerProviderMessage>(a_msg))
			{
				// Rule 30: the function pointer is asked for, not assumed. An older Local Map
				// Upgrade that predates this seam sends a message this add-on will never see, and
				// a null pointer here would be a crash rather than a polite nothing.
				if (!msg->RegisterExtraMarkerProvider)
				{
					logger::error("Local Map Upgrade answered without a registration function - "
								  "it is older than this add-on needs. No markers will be added.");
					return;
				}

				if (msg->RegisterExtraMarkerProvider(&Provider))
				{
					g_registered.store(true);
					logger::info("registered with Local Map Upgrade as its extra-marker provider");
				}
				else
				{
					logger::warn("Local Map Upgrade refused the registration - another add-on got "
								 "there first. This one will stay idle.");
				}
			}
		}
	}

	void Install()
	{
		// The listener is registered by NAME, so it does not matter whether Local Map Upgrade has
		// loaded yet; SKSE routes the dispatch when it happens.
		if (SKSE::GetMessagingInterface()->RegisterListener("LocalMapUpgrade", LMUMessageListener))
		{
			logger::info("listening for Local Map Upgrade's provider dispatch");
		}
		else
		{
			logger::error("could not listen to Local Map Upgrade - is it installed? "
						  "This add-on does nothing without it.");
		}
	}

	bool IsRegistered()
	{
		return g_registered.load();
	}

	Stats GetStats()
	{
		Stats s;
		s.weapons = g_weapons.load();
		s.armour = g_armour.load();
		s.containers = g_containers.load();
		s.questItems = g_questItems.load();
		s.contributed = g_contributed.load();
		s.scanned = g_scanned.load();
		s.cappedAway = g_cappedAway.load();
		s.lastScanMicros = g_lastScanMicros.load();
		s.builds = g_builds.load();
		return s;
	}
}
