// Ported from Dawnlight, src/enemy_slow_motion.hpp
// Source commit: 9bb214d47617c9bc295d22facf36c35d17661e5a.
// Adapted to the Definitive Overhaul game mode, shared hooks and tuning.
#pragma once

#include "mods/api.h"

class fopAc_ac_c;

namespace remaster::combat::enemies {
float enemy_slow_motion_scale(fopAc_ac_c* actor);
bool enemy_uses_continuous_slow(fopAc_ac_c* actor);
bool supports_combat_slow(fopAc_ac_c* actor);
ModResult initialize_enemy_slow_motion();
void reset_enemy_slow_motion();
}
