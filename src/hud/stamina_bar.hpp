#pragma once

#include "mods/api.h"

// Stamina bar under the hearts. Reuses the game's own magic/lantern meter panes from the Kantera
// screen (tinted green, orange when exhausted), so it inherits the native HUD look and scaling.
// Ported from Dawnlight (reference-mods/dawnlight/src/combat_meter.cpp) without its HUD-layout
// editor; fades out while full, like Breath of the Wild.
namespace remaster::hud::stamina_bar {

ModResult install();

}  // namespace remaster::hud::stamina_bar
