// Adapted from dusklight-snappy-turns/src/mod.cpp, integrated with our mode,
// controls and shared wolf movement hook. Original movement always executes.
#include "movement/responsiveness.hpp"
#include "movement/turn_model.hpp"
#include "core/hooks.hpp"
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "d/d_bg_w_base.h"
#include <vector>
namespace remaster::movement::responsiveness {
namespace {
DEFINE_HOOK(&daAlink_c::setSpeedAndAngleNormal, HumanMovement);
struct Capture { void* args; daAlink_c* link; s16 angle,shape; bool eligible; };
std::vector<Capture> g_human,g_wolf;
bool allowed(daAlink_c* link,bool wolf) {
    if (!is_active() || !link || !player::input_allowed() || link->checkWolf()!=wolf ||
        !(wolf?tuning::get().wolfTurning:tuning::get().humanTurning) ||
        link->checkEventRun() || link->checkAttentionLock() || link->mTargetedActor ||
        link->checkBootsOrArmorHeavy() || link->checkMagneBootsOn() ||
        link->mGndPolySpecialCode==dBgW_SPCODE_ICE || !link->mLinkAcch.ChkGroundHit()) return false;
    if (wolf) return (link->mProcID==daAlink_c::PROC_WOLF_WAIT || link->mProcID==daAlink_c::PROC_WOLF_MOVE) &&
        !link->checkAttentionState();
    return link->mProcID==daAlink_c::PROC_WAIT || link->mProcID==daAlink_c::PROC_MOVE;
}
HookAction capture_before(void* args,bool wolf) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    (wolf?g_wolf:g_human).push_back({args,link,link?link->current.angle.y:s16(0),
        link?link->shape_angle.y:s16(0),allowed(link,wolf)});
    return HOOK_CONTINUE;
}
void turn_after(void* args,bool wolf) {
    auto& stack=wolf?g_wolf:g_human;
    if (stack.empty() || stack.back().args!=args) return;
    const auto saved=stack.back(); stack.pop_back();
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!saved.eligible || link!=saved.link || !allowed(link,wolf) || !link->checkInputOnR()) return;
    if (wolf && link->checkEndResetFlg1(daPy_py_c::ERFLG1_UNK_200000)) return; // native wall correction
    const s16 target=link->mMoveAngle;
    const s16 shapeTarget=wolf && link->checkWolfShapeReverse()?s16(target+0x8000):target;
    if (tuning::get().quickPivots && angle_distance(target,link->current.angle.y)>(wolf?0x7000:0x6000) &&
        (!wolf || std::abs(int(link->field_0x3180))<0x5000)) {
        link->current.angle.y=target; link->shape_angle.y=shapeTarget;
        link->speedF*=0.5f; link->mNormalSpeed*=0.5f;
        return;
    }
    const float multiplier=float(std::clamp<int64_t>(wolf?tuning::get().wolfTurnSpeedPercent:tuning::get().humanTurnSpeedPercent,100,500))/100;
    link->current.angle.y=extend_turn(saved.angle,link->current.angle.y,target,multiplier);
    link->shape_angle.y=extend_turn(saved.shape,link->shape_angle.y,shapeTarget,multiplier);
}
HookAction human_pre(ModContext*,void* args,void*,void*) { return capture_before(args,false); }
HookAction wolf_pre(ModContext*,void* args,void*,void*) { return capture_before(args,true); }
void human_post(ModContext*,void* args,void*,void*) { turn_after(args,false); }
void wolf_post(ModContext*,void* args,void*,void*) { turn_after(args,true); }
}
void reset() { g_human.clear(); g_wolf.clear(); }
ModResult install() {
    auto r=mods::hook::add_pre<HumanMovement>(human_pre);
    if (r==MOD_OK) r=mods::hook::add_post<HumanMovement>(human_post);
    if (r==MOD_OK) r=mods::hook::add_pre<hooks::WolfMovement>(wolf_pre);
    if (r==MOD_OK) r=mods::hook::add_post<hooks::WolfMovement>(wolf_post);
    return r;
}
}
