// Keep native obstacle checks, backwards movement, spur limits and animation
// selection. Only ordinary, player-controlled riding receives the modifiers.
#include "movement/horse.hpp"
#include "movement/turn_model.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "core/hooks.hpp"
#include "d/actor/d_a_horse.h"
#include <vector>
#include <cmath>

namespace remaster::movement::horse {
namespace {
DEFINE_HOOK(&daHorse_c::setSpeedAndAngle, HorseMovement);
struct Capture { void* args; daHorse_c* horse; s16 angle,shape; bool eligible; };
std::vector<Capture> g_calls;
bool allowed(daHorse_c* horse) {
    auto* link=daAlink_getAlinkActorClass();
    if (!is_active() || !tuning::get().horseHandling || !horse || !link ||
        !player::input_allowed() || !link->checkHorseRideOn() || link->getRideActor()!=horse ||
        link->checkEventRun() || link->checkHorseSubjectivity() || link->checkAttentionLock() ||
        link->mTargetedActor || link->checkHorseHangMode() || horse->m_demoMode!=0 ||
        !horse->m_acch.ChkGroundHit() || horse->m_cowHit || horse->field_0x1702 ||
        horse->checkStateFlg0(daHorse_c::daHorse_FLG0(daHorse_c::FLG0_RODEO_MODE |
            daHorse_c::FLG0_UNK_10000000 | daHorse_c::FLG0_UNK_100000 | daHorse_c::FLG0_UNK_200000 |
            daHorse_c::FLG0_UNK_40000 | daHorse_c::FLG0_UNK_4))) return false;
    return horse->m_procID==daHorse_c::PROC_WAIT_e || horse->m_procID==daHorse_c::PROC_MOVE_e;
}
float multiplier(int64_t percent) { return float(std::clamp<int64_t>(percent,100,300))/100; }
HookAction movement_pre(ModContext*,void* args,void*,void*) {
    auto* horse=mods::arg<daHorse_c*>(args,0);
    g_calls.push_back({args,horse,horse?horse->current.angle.y:s16(0),
        horse?horse->shape_angle.y:s16(0),allowed(horse)});
    return HOOK_CONTINUE;
}
HookAction chase_pre(ModContext*,void* args,void*,void*) {
    if (g_calls.empty() || !g_calls.back().eligible) return HOOK_CONTINUE;
    auto* horse=g_calls.back().horse;
    auto* value=mods::arg<float*>(args,0);
    if (value!=&horse->speedF || !allowed(horse)) return HOOK_CONTINUE;
    const float target=mods::arg<float>(args,1);
    auto& step=mods::arg_ref<float>(args,2);
    // Reverse movement retains its native rates. chaseF retains its own clamp,
    // so a larger step cannot overshoot a stop or exceed the native speed cap.
    if (!std::isfinite(target) || !std::isfinite(step) || step<0 || target<0 || horse->speedF<0) return HOOK_CONTINUE;
    step*=multiplier(target>horse->speedF?tuning::get().horseAccelerationPercent:tuning::get().horseBrakingPercent);
    return HOOK_CONTINUE;
}
void movement_post(ModContext*,void* args,void* result,void*) {
    if (g_calls.empty() || g_calls.back().args!=args) return;
    const auto saved=g_calls.back(); g_calls.pop_back();
    auto* horse=mods::arg<daHorse_c*>(args,0);
    if (!saved.eligible || horse!=saved.horse || !allowed(horse) || !result ||
        *static_cast<int*>(result)!=0 || !horse->checkInputOnR() || horse->m_acch.ChkWallHit() ||
        responsiveness::angle_distance(horse->m_padStickAngleY,saved.shape)>0x6000) return;
    const float scale=multiplier(tuning::get().horseTurnSpeedPercent);
    horse->current.angle.y=responsiveness::extend_turn(saved.angle,horse->current.angle.y,horse->m_padStickAngleY,scale);
    horse->shape_angle.y=responsiveness::extend_turn(saved.shape,horse->shape_angle.y,horse->m_padStickAngleY,scale);
}
}
void reset() { g_calls.clear(); }
ModResult install() {
    auto r=mods::hook::add_pre<HorseMovement>(movement_pre);
    if (r==MOD_OK) r=mods::hook::add_post<HorseMovement>(movement_post);
    if (r==MOD_OK) r=mods::hook::add_pre<hooks::Shared2>(chase_pre);
    return r;
}
}
