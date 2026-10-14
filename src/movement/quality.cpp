// Movement behavior adapted from Dusklight v2.0.3's d_a_alink_hang.inc
// and d_a_alink_dusk.cpp. All feature decisions use our own saved controls.
#include "movement/quality.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_midna.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "services.hpp"

namespace remaster::movement::quality {
namespace {
DEFINE_HOOK(&daAlink_c::getHangMoveAnmSpeed, HangSpeed);
DEFINE_HOOK(&daAlink_c::getLadderMoveAnmSpeed, LadderSpeed);
DEFINE_HOOK(&daAlink_c::getClimbMoveUpDownAnmSpeed, ClimbVerticalSpeed);
DEFINE_HOOK(&daAlink_c::getClimbMoveSideAnmSpeed, ClimbSideSpeed);
DEFINE_HOOK(&daAlink_c::setSingleAnimeBaseSpeed, SingleAnimationSpeed);
DEFINE_HOOK(&daAlink_c::setSingleAnimeParam, SingleAnimationParams);
DEFINE_HOOK(&daAlink_c::procHangStartInit, HangStart);
DEFINE_HOOK(&daAlink_c::procClimbUpStartInit, ClimbUpStart);
DEFINE_HOOK(&daAlink_c::procClimbDownStartInit, ClimbDownStart);
DEFINE_HOOK(&daAlink_c::handleQuickTransform, QuickTransform);

template<int Kind>
HookAction speed_before(ModContext*, void* args, void* result, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || link->mpHIO == nullptr) return HOOK_CONTINUE;
    const auto& ladder = link->mpHIO->mLadder.m;
    const auto& wall = link->mpHIO->mWallHang.mWallMove.m;
    float low, high;
    if constexpr (Kind == 0) { low = wall.mMinAnmSpeed; high = wall.mMaxAnmSpeed; }
    if constexpr (Kind == 1) { low = ladder.mMoveMinASpeed; high = ladder.mMoveMaxSpeed; }
    if constexpr (Kind == 2) { low = ladder.mWallVerticalMinAnmSpeed; high = ladder.mWallVerticalMaxAnmSpeed; }
    if constexpr (Kind == 3) { low = ladder.mWallHorizontalMinAnmSpeed; high = ladder.mWallHorizontalMaxAnmSpeed; }
    if (tuning::get().fastClimbing) {
        if constexpr (Kind == 0) low = high = 2.0f;
        if constexpr (Kind == 1) high = 1.575f;
        if constexpr (Kind == 2) high = 1.875f;
        if constexpr (Kind == 3) high = 2.0f;
    }
    *static_cast<float*>(result) = link->getAnmSpeedStickRate(low, high);
    return HOOK_SKIP_ORIGINAL;
}

HookAction animation_speed_before(ModContext*, void* args, void*, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !link->mpHIO) return HOOK_CONTINUE;
    const auto animation = mods::arg<daAlink_c::daAlink_ANM>(args, 1);
    const auto& ladder = link->mpHIO->mLadder.m;
    float speed;
    switch (animation) {
    case daAlink_c::ANM_LADDER_UP_START: speed = ladder.mClimbUpStartASpeed; break;
    case daAlink_c::ANM_LADDER_UP_END_LEFT:
    case daAlink_c::ANM_LADDER_UP_END_RIGHT: speed = ladder.mClimbUpEndASpeed; break;
    case daAlink_c::ANM_LADDER_DOWN_START: speed = ladder.mClimbDownStartASpeed; break;
    case daAlink_c::ANM_LADDER_DOWN_END_LEFT:
    case daAlink_c::ANM_LADDER_DOWN_END_RIGHT: speed = ladder.mClimbDownEndASpeed; break;
    default: return HOOK_CONTINUE;
    }
    if (tuning::get().fastClimbing) {
        speed = animation == daAlink_c::ANM_LADDER_UP_END_LEFT ||
                animation == daAlink_c::ANM_LADDER_UP_END_RIGHT ? 1.765f : 1.8f;
    }
    mods::arg_ref<float>(args, 2) = speed;
    return HOOK_CONTINUE;
}

