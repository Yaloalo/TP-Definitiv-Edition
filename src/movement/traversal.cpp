#include "movement/traversal.hpp"

#include "combat/stamina.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "services.hpp"

namespace remaster::movement::traversal {
namespace {

constexpr float kTickSeconds = 1.0f / 30.0f;

// Procs in which Link is actively moving while climbing or hanging (idle hanging is free).
bool is_climb_move(u16 proc) {
    switch (proc) {
    case daAlink_c::PROC_CLIMB_MOVE_UPDOWN:
    case daAlink_c::PROC_CLIMB_MOVE_SIDE:
    case daAlink_c::PROC_LADDER_MOVE:
    case daAlink_c::PROC_HANG_MOVE:
    case daAlink_c::PROC_ROOF_HANG_FRONT_MOVE:
    case daAlink_c::PROC_ROOF_HANG_SIDE_MOVE:
    case daAlink_c::PROC_WOLF_ROPE_MOVE:
        return true;
    default:
        return false;
    }
}

bool is_swim_dash(daAlink_c* link) {
    if (link->checkWolf()) {
        return link->mProcID == daAlink_c::PROC_WOLF_SWIM_MOVE && link->checkWolfSwimDashAnime();
    }
    return link->mProcID == daAlink_c::PROC_SWIM_MOVE && link->checkSwimDashMode();
}

}  // namespace

void tick(daAlink_c* link) {
    const tuning::Values& values = tuning::get();
    if (is_climb_move(link->mProcID)) {
        combat::stamina::drain(static_cast<float>(values.climbCostPerSecond) * kTickSeconds, "climb");
    } else if (is_swim_dash(link)) {
        combat::stamina::drain(static_cast<float>(values.swimDashCostPerSecond) * kTickSeconds, "swim dash");
    }
}

}  // namespace remaster::movement::traversal
