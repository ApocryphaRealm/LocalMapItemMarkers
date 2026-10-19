# LocalMapItemMarkers - changelog

Written as changes happen, not reconstructed afterwards (rule 61). Each version carries its
**version-ledger status**, so this file cannot quietly claim more than the ledger does:

* **working** - observed running in game
* **untested** - built and packaged, not yet confirmed
* **failed** - built but crashed or malfunctioned; the number was reclaimed
* **scratch** - a hypothesis-test build that never held a real number

## 1.0.1 - 2026-10-18 - untested

### Fixed
- No item markers at all when another mod's IconDisplayExtensionArt.swf wins the file conflict (Norden UI, Local Map Upgrade's own, Norden UI Black): the class that holds the icon list lives in that art file and has six entries, so the extra four icon types resolved to nothing and every item marker was dropped silently, indoors and out. The extension now completes the list (and the four tints) itself before it places a marker, whichever art file was loaded first.

### Added
- DevBench tool lmim.status gains op=questprobe (lists references carrying quest-alias data and every runtime-created one, with the engine's own answer and flags) and op=undelete:<ref> (test fixture: clears the Deleted flag on one reference). Proven in game 2026-10-18: a quest's reserved alias item (MQ103 Dragonstone) is classified as a quest item (icon 9) but carries the engine's Deleted flag until the quest really fills it, and the marker build skips deleted references on purpose; with the flag cleared the green quest-item marker drew at the item.
- Address Library guard at start-up (gate row skse-plugin-carries-address-library-guard): a missing file is named in the log and the plugin loads inert.
### Changed
- Ships at log level info (uLogLevel=2, compiled default and INI) instead of trace, as every mod of ours now does (pre-finalize-check).
- Licence is GPL-3.0-or-later like every other mod of ours (LICENSE, NOTICE.md, THIRD_PARTY_NOTICES.md; the alexsylex MIT notice for the two Scaleform files is kept). 1.0.0 had been labelled MIT.
### Removed
- The add-on no longer ships its own copy of IconDisplayExtensionArt.swf. The shared marker art is Local Map Upgrade's (or a UI replacer's, such as Norden UI); the extension now needs nothing from it beyond the NeutralMarker symbol every version has, so the add-on overwrites ONE of Local Map Upgrade's files instead of two and no longer fights a UI replacer over the art.
### Known
- Dragon's Eye Minimap (1.7.4 tested) draws none of these markers on its minimap: only the real local map swaps them in (the extension's PostCreateMarkers never fires for the minimap), while the provider still runs about 20 times a second under the minimap (about 130 microseconds each, measured). The 1.0.0 page text saying they show on the minimap is wrong. When a build takes more than 60 matches the kept 60 are the first found, not the nearest.