#include "core/hooks.hpp"
// Ported from Dawnlight src/manual_shield_hooks.cpp and lazytweaks-modapi
// src/hooks.cpp. Adapted for R shielding without targeting and shared stamina.
#include "combat/manual_shield.hpp"
#include "combat/techniques.hpp"

#include "core/player.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
#include "services.hpp"

namespace remaster::combat::manual_shield {
namespace {
DEFINE_HOOK(&daAlink_c::swordSwingTrigger, SwordSwingTrigger);
DEFINE_HOOK(&daAlink_c::setShieldGuard, SetShieldGuard);
using CheckItemAction = remaster::hooks::Shared0;

thread_local daAlink_c* g_guardAttackOwner = nullptr;

bool enabled(daAlink_c* link) {
    return is_active() && tuning::get().manualShield && link != nullptr &&
           !link->checkWolf() && player::gameplay_running();
}

bool held(daAlink_c* link) {
    return enabled(link) && mDoCPd_c::getHoldLockR(PAD_1) != 0;
}

bool can_bash(daAlink_c* link) {
    return (dComIfGs_isEventBit(dSv_event_flag_c::F_0338) ||
            link->checkNoResetFlg3(daPy_py_c::FLG3_TRANING_SHIELD_ATTACK)) &&
           link->checkGuardActionChange() && !link->checkUpperReadyThrowAnime() &&
           !link->checkModeFlg(0x70C52) && daPy_py_c::checkShieldGet() &&
           !daAlink_c::checkNotBattleStage() &&
           (link->mLinkAcch.ChkGroundHit() || link->checkMagneBootsOn());
}

HookAction before_sword_trigger(ModContext*, void* args, void* result, void*) {
    if (!held(mods::arg<daAlink_c*>(args, 0))) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

void after_shield_guard(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!enabled(link)) return;
    const bool recovering = (link->mProcID == daAlink_c::PROC_GUARD_SLIP &&
                             link->mEquipItem != dItemNo_IRONBALL_e) ||
                            link->checkSmallUpperGuardAnime();
    const bool accepting = held(link) && daPy_py_c::checkShieldGet() &&
        link->checkGuardAccept() && !link->checkGrabAnime() &&
        !link->checkUpperReadyThrowAnime() && !link->checkDkCaught2Anime() &&
        !link->checkKandelaarSwingAnime() && !link->checkCutDashAnime() &&
        !link->checkCutDashChargeAnime() &&
        (!link->checkEquipAnime() || link->checkUpperGuardAnime()) &&
        !link->checkRideOn() && link->checkGuardActionChange();
    if (recovering || accepting) link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    else link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    if (held(link) && can_bash(link)) link->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
}

HookAction before_item_action(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    int techniqueResult = 0;
    if (techniques::try_action(link, techniqueResult)) {
        *static_cast<BOOL*>(result) = techniqueResult;
        return HOOK_SKIP_ORIGINAL;
    }
    if (!held(link) || !can_bash(link)) return HOOK_CONTINUE;
    link->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
    *static_cast<BOOL*>(result) = FALSE;
    if (link->itemTriggerCheck(daAlink_c::BTN_B)) {
        g_guardAttackOwner = link;
        *static_cast<BOOL*>(result) = link->procGuardAttackInit();
        g_guardAttackOwner = nullptr;
    }
    return HOOK_SKIP_ORIGINAL;
}
}

bool allow_guard_attack(daAlink_c* link) {
    return !enabled(link) || g_guardAttackOwner == link;
}

ModResult install() {
    ModResult result = mods::hook::add_pre<SwordSwingTrigger>(before_sword_trigger);
    if (result == MOD_OK) result = mods::hook::add_post<SetShieldGuard>(after_shield_guard);
    if (result == MOD_OK) result = mods::hook::add_pre<CheckItemAction>(before_item_action);
    return result;
}
}
