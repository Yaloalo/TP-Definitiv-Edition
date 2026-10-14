#include "combat/stamina.hpp"
#include "core/hooks.hpp"
#include "combat/manual_shield.hpp"
#include "combat/techniques.hpp"

#include <algorithm>

#include "core/player.hpp"
#include "core/tuning.hpp"
#include "progression/rewards.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "movement/sprint.hpp"
#include "services.hpp"

namespace remaster::combat::stamina {
namespace {

constexpr float kTickSeconds = 1.0f / 30.0f;

// --- Gating hooks ---
DEFINE_HOOK(&daAlink_c::decideCommonDoStatus, LinkDecideCommonDoStatus);
using LinkSetStickData = hooks::StickData;

// --- Charging hooks (observe successful proc inits) ---
DEFINE_HOOK(&daAlink_c::procFrontRollInit, LinkFrontRollInit);
DEFINE_HOOK(&daAlink_c::procSideRollInit, LinkSideRollInit);
DEFINE_HOOK(&daAlink_c::procSideStepInit, LinkSideStepInit);
DEFINE_HOOK(&daAlink_c::procBackJumpInit, LinkBackJumpInit);
DEFINE_HOOK(&daAlink_c::procCutNormalInit, LinkCutNormalInit);
DEFINE_HOOK(&daAlink_c::procCutFinishInit, LinkCutFinishInit);
DEFINE_HOOK(&daAlink_c::procCutTurnInit, LinkCutTurnInit);
DEFINE_HOOK(&daAlink_c::procCutJumpInit, LinkCutJumpInit);
DEFINE_HOOK(&daAlink_c::procCutLargeJumpInit, LinkJumpStrikeInit);
DEFINE_HOOK(&daAlink_c::procCutHeadInit, LinkCutHeadInit);
DEFINE_HOOK(&daAlink_c::procCutFinishJumpUpInit, LinkCutFinishJumpUpInit);
DEFINE_HOOK(&daAlink_c::procCutDownInit, LinkCutDownInit);
DEFINE_HOOK(&daAlink_c::procGuardAttackInit, LinkGuardAttackInit);

struct Model {
    float current = 0.0f;
    bool exhausted = false;
    int ticksSinceSpend = 0;
    fpc_ProcID linkId = fpcM_ERROR_PROCESS_ID_e;
    bool initialized = false;
};
Model g_model;

bool enabled() {
    return is_active() && tuning::get().staminaEnabled;
}

float maximum() {
    return static_cast<float>(std::max<int64_t>(1, tuning::get().staminaMax + progression::stamina_bonus()));
}

void refill() {
    g_model.current = maximum();
    g_model.exhausted = false;
    g_model.ticksSinceSpend = 0;
}

// Souls rule: any positive stamina allows starting an action.
bool can_act() {
    return !enabled() || (!g_model.exhausted && g_model.current > 0.0f);
}

void spend(int64_t cost, const char* what) {
    if (!enabled() || cost <= 0) {
        return;
    }
    g_model.current = std::max(0.0f, g_model.current - static_cast<float>(cost));
    g_model.ticksSinceSpend = 0;
    if (g_model.current <= 0.0f) {
        g_model.exhausted = true;
    }
    if (tuning::get().debugLog) {
        mods::log::info("stamina -{} ({}) -> {:.0f}{}", cost, what, g_model.current,
            g_model.exhausted ? " EXHAUSTED" : "");
    }
}

bool succeeded(void* retval) {
    return retval != nullptr && *static_cast<int*>(retval) != 0;
}

// --- Gating ---

bool is_stamina_do_status(u8 status) {
    switch (status) {
    case BUTTON_STATUS_UNK_121:        // roll
    case BUTTON_STATUS_JUMP:           // sidestep / backflip while targeting
    case BUTTON_STATUS_UNK_134:        // jump attack
    case BUTTON_STATUS_HELM_SPLITTER:
    case BUTTON_STATUS_FINISH:         // ending blow
        return true;
    default:
        return false;
    }
}

// Single decideCommonDoStatus hook for the whole mod (one DEFINE_HOOK per target).
void on_decide_do_status_post(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || !is_active()) {
        return;
    }
    movement::sprint::after_decide_do_status(link);
    if (!can_act() && is_stamina_do_status(dComIfGp_getDoStatus())) {
        link->setDoStatus(BUTTON_STATUS_NONE);
    }
}

void on_set_stick_data_post(ModContext*, void* args, void*, void*) {
    if (can_act()) {
        return;
    }
    auto* link = mods::arg<daAlink_c*>(args, 0);
    // Wolf attacks are not part of milestone 1; leave them alone.
    if (link != nullptr && !link->checkWolf()) {
        link->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_B);
    }
}

HookAction on_guard_attack_pre(ModContext*, void* args, void* retval, void*) {
    if (can_act() && manual_shield::allow_guard_attack(mods::arg<daAlink_c*>(args, 0))) {
        return HOOK_CONTINUE;
    }
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 0;
    }
    return HOOK_SKIP_ORIGINAL;
}

// --- Charging ---

template <int64_t tuning::Values::*Cost>
void charge_post(ModContext*, void*, void* retval, void* what) {
    if (succeeded(retval)) {
        spend(tuning::get().*Cost, static_cast<const char*>(what));
    }
}

