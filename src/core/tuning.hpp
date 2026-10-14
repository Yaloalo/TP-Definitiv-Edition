#pragma once

#include <cstdint>

#include "mods/api.h"
#include "mods/svc/ui.h"

// Player-tunable balance values. Each value is a ConfigService var (persisted in Dusklight's
// config.json as mod.dev_yaloalo_tp__remaster.<name>) and is cached here so hot paths never
// call into the host. Stamina amounts are in points; a full bar is staminaMax points.
namespace remaster::tuning {

struct Values {
    // --- Damage ---
    // Damage Link takes, in percent of vanilla, before the difficulty curve is applied.
    int64_t enemyDamagePercent = 200;
    // Length of Link's invincibility after a normal hit, in percent of vanilla. Knockdowns keep
    // the full duration so Link can get up safely.
    int64_t postHitInvincibilityPercent = 40;
    // Scale damage by area/progress (see difficulty.cpp). Off = flat enemyDamagePercent.
    bool difficultyCurve = true;

    // --- Stamina ---
    bool staminaEnabled = true;
    int64_t staminaMax = 100;
    int64_t staminaRegenPerSecond = 40;
    // Pause after spending stamina before it starts to regenerate.
    int64_t staminaRegenDelayMs = 600;
    // After running dry, actions stay locked until the bar refills to this percentage.
    int64_t exhaustionRecoverPercent = 30;
    int64_t costRoll = 20;
    int64_t costSidestep = 15;  // also backflip
    int64_t costSwing = 12;
    int64_t costSpin = 30;
    int64_t costTechnique = 25;  // jump attack, helm splitter, back slice, ending blow
    int64_t costShieldAttack = 15;

    // --- Movement ---
    bool sprintEnabled = true;
    int64_t sprintSpeedPercent = 100;  // 100 % = Roll-and-Sprint's tuned sprint speed
    int64_t sprintCostPerSecond = 20;
    int64_t climbCostPerSecond = 6;
    int64_t swimDashCostPerSecond = 15;
    bool fastClimbing = true;
    bool noMissClimbing = true;
    bool quickTransform = true;
    bool humanTurning = true, wolfTurning = true, quickPivots = true;
    int64_t humanTurnSpeedPercent = 200, wolfTurnSpeedPercent = 200;
    bool fastCrawling = true;
    int64_t crawlSpeedPercent = 175;
    bool horseHandling = true;
    int64_t horseTurnSpeedPercent = 150, horseAccelerationPercent = 140, horseBrakingPercent = 150;
    bool idleHudFade = true;
    int64_t hudIdleDelayMs = 5000, hudFadeDurationMs = 800, hudIdleOpacityPercent = 0;
    bool damageFeedback = true;
    int64_t damageFeedbackOpacityPercent = 25, damageFeedbackDurationMs = 500;

    // Pacing features are implemented here; no host preset enables them.
    bool instantText = true;
    bool fastTransitions = true;
    bool instantSaves = true;
    bool fastTears = true;
    bool skipRupeeCutscenes = true;
    bool noRupeeReturns = true;

    bool aimingReticle = true;
    bool mouseAim = true;
    bool mouseCamera = true;
    bool invertMouseY = false;
    int64_t mouseAimSensitivityPercent = 100;
    int64_t mouseCameraSensitivityPercent = 100;

    // --- Dodge ---
    // Invincibility window at the start of a roll / sidestep / backflip, in 30 Hz game ticks.
    int64_t dodgeInvincibilityTicks = 8;

    // --- Combat depth ---
    bool manualShield = true;
    bool enemyHardMode = true;
    bool bossHardMode = true;

    bool unrestrictedItems = true;
    bool controllerUi = true;
    bool compassMirrorMode = false;

    // Renewable healing and permanent exploration rewards.
    bool flaskEnabled = true;
    bool removeHeartDrops = true;
    bool removeCurrency = true;
    int64_t flaskStartingCharges = 3;
    int64_t flaskMaximumCharges = 8;
    int64_t flaskHealingHearts = 3;
    bool restStones = true;
    int64_t restNoHitDelayMs = 10000, restChannelMs = 3000;
    bool passiveFlaskRecovery = true;
    int64_t passiveFlaskDelayMs = 90000, passiveFlaskRestMs = 30000;

    int64_t wolfHealingWindupMs = 2000;
    bool ammoRecharge = true;
    bool removeConsumableDrops = true;
    int64_t arrowRechargeDelayMs = 5000, arrowRechargeIntervalMs = 1000;
    int64_t seedRechargeDelayMs = 3000, seedRechargeIntervalMs = 1000;
    int64_t bombRechargeDelayMs = 8000, bombRechargeIntervalMs = 4000;
    int64_t waterBombRechargeDelayMs = 10000, waterBombRechargeIntervalMs = 5000;
    int64_t bomblingRechargeDelayMs = 10000, bomblingRechargeIntervalMs = 6000;
    int64_t oilRechargeDelayMs = 3000, oilRechargeIntervalMs = 1000;
    int64_t baitRechargeDelayMs = 3000, baitRechargeIntervalMs = 2000;
    bool merchantRewards = true;

    // --- Debug ---
    bool debugLog = false;
};

// Current values. Updated on config changes; read freely from the game thread.
const Values& get();

// Registers the config vars. Call once from mod_initialize.
ModResult register_vars();

// Adds the controls for all tuning values to a UI pane.
ModResult build_controls(UiElementHandle pane);

}  // namespace remaster::tuning
