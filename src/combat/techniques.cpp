#include "combat/techniques.hpp"
#include "combat/technique_model.hpp"
#include "combat/stamina.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "input/combat_controller.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include <algorithm>
#include <string>

namespace remaster::combat::techniques {
namespace {
DEFINE_HOOK(&daAlink_c::checkCutLargeTurnState, GreatSpin);
DEFINE_HOOK(&daAlink_c::setCutType, CutType);
DEFINE_HOOK(&daAlink_c::procGuardSlipInit, GuardSlip);
DEFINE_HOOK(&daAlink_c::setSmallGuard, SmallGuard);
DEFINE_HOOK(&daAlink_c::checkCutAction, CutAction);
InputState g_input;
Attack g_start = Attack::Ordinary, g_attack = Attack::Ordinary;
uint64_t g_serial = 0;
daAlink_c* g_owner = nullptr;
fpc_ProcID g_id = fpcM_ERROR_PROCESS_ID_e;
std::string g_stage;
int g_room = -1;

bool enabled(daAlink_c* link) {
    return is_active() && tuning::get().combatTechniques && link &&
        link == daAlink_getAlinkActorClass() && !link->checkWolf() &&
        player::input_allowed() && !link->checkEventRun() && dComIfGs_getLife() > 0;
}
bool ready() { const auto st = stamina::state(); return !st.exhausted && st.current > 0; }
void sample_before(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link) { reset(); return; }
    const auto id = fopAcM_GetID(link);
    const std::string stage(dComIfGp_getStartStageName());
    const auto room = dComIfGp_getStartStageRoomNo();
    if (g_owner != link || g_id != id || stage != g_stage || room != g_room) {
        reset(); g_owner = link; g_id = id; g_stage = stage; g_room = room;
    }
    const auto sample = input::combat_controller::sample();
    g_input.sample(sample.heavy, enabled(link) && sample.focused, sample.device);
    if (!enabled(link)) g_attack = Attack::Ordinary;
    if (link->mProcID != daAlink_c::PROC_CUT_FINISH) g_attack = Attack::Ordinary;
}
HookAction execute_pre(ModContext* c, void* a, void* r, void* u) {
    sample_before(c, a, r, u); return HOOK_CONTINUE;
}
void cut_type_post(ModContext*, void* args, void*, void*) {
    if (mods::arg<daAlink_c*>(args, 0) != g_owner) return;
    ++g_serial;
    g_attack = g_start;
}
HookAction cut_action_pre(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!enabled(link) || ready()) return HOOK_CONTINUE;
    // This decision function's callers use its result. Do not skip native proc
    // initializers whose callers may already have committed to an action.
    link->clearComboReserb();
    *static_cast<int*>(result) = 0;
    return HOOK_SKIP_ORIGINAL;
}
HookAction great_spin_pre(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!is_active() || !tuning::get().combatTechniques || !link) return HOOK_CONTINUE;
    // Preserve teaching/cinematic script behavior. Gameplay ignores launcher
    // Great Spin presets and owns both the learned gate and no-health rule.
    if (link->checkEventRun()) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = dComIfGs_isEventBit(dSv_event_flag_c::F_0344) ||
        link->checkNoResetFlg3(daPy_py_c::FLG3_TRANING_CUT_LARGE_TURN);
    return HOOK_SKIP_ORIGINAL;
}
bool ordinary_shield(daAlink_c* link) {
    return enabled(link) && !link->checkRideOn() &&
        link->mEquipItem != dItemNo_IRONBALL_e && daPy_py_c::checkShieldGet();
}
void block_received(daAlink_c* link) {
    stamina::drain(static_cast<float>(tuning::get().costBlock), "blocked impact");
    if (!ready()) {
        g_input.consume(); link->procGuardBreakInit();
    } else g_input.blocked(30);
}
HookAction guard_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (ordinary_shield(link) && link->mProcID != daAlink_c::PROC_GUARD_SLIP)
        link->field_0x2fd5 = 0; // Replace the native every-fourth-block break with stamina.
    return HOOK_CONTINUE;
}
void guard_post(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (ordinary_shield(link) && result && *static_cast<int*>(result) &&
        link->mProcID == daAlink_c::PROC_GUARD_SLIP) block_received(link);
}
void small_guard_post(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (ordinary_shield(link)) block_received(link);
}
void loaded(ModContext*, uint32_t, void*) { reset(); }
}

