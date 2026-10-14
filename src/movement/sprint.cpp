#include "movement/sprint.hpp"

#include <algorithm>
#include <vector>

#include "combat/stamina.hpp"
#include "core/hooks.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
#include "services.hpp"

namespace remaster::movement::sprint {
namespace {

constexpr float kTickSeconds = 1.0f / 30.0f;
// Roll-and-Sprint's tuned run speed and animation rate at 100 %.
constexpr float kSprintNormalSpeed = 35.0f;
constexpr float kSprintAnimRate = 2.0f;

// --- Human sprint (Roll-and-Sprint) ---
DEFINE_HOOK(&daAlink_c::procMoveInit, LinkProcMoveInit);
DEFINE_HOOK(&daAlink_c::setDoubleAnime, LinkSetDoubleAnime);
DEFINE_HOOK(&daAlink_c::checkNormalAction, LinkCheckNormalAction);

// --- Wolf sprint (Dawnlight) ---
using WolfSpeedAndAngle=remaster::hooks::WolfMovement;
DEFINE_HOOK(&daAlink_c::procWolfDash, WolfDash);
DEFINE_HOOK(&daAlink_c::procWolfAutoJumpInit, WolfAutoJumpInit);

bool g_running = false;
daAlink_c* g_wolfSprintOwner = nullptr;
bool g_wolfSprintThisTick = false;

bool enabled() {
    return is_active() && tuning::get().sprintEnabled;
}

float speed_scale() {
    return static_cast<float>(tuning::get().sprintSpeedPercent) / 100.0f;
}

// Starting needs stamina; once running, sprint continues until stamina is exhausted.
bool stamina_allows_sprint() {
    const combat::stamina::State st = combat::stamina::state();
    return !st.exhausted && st.current > 0.0f;
}

HookAction on_proc_move_init_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!enabled() || g_running || link == nullptr || link->mProcID != daAlink_c::PROC_FRONT_ROLL) {
        return HOOK_CONTINUE;
    }
    if (mDoCPd_c::getHoldA(PAD_1) != 0 && !link->checkEventRun() && !link->checkBootsOrArmorHeavy() &&
        stamina_allows_sprint()) {
        if (link->mEquipItem != 0xFF) {
            link->allUnequip(0);
        }
        link->setSwordVoiceSe(Z2SE_AL_V_THROW_IB);
        g_running = true;
        link->setDoStatus(BUTTON_STATUS_NONE);  // avoid a roll prompt flashing on the first frame
    }
    return HOOK_CONTINUE;
}

HookAction on_set_double_anime_pre(ModContext*, void* args, void*, void*) {
    if (!g_running) {
        return HOOK_CONTINUE;
    }
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto& anmB = mods::arg_ref<daAlink_c::daAlink_ANM>(args, 5);
    if (link != nullptr && anmB == daAlink_c::ANM_RUN) {
        mods::arg_ref<f32>(args, 3) = kSprintAnimRate * speed_scale();
        anmB = daAlink_c::ANM_RUN_B;
        link->mNormalSpeed = kSprintNormalSpeed * speed_scale();
    }
    return HOOK_CONTINUE;
}

// B while sprinting: jump slash. Stamina gating (stamina.cpp) already removes B when Link cannot
// act, and procCutJumpInit is charged as a technique.
HookAction on_check_normal_action_pre(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!g_running || link == nullptr || !link->swordSwingTrigger()) {
        return HOOK_CONTINUE;
    }
    const u8 equipSword = dComIfGs_getSelectEquipSword();
    if (equipSword == 0xFF) {
        return HOOK_CONTINUE;
    }
    if (!link->checkSwordEquipAnime()) {
        link->seStartSwordCut(daPy_py_c::checkWoodSwordEquip() ? Z2SE_AL_ITEM_TAKEOUT_FAST : Z2SE_AL_SWORD_PULLOUT);
    }
    link->mEquipItem = 0x103;  // sword
    link->setItemModel();
    link->procCutJumpInit(FALSE);
    g_running = false;
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

// --- Wolf sprint ---

float wolf_dash_speed(daAlink_c* link) {
    const auto& move = link->mpHIO->mWolf.mWlMove.m;
    if (link->checkWolfSlowDash()) {
        return move.mADashMaxSpeedSlow;
    }
    if (link->field_0x2fc7 == 2) {
        return move.mADashMaxSpeedSlow2;
    }
    return move.mADashMaxSpeed;
}

