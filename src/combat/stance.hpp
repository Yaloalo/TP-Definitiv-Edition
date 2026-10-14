#pragma once
#include "mods/api.h"
class fopAc_ac_c;
class daAlink_c;
namespace remaster::combat::stance {
ModResult install();
void reset();
void tick();
bool owns_bokoblin(fopAc_ac_c* actor);
// Returns true only when the first supported adapter accepted an actual melee parry.
bool parry(fopAc_ac_c* attacker, daAlink_c* link);
}
