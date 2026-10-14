#pragma once

#include "mods/api.h"

class daAlink_c;

// Shared per-tick hook on Link and gameplay-state helpers.
namespace remaster::player {

// Installs the daAlink_c::execute post-hook that drives all per-tick systems (stamina, dodge).
// Runs once per 30 Hz simulation tick while Link exists.
ModResult install();

// True while normal gameplay is visible: no pause/menu window, event/cutscene, shop or dialogue.
// Port of Dawnlight's combat_meter_hud_visible (reference-mods/dawnlight/src/combat_meter.cpp).
bool gameplay_running();
bool input_allowed();

}  // namespace remaster::player