bool wolf_sprint_requested(daAlink_c* link) {
    if (!enabled() || link == nullptr || !link->checkWolf() || link->mpHIO == nullptr || !link->doButton() ||
        !link->checkInputOnR() || !link->checkWolfDashMode() || !link->mLinkAcch.ChkGroundHit() ||
        !stamina_allows_sprint()) {
        return false;
    }
    if (link->mProcID != daAlink_c::PROC_WOLF_WAIT && link->mProcID != daAlink_c::PROC_WOLF_MOVE &&
        link->mProcID != daAlink_c::PROC_WOLF_DASH) {
        return false;
    }
    return !link->checkPlayerDemoMode() && !link->checkEventRun() && !dComIfGp_event_runCheck() &&
           !link->checkAttentionState() && !link->checkSlope() && !link->checkNotJumpSinkLimit() &&
           !link->checkWolfGrabAnime() && link->mGrabItemAcKeep.getActor() == nullptr;
}

HookAction on_wolf_movement_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!wolf_sprint_requested(link)) {
        if (link != nullptr && g_wolfSprintOwner == link) {
            // The dash initializer sets this limit only once; undo our override on release.
            if (link->mProcID == daAlink_c::PROC_WOLF_DASH && link->mpHIO != nullptr) {
                link->mMaxSpeed = wolf_dash_speed(link);
            }
            g_wolfSprintOwner = nullptr;
        }
        return HOOK_CONTINUE;
    }
    // Renew the dash state (not its start animation); native steering and transitions still run.
    link->field_0x30d0 = std::max<s16>(link->field_0x30d0, 2);
    link->mMaxSpeed = wolf_dash_speed(link) * speed_scale();
    g_wolfSprintOwner = link;
    g_wolfSprintThisTick = true;
    return HOOK_CONTINUE;
}

struct WolfJumpMomentum {
    daAlink_c* link;
    void* args;
    float speed;
    bool carry;
};
std::vector<WolfJumpMomentum> g_wolfJumps;

HookAction on_wolf_jump_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const bool carry = enabled() && link != nullptr && link->checkWolf() && g_wolfSprintOwner == link &&
                       link->checkWolfDashMode() && !link->checkPlayerDemoMode() && !link->checkEventRun() &&
                       !dComIfGp_event_runCheck();
    g_wolfJumps.push_back({link, args, carry ? link->speedF : 0.0f, carry});
    return HOOK_CONTINUE;
}

void on_wolf_jump_post(ModContext*, void* args, void* retval, void*) {
    if (g_wolfJumps.empty() || g_wolfJumps.back().args != args) {
        return;
    }
    const WolfJumpMomentum saved = g_wolfJumps.back();
    g_wolfJumps.pop_back();
    if (!saved.carry || retval == nullptr || *static_cast<int*>(retval) == 0 ||
        saved.link->mProcID != daAlink_c::PROC_WOLF_AUTO_JUMP) {
        return;
    }
    saved.link->mNormalSpeed = saved.speed;
    saved.link->speedF = saved.speed;
    saved.link->mMaxSpeed = std::max(saved.link->mMaxSpeed, saved.speed);
}

}  // namespace

ModResult install() {
    ModResult r = mods::hook::add_pre<LinkProcMoveInit>(on_proc_move_init_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<LinkSetDoubleAnime>(on_set_double_anime_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<LinkCheckNormalAction>(on_check_normal_action_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<WolfSpeedAndAngle>(on_wolf_movement_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<WolfDash>(on_wolf_movement_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<WolfAutoJumpInit>(on_wolf_jump_pre);
    if (r == MOD_OK) r = mods::hook::add_post<WolfAutoJumpInit>(on_wolf_jump_post);
    return r;
}

void tick(daAlink_c* link) {
    const tuning::Values& values = tuning::get();
    const float drain = static_cast<float>(values.sprintCostPerSecond) * kTickSeconds;

    if (g_running) {
        const bool stop = !enabled() || mDoCPd_c::getHoldA(PAD_1) == 0 || link->checkEventRun() ||
                          link->mProcID != daAlink_c::PROC_MOVE || !stamina_allows_sprint();
        if (stop) {
            g_running = false;
        } else {
            combat::stamina::drain(drain, "sprint");
        }
    }
    if (g_wolfSprintThisTick) {
        g_wolfSprintThisTick = false;
        combat::stamina::drain(drain, "wolf sprint");
    }
}

void after_decide_do_status(daAlink_c* link) {
    if (g_running && link != nullptr && dComIfGp_getDoStatus() == BUTTON_STATUS_UNK_121) {
        link->setDoStatus(BUTTON_STATUS_NONE);
    }
}

}  // namespace remaster::movement::sprint
