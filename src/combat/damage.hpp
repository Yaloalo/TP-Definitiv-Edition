#pragma once

#include "mods/api.h"

// Incoming damage model: enemy damage scaling (with the difficulty curve) and shorter
// post-hit invincibility. DESIGN_VISION.md §3, §9.
namespace remaster::combat::damage {

// Installs the hooks once; they are inert while the game mode is inactive.
ModResult install();

}  // namespace remaster::combat::damage