HookAction animation_params_before(ModContext*, void* args, void*, void*) {
    if (!is_active() || !tuning::get().noMissClimbing) return HOOK_CONTINUE;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto& animation = mods::arg_ref<daAlink_c::daAlink_ANM>(args, 1);
    if (link && link->mpHIO && animation == daAlink_c::ANM_CLIMB_HANG_MISS) {
        animation = daAlink_c::ANM_CLIMB_HANG;
        mods::arg_ref<const daAlinkHIO_anm_c*>(args, 2) = &link->mpHIO->mLadder.m.mWallAttachAnm;
    }
    return HOOK_CONTINUE;
}

void climb_start_after(ModContext*, void* args, void* result, void*) {
    if (!is_active() || !tuning::get().noMissClimbing || !result || !*static_cast<int*>(result)) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !link->mpHIO || !link->checkHangFootWall()) return;
    link->field_0x3478 = link->mpHIO->mLadder.m.mWallAttachAnm.mCancelFrame;
    if (link->mProcID == daAlink_c::PROC_CLIMB_DOWN_START) link->mProcVar0.field_0x3008 = 0;
}

HookAction transform_before(ModContext*, void* args, void*, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    if (!tuning::get().quickTransform) return HOOK_SKIP_ORIGINAL;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !dComIfGs_isEventBit(dSv_event_flag_c::M_077)) return HOOK_SKIP_ORIGINAL;
    auto* meter = dMeter2Info_getMeterClass();
    auto* midna = daPy_py_c::getMidnaActor();
    if (!meter || !meter->getMeterDrawPtr() || !midna || link->checkEventRun() ||
        link->checkStageName("R_SP161") || meter->getMeterDrawPtr()->getButtonZAlpha() != 1.0f ||
        link->mEquipItem == dItemNo_IRONBALL_e || !link->checkNoResetFlg0(daAlink_c::FLG0_SWIM_UP) ||
        !midna->checkMetamorphoseEnableBase() || !link->mLinkAcch.ChkGroundHit() ||
        link->checkModeFlg(daAlink_c::MODE_PLAYER_FLY) || link->checkMagneBootsOn() ||
        !link->checkMidnaRide()) return HOOK_SKIP_ORIGINAL;
    const bool allowed = link->checkWolf()
        ? (link->checkModeFlg(daAlink_c::MODE_UNK_1000) || dComIfGp_checkPlayerStatus0(0, 0x10))
        : (midna->checkMetamorphoseEnable() &&
           (link->checkModeFlg(4) || dComIfGp_checkPlayerStatus0(0, 0x10)));
    if (allowed) link->procCoMetamorphoseInit();
    return HOOK_SKIP_ORIGINAL;
}
}

ModResult install() {
    ModResult r = mods::hook::add_pre<HangSpeed>(speed_before<0>);
    if (r == MOD_OK) r = mods::hook::add_pre<LadderSpeed>(speed_before<1>);
    if (r == MOD_OK) r = mods::hook::add_pre<ClimbVerticalSpeed>(speed_before<2>);
    if (r == MOD_OK) r = mods::hook::add_pre<ClimbSideSpeed>(speed_before<3>);
    if (r == MOD_OK) r = mods::hook::add_pre<SingleAnimationSpeed>(animation_speed_before);
    if (r == MOD_OK) r = mods::hook::add_pre<SingleAnimationParams>(animation_params_before);
    if (r == MOD_OK) r = mods::hook::add_post<HangStart>(climb_start_after);
    if (r == MOD_OK) r = mods::hook::add_post<ClimbUpStart>(climb_start_after);
    if (r == MOD_OK) r = mods::hook::add_post<ClimbDownStart>(climb_start_after);
    if (r == MOD_OK) r = mods::hook::add_pre<QuickTransform>(transform_before);
    return r;
}
}
