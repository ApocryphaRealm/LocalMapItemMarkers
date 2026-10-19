# Local Map Item Markers

Version 1.0.1

An OPTIONAL add-on to Local Map Upgrade. It marks nearby weapons, armour, containers and quest
items on the local map, each kind in its own colour, indoors and out.

REQUIRES Local Map Upgrade 1.2.9 or newer. Earlier versions do not offer the seam this plugin
registers through; install it on one of those and it will simply do nothing and say so in its log.

It replaces one of Local Map Upgrade's files - the marker extension - with a version that knows four
more marker kinds. Let it overwrite it. Uninstalling it puts the original back.

Settings: SKSE\Plugins\LocalMapItemMarkers.ini, or the settings page in the menu framework.
Log: Documents\My Games\Skyrim Special Edition\SKSE\LocalMapItemMarkers.log

GPL-3.0-or-later (see LICENSE and NOTICE.md). Built as an add-on to Local Map Upgrade; the marker
extension its Scaleform file descends from is MIT (alexsylex) - see THIRD_PARTY_NOTICES.md.

WHAT CHANGED
------------

Version 1.0.1
Fixed no item markers appearing when another mod's marker art (Norden UI, for one) overrides Local Map Upgrade's - the extension now supplies its own marker list instead of relying on the art file that loaded first.
Changed the add-on to overwrite one Local Map Upgrade file instead of two; it no longer ships a copy of the marker art, so a UI replacer's art stays as it is.
Added a check for the Address Library file at start-up; if it is missing the plugin says so in its log and stays inactive instead of failing with a bare error.
Changed the licence to GPL-3.0-or-later, matching Local Map Upgrade.
Changed the log to the info level by default; set uLogLevel to 0 or 1 in the INI for more detail.
Version 1.0.0
First release. Marks nearby weapons, armour, containers and quest items on the local map, each category switchable on its own.
