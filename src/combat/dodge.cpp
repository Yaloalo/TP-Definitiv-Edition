#include "combat/dodge.hpp"

#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "services.hpp"

namespace remaster::combat::dodge {
namespace {

// setCollision registers Link's hurt cylinders with the collision manager each execute; the
// global collision pass reads their Tg bits later in the frame, so clearing them in a post-hook
// prevents hits from being recorded.
DEFINE_HOOK(&daAlink_c::setCollision, LinkSetCollision);

u16 g_lastProc = 0xFFFF;
int g_procTicks = 0;

bool is_dodge(u16 proc) {
    switch (proc) {
    case daAlink_c::PROC_FRONT_ROLL:
    case daAlink_c::PROC_SIDE_ROLL:
    case daAlink_c::PROC_SIDESTEP:
    case daAlink_c::PROC_BACK_JUMP:
        return true;
    default:
        return false;
    }
}

bool in_window(const daAlink_c* link) {
    if (!is_active() || link->checkWolf() || !is_dodge(link->mProcID)) {
        return false;
    }
    // The proc may have started during this execute, before tick() has seen it.
    const int ticks = link->mProcID == g_lastProc ? g_procTicks : 0;
    return ticks < tuning::get().dodgeInvincibilityTicks;
}

void disable_hurt_cylinders(daAlink_c* link) {
    for (dCcD_Cyl& cyl : link->mTgCyls) {
        cyl.OffTgSetBit();
        cyl.ResetTgHit();
    }
}

void on_set_collision_post(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link != nullptr && in_window(link)) {
        disable_hurt_cylinders(link);
    }
}

}  // namespace

void before_damage_check(daAlink_c* link) {
    if (link != nullptr && (link->mProcID == g_lastProc && in_window(link))) {
        for (dCcD_Cyl& cyl : link->mTgCyls) {
            cyl.ResetTgHit();
        }
    }
}

ModResult install() {
    return mods::hook::add_post<LinkSetCollision>(on_set_collision_post);
}

void tick(daAlink_c* link) {
    if (link->mProcID != g_lastProc) {
        g_lastProc = link->mProcID;
        g_procTicks = 0;
    } else if (g_procTicks < 0x7FFF) {
        ++g_procTicks;
    }
}

}  // namespace remaster::combat::dodge
