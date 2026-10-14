#pragma once

#include "mods/api.h"

class daAlink_c;

// Sprint (DESIGN_VISION.md §2): stamina-driven sprint for human and wolf Link.
//  - Human: keep holding A after a roll to sprint (faster run animation); B while sprinting does a
//    jump slash. Ported from Roll-and-Sprint by MelonSpeedruns (reference-mods/Roll-and-Sprint).
//  - Wolf: keep holding A during the wolf dash to sustain it at full speed, with jump momentum
//    carried over. Ported from Dawnlight (reference-mods/dawnlight/src/wolf_sprint.inc).
// Both drain stamina continuously and stop when Link is exhausted.
namespace remaster::movement::sprint {

ModResult install();

// Called once per 30 Hz tick from player.cpp, before stamina::tick.
void tick(daAlink_c* link);

// Called from the shared decideCommonDoStatus post-hook (stamina.cpp): hides the roll prompt while
// sprinting so holding A does not roll again.
void after_decide_do_status(daAlink_c* link);

}  // namespace remaster::movement::sprint
