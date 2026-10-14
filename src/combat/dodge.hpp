#pragma once

#include "mods/api.h"

class daAlink_c;

// Dodge invincibility (DESIGN_VISION.md §3). Vanilla dodges have none: setCollision only disables
// Link's hurt cylinders while mDamageTimer != 0 or MODE_HIT_STUN is set (d_a_alink.cpp:6833), and
// the dodge procs carry neither. During the first dodgeInvincibilityTicks of a roll, side roll,
// sidestep/backflip or back jump, the hurt cylinders are switched off the same way the game does it.
// mDamageTimer is deliberately not used: it also flashes Link in the damage colour.
namespace remaster::combat::dodge {

ModResult install();

// Called once per 30 Hz tick from player.cpp while the game mode is active.
void tick(daAlink_c* link);

// Called from the (single) checkDamageAction pre-hook in damage.cpp: discards hits already
// recorded on the hurt cylinders while a dodge window is active.
void before_damage_check(daAlink_c* link);

}  // namespace remaster::combat::dodge
