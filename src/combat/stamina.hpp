#pragma once

#include "mods/api.h"

class daAlink_c;

// Stamina (DESIGN_VISION.md: movement and combat). Souls-style rules:
//  - an action may start whenever stamina is above zero; its full cost is then deducted (floored at 0)
//  - running dry exhausts Link: stamina actions stay locked until the bar refills to
//    exhaustionRecoverPercent
//  - regeneration starts staminaRegenDelayMs after the last spend and freezes outside gameplay
//
// Gating happens at the input/decision layer, never by skipping a procXxxInit whose return value
// callers ignore (see REMASTER_PLAN.md / research notes):
//  - A-button actions: do-status cleared in a decideCommonDoStatus post-hook (as Roll-and-Sprint)
//  - B-button actions: BTN_B removed from mItemTrigger in a setStickData post-hook
//  - shield attack: procGuardAttackInit pre-skip (its only input caller uses the return value)
// Costs are charged in post-hooks of the proc inits, only when the init reports success.
namespace remaster::combat::stamina {

struct State {
    float current;
    float maximum;
    bool exhausted;
};

ModResult install();

// Called once per 30 Hz tick from player.cpp while the game mode is active.
void tick(daAlink_c* link);

// Current state for the HUD. When the system is disabled, current == maximum.
State state();

// Continuous drain (sprint, climbing, swimming): deducts `amount`, delays regeneration and
// exhausts Link at zero, like an action cost, but without per-tick logging.
void drain(float amount, const char* what);

}  // namespace remaster::combat::stamina
