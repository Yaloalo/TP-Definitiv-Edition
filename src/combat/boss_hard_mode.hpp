// Ported from Dawnlight, src/boss_hard_mode.hpp
// Source commit: 9bb214d47617c9bc295d22facf36c35d17661e5a.
// Adapted to the Definitive Overhaul game mode, shared hooks and tuning.
#pragma once

#include "mods/api.h"

namespace remaster::combat::enemies {

ModResult install_boss_hard_mode_hooks();
void before_boss_hard_collision(void* args);
void reset_boss_hard_mode();

}  // namespace remaster::combat::enemies