bool try_action(daAlink_c* link, int& result) {
    if (!enabled(link) || !g_input.pending) return false;
    if (!ready()) { g_input.consume(); result = 0; return true; }
    if (link->mEquipItem != 0x103 || !link->mLinkAcch.ChkGroundHit() ||
        link->checkRideOn() || link->checkGrabAnime() || link->checkEquipAnime() ||
        link->checkUpperReadyThrowAnime() || link->checkModeFlg(0x70C52) ||
        daAlink_c::checkNotBattleStage()) return false;
    // Only neutral grounded actions (or completed guard recovery when native
    // checkItemAction is called). Never cancel another swing, dodge or item.
    switch (link->mProcID) {
    case daAlink_c::PROC_WAIT: case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_ACTOR_WAIT: case daAlink_c::PROC_ATN_ACTOR_MOVE:
    case daAlink_c::PROC_GUARD_SLIP: break;
    default: return false;
    }
    g_start = g_input.counter > 0 ? Attack::GuardCounter : Attack::Heavy;
    link->resetCombo(TRUE);
    // Existing full finisher animations, entered as dedicated actions. Native
    // collision, wall recoil, sounds and damage reactions continue to execute.
    // Private cut.inc enum: CUT_FINISH_PARAM_VERTICAL=1, RIGHT=5.
    result = link->procCutFinishInit(g_start == Attack::Heavy ? 1 : 5);
    if (result) {
        const float rate = g_start == Attack::Heavy ? 0.75f : 0.9f;
        link->mUnderFrameCtrl[0].setRate(link->mUnderFrameCtrl[0].getRate() * rate);
        link->mUpperFrameCtrl[0].setRate(link->mUpperFrameCtrl[0].getRate() * rate);
        link->field_0x3484 = std::max(link->field_0x3484, link->field_0x347c + 6.0f);
    }
    g_start = Attack::Ordinary;
    g_input.consume();
    return true;
}
int64_t finish_cost(int64_t ordinary) {
    if (g_start == Attack::Heavy) return tuning::get().costHeavy;
    if (g_start == Attack::GuardCounter) return tuning::get().costGuardCounter;
    return ordinary;
}
Attack current_attack(const daAlink_c* link) {
    return link == g_owner && link->mProcID == daAlink_c::PROC_CUT_FINISH ? g_attack : Attack::Ordinary;
}
uint64_t attack_serial() { return g_serial; }
void reset() {
    g_input = {}; g_start = g_attack = Attack::Ordinary;
    g_owner = nullptr; g_id = fpcM_ERROR_PROCESS_ID_e; g_stage.clear(); g_room = -1;
    ++g_serial; // Never recycle a previous contact identity on reset/load.
}
ModResult install() {
    auto r = input::combat_controller::install();
    if (r == MOD_OK) r = mods::hook::add_pre<hooks::LinkExecute>(execute_pre);
    if (r == MOD_OK) r = mods::hook::add_post<CutType>(cut_type_post);
    if (r == MOD_OK) r = mods::hook::add_pre<CutAction>(cut_action_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<GreatSpin>(great_spin_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<GuardSlip>(guard_pre);
    if (r == MOD_OK) r = mods::hook::add_post<GuardSlip>(guard_post);
    if (r == MOD_OK) r = mods::hook::add_post<SmallGuard>(small_guard_post);
    if (r == MOD_OK) r = svc_save->observe_saves(mod_ctx, loaded, loaded, nullptr, nullptr, nullptr);
    return r;
}
}
