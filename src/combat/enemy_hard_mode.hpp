// Ported from Dawnlight, src/enemy_hard_mode.hpp
// Source commit: 9bb214d47617c9bc295d22facf36c35d17661e5a.
// Adapted to the Definitive Overhaul game mode, shared hooks and tuning.
#pragma once

#include "mods/api.h"

class fopAc_ac_c;

namespace remaster::combat::enemies {
struct EnemySlowStep;

bool enemy_hard_mode_applies(int profileName);
ModResult install_enemy_hard_mode_hooks();
float enemy_hard_mode_turn_scale(const EnemySlowStep& step);
float enemy_hard_mode_chase_scale(const EnemySlowStep& step, const float* value);
void prepare_enemy_hard_mode(EnemySlowStep& step);
void finish_enemy_hard_mode(EnemySlowStep& step);
void reset_enemy_hard_mode();
}