// Rolls also start automatically when landing from a fall (checkLandAction); only charge rolls the
// player asked for with A this tick.
void on_roll_post(ModContext*, void* args, void* retval, void* what) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (succeeded(retval) && link != nullptr && link->doTrigger()) {
        spend(tuning::get().costRoll, static_cast<const char*>(what));
    }
}

template <class Hook>
ModResult add_charge(HookPostFn fn, const char* what) {
    HookOptions options = HOOK_OPTIONS_INIT;
    options.userdata = const_cast<char*>(what);
    return mods::hook::add_post<Hook>(fn, &options);
}

void on_targeted_dodge_post(ModContext* ctx, void* args, void* result, void* what) {
    charge_post<&tuning::Values::costSidestep>(ctx, args, result, what);
}

void finish_charge_post(ModContext*, void* args, void* result, void*) {
    if (!succeeded(result)) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const bool mortal = link && (link->getCutType() == daPy_py_c::CUT_TYPE_MORTAL_DRAW_A ||
                                 link->getCutType() == daPy_py_c::CUT_TYPE_MORTAL_DRAW_B);
    spend(techniques::finish_cost(mortal ? tuning::get().costTechnique : tuning::get().costSwing), "sword technique");
}
void spin_charge_post(ModContext*, void* args, void* result, void*) {
    if (!succeeded(result)) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const bool great = link && (link->getCutType() == daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT ||
                                link->getCutType() == daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT);
    spend(great && tuning::get().combatTechniques ? tuning::get().costGreatSpin : tuning::get().costSpin, "spin attack");
}

}  // namespace

ModResult install() {
    using V = tuning::Values;
    ModResult r = mods::hook::add_post<LinkDecideCommonDoStatus>(on_decide_do_status_post);
    if (r == MOD_OK) r = mods::hook::add_post<LinkSetStickData>(on_set_stick_data_post);
    if (r == MOD_OK) r = mods::hook::add_pre<LinkGuardAttackInit>(on_guard_attack_pre);

    if (r == MOD_OK) r = add_charge<LinkFrontRollInit>(on_roll_post, "roll");
    if (r == MOD_OK) r = add_charge<LinkSideRollInit>(on_roll_post, "side roll");
    if (r == MOD_OK) r = add_charge<LinkSideStepInit>(on_targeted_dodge_post, "sidestep");
    if (r == MOD_OK) r = add_charge<LinkBackJumpInit>(on_targeted_dodge_post, "back jump");
    if (r == MOD_OK) r = add_charge<LinkCutNormalInit>(charge_post<&V::costSwing>, "swing");
    if (r == MOD_OK) r = add_charge<LinkCutFinishInit>(finish_charge_post, "combo finisher");
    if (r == MOD_OK) r = add_charge<LinkCutTurnInit>(spin_charge_post, "spin attack");
    if (r == MOD_OK) r = add_charge<LinkJumpStrikeInit>(charge_post<&V::costTechnique>, "Jump Strike");
    if (r == MOD_OK) r = add_charge<LinkCutJumpInit>(charge_post<&V::costTechnique>, "jump attack");
    if (r == MOD_OK) r = add_charge<LinkCutHeadInit>(charge_post<&V::costTechnique>, "helm splitter");
    if (r == MOD_OK) r = add_charge<LinkCutFinishJumpUpInit>(charge_post<&V::costTechnique>, "back slice");
    if (r == MOD_OK) r = add_charge<LinkCutDownInit>(charge_post<&V::costTechnique>, "ending blow");
    if (r == MOD_OK) r = add_charge<LinkGuardAttackInit>(charge_post<&V::costShieldAttack>, "shield attack");
    return r;
}

void tick(daAlink_c* link) {
    if (!tuning::get().staminaEnabled) {
        refill();
        return;
    }
    // New Link actor: new save, scene change or respawn after death.
    const fpc_ProcID id = fopAcM_GetID(link);
    if (!g_model.initialized || id != g_model.linkId) {
        g_model.initialized = true;
        g_model.linkId = id;
        refill();
        return;
    }
    g_model.current = std::min(g_model.current, maximum());
    if (!player::gameplay_running()) {
        return;  // frozen in menus, cutscenes, dialogue
    }

    const tuning::Values& values = tuning::get();
    ++g_model.ticksSinceSpend;
    const float sinceSpendMs = static_cast<float>(g_model.ticksSinceSpend) * kTickSeconds * 1000.0f;
    if (sinceSpendMs >= static_cast<float>(values.staminaRegenDelayMs)) {
        g_model.current = std::min(maximum(),
            g_model.current + static_cast<float>(values.staminaRegenPerSecond) * kTickSeconds);
    }
    if (g_model.exhausted &&
        g_model.current >= maximum() * static_cast<float>(values.exhaustionRecoverPercent) / 100.0f) {
        g_model.exhausted = false;
        if (values.debugLog) {
            mods::log::info("stamina recovered from exhaustion");
        }
    }
}

void drain(float amount, const char* what) {
    if (!enabled() || amount <= 0.0f) {
        return;
    }
    const bool wasExhausted = g_model.exhausted;
    g_model.current = std::max(0.0f, g_model.current - amount);
    g_model.ticksSinceSpend = 0;
    if (g_model.current <= 0.0f) {
        g_model.exhausted = true;
        if (!wasExhausted && tuning::get().debugLog) {
            mods::log::info("stamina exhausted by {}", what);
        }
    }
}

State state() {
    if (!enabled()) {
        return {maximum(), maximum(), false};
    }
    return {g_model.current, maximum(), g_model.exhausted};
}

}  // namespace remaster::combat::stamina
