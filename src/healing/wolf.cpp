#include "healing/wolf.hpp"
#include "core/hooks.hpp"
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "progression/rewards.hpp"
#include "resources/recharge_model.hpp"
#include "services.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
namespace remaster::healing::wolf {
namespace {
DEFINE_HOOK(&daAlink_c::procWolfSit, WolfSit);
DEFINE_HOOK(&daAlink_c::handleWolfHowl, RecoveryCombo);
daAlink_c* g_owner = nullptr;
int g_ticks = 0, g_windup = 60;
bool g_healed = false;
bool enabled() { return is_active() && tuning::get().flaskEnabled && progression::ready(); }
bool allowed_proc(u16 proc) {
    return proc == daAlink_c::PROC_WOLF_WAIT || proc == daAlink_c::PROC_WOLF_MOVE ||
        proc == daAlink_c::PROC_WOLF_SIT || proc == daAlink_c::PROC_WOLF_TIRED_WAIT || proc == daAlink_c::PROC_WOLF_WAIT_TURN;
}
void lock_input(daAlink_c* link) {
    link->mItemTrigger &= ~(daAlink_c::BTN_A | daAlink_c::BTN_B | daAlink_c::BTN_X | daAlink_c::BTN_Y);
    link->mItemButton &= ~(daAlink_c::BTN_A | daAlink_c::BTN_B | daAlink_c::BTN_X | daAlink_c::BTN_Y);
    link->mStickValue = link->mMoveValue = link->mNormalSpeed = link->speedF = 0.0f;
}
bool begin_recovery(daAlink_c* link) {
    if (!link || !enabled() || g_owner || !link->checkWolf() || !player::input_allowed() ||
        !allowed_proc(link->mProcID) || !link->mLinkAcch.ChkGroundHit() || dComIfGs_getLife() == 0 ||
        dComIfGs_getLife() >= dComIfGs_getMaxLifeGauge()) return false;
    auto& state = progression::model();
    if (!state.charges) return false;
    if (link->mProcID != daAlink_c::PROC_WOLF_SIT && !link->procWolfSitInit(1)) return false;
    if (!state.spend(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges)) return false;
    g_owner = link; g_ticks = 0; g_healed = false;
    g_windup = std::max(1, resources::ticks_for_ms(tuning::get().wolfHealingWindupMs));
    lock_input(link); progression::persist(); return true;
}
HookAction combo_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!enabled() || !link || !link->checkWolf()) return HOOK_CONTINUE;
    // The engine consumes R+X before setStickData. Own its existing dispatcher target so the
    // gesture works with all native cheats off; ordinary howling-stone A actions are untouched.
    begin_recovery(link); return HOOK_SKIP_ORIGINAL;
}
void input_post(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (enabled() && link && g_owner == link) lock_input(link);
}
HookAction sit_pre(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!enabled() || link != g_owner) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 1;
    if (dComIfGp_event_runCheck() || !link->checkWolf() || !link->mLinkAcch.ChkGroundHit()) {
        interrupt(link); return HOOK_SKIP_ORIGINAL;
    }
    if (!player::input_allowed()) return HOOK_SKIP_ORIGINAL; // no progress while paused/in menus
    lock_input(link);
    if (link->checkAnmEnd(link->mUnderFrameCtrl) && !link->checkUnderMove0BckNoArcWolf(daAlink_c::WANM_WAIT_SIT))
        link->setSingleAnimeWolfBase(daAlink_c::WANM_WAIT_SIT);
    ++g_ticks;
    if (!g_healed && g_ticks >= g_windup) {
        dComIfGp_setItemLifeCount(progression::model().potency(tuning::get().flaskHealingHearts) * 4.0f, 0);
        g_healed = true;
    }
    if (g_ticks >= g_windup + 18) interrupt(link); // 0.6 s recovery, still vulnerable
    return HOOK_SKIP_ORIGINAL;
}
}
bool busy() { return g_owner != nullptr; }
void interrupt(daAlink_c* link) {
    if (link && link == g_owner) { g_owner = nullptr; g_ticks = 0; link->procWolfWaitInit(); }
}
void reset() {
    auto* link = daAlink_getAlinkActorClass();
    if (link && link == g_owner && link->mProcID == daAlink_c::PROC_WOLF_SIT) interrupt(link);
    g_owner = nullptr; g_ticks = 0; g_healed = false;
}
void tick(daAlink_c* link) {
    if (!enabled()) { reset(); return; }
    if (g_owner && (g_owner != link || !link->checkWolf() || link->mProcID != daAlink_c::PROC_WOLF_SIT)) {
        g_owner = nullptr; g_ticks = 0; g_healed = false;
    }
}
ModResult install() {
    auto r = mods::hook::add_post<hooks::StickData>(input_post);
    if (r == MOD_OK) r = mods::hook::add_pre<RecoveryCombo>(combo_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<WolfSit>(sit_pre);
    return r;
}
}
