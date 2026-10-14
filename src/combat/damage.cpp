#include "combat/damage.hpp"

#include <algorithm>
#include <cmath>
#include <vector>
#include "core/hooks.hpp"

#include "combat/dodge.hpp"
#include "combat/ward.hpp"
#include "combat/stamina.hpp"
#include "healing/flask.hpp"
#include "resources/ammo.hpp"
#include "core/difficulty.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "services.hpp"
#include "progression/rewards.hpp"

namespace remaster::combat::damage {
namespace {

// Every hit on Link ends in setDamagePoint (d_a_alink_damage.inc:178): enemy attacks pass
// mCcStts.GetDmg() (:714), floor/hazard damage passes small constants. amount <= 0 is a heal.
// Args: 0 = this, 1 = int amount, 2 = BOOL zora magnification, 3 = BOOL set invincibility timer.
DEFINE_HOOK(&daAlink_c::setDamagePoint, LinkSetDamagePoint);
DEFINE_HOOK(&daAlink_c::damageMagnification, DamageMagnification);

// checkDamageAction (d_a_alink_damage.inc:423) consumes collider hits once per execute, calls
// setDamagePoint and enters the reaction proc. Comparing mDamageTimer around it tells whether a
// new hit started the invincibility timer, and mProcID afterwards tells whether it was a knockdown.
// Pattern ported from Dawnlight (reference-mods/dawnlight/src/player_hard_mode.cpp).
DEFINE_HOOK(&daAlink_c::checkDamageAction, LinkCheckDamageAction);

struct TimerSnapshot {
    daAlink_c* link = nullptr;
    s16 timer = 0;
};
TimerSnapshot g_snapshot;
struct DamageScope { daAlink_c* link=nullptr;int expected=0;bool correctArmor=false; };
std::vector<DamageScope> g_scopes;
void damage_end(ModContext*,void*,void*,void*) {if(!g_scopes.empty())g_scopes.pop_back();}
HookAction armor_life_pre(ModContext*,void* args,void*,void*) {
    if(!is_active()||g_scopes.empty()||!g_scopes.back().correctArmor)return HOOK_CONTINUE;
    auto& life=mods::arg_ref<f32>(args,0);
    if(life<0)life=-float(g_scopes.back().expected);
    return HOOK_CONTINUE;
}
// Link instance currently recovering from a knockdown; keeps its full timer until it runs out.
daAlink_c* g_knockdownOwner = nullptr;

bool is_knockdown(u16 proc) {
    switch (proc) {
    case daAlink_c::PROC_LARGE_DAMAGE:
    case daAlink_c::PROC_LARGE_DAMAGE_UP:
    case daAlink_c::PROC_LARGE_DAMAGE_WALL:
    case daAlink_c::PROC_LAND_DAMAGE:
    case daAlink_c::PROC_WOLF_LARGE_DAMAGE_UP:
    case daAlink_c::PROC_WOLF_LAND_DAMAGE:
        return true;
    default:
        return false;
    }
}

HookAction magnification_pre(ModContext*, void* args, void* result, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const BOOL zora = mods::arg<BOOL>(args, 1);
    const int kind = mods::arg<int>(args, 2);
    float base = kind == 0 && link->checkNoResetFlg3(daAlink_c::FLG3_UNK_40000000) &&
        !link->checkEndResetFlg2(daAlink_c::ERFLG2_UNK_40) ? 1.5f : 1.0f;
    if (link->checkWolf() && !link->checkCargoCarry() && kind == 0) base *= 2.0f;
    else if (link->checkZoraWearAbility() && zora) base *= 10.0f;
    *static_cast<float*>(result) = base;
    return HOOK_SKIP_ORIGINAL;
}

HookAction on_set_damage_pre(ModContext*, void* args, void*, void*) {
    auto* actor=mods::arg<daAlink_c*>(args,0);
    g_scopes.push_back({actor,0,false});
    if (!is_active()) {
        return HOOK_CONTINUE;
    }
    int& amount = mods::arg_ref<int>(args, 1);
    if (amount <= 0) {
        return HOOK_CONTINUE;
    }

    const tuning::Values& values = tuning::get();
    const float scale = static_cast<float>(values.enemyDamagePercent) / 100.0f * difficulty::multiplier();
    const int original = amount;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    healing::damage_interrupted(link);
    resources::ammo::damage_received();
    ward::damage_received();
    const float protection=ward::absorb(link);
    // Protection never reduces a hit to zero and costs one finite ward charge.
    amount = std::max(1, static_cast<int>(std::ceil(static_cast<float>(amount) * scale * protection)));
    if(link && link->checkMagicArmorWearAbility()) {
        // Retain native reactions/flags, but own final armor damage even if a
        // launcher/other mod selected the native double-defense preset.
        const auto zora=mods::arg<BOOL>(args,2);const int kind=mods::arg<int>(args,4);
        g_scopes.back().expected=std::max(1,int(std::ceil(amount*link->damageMagnification(zora,kind))));
        g_scopes.back().correctArmor=true;
    }

    if (values.debugLog) {
        mods::log::info("damage {} -> {} (x{:.2f}, tier '{}')", original, amount, scale, difficulty::tier_name());
    }
    return HOOK_CONTINUE;
}

HookAction on_damage_action_pre(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    dodge::before_damage_check(link);
    g_snapshot = {link, link != nullptr ? link->mDamageTimer : s16(0)};
    if (link != nullptr && link->mDamageTimer == 0 && g_knockdownOwner == link) {
        g_knockdownOwner = nullptr;
    }
    return HOOK_CONTINUE;
}

void on_damage_action_post(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const TimerSnapshot snapshot = g_snapshot;
    g_snapshot = {};
    if (!is_active() || link == nullptr || snapshot.link != link || link->mDamageTimer <= snapshot.timer) {
        return;
    }
    if (is_knockdown(link->mProcID)) {
        g_knockdownOwner = link;
        return;
    }
    if (g_knockdownOwner == link) {
        return;
    }
    // mDamageTimer counts down once per 30 Hz tick; while non-zero setCollision disables Link's
    // hurt cylinders (d_a_alink.cpp:6833), so shortening it shortens the invincibility.
    const int64_t percent = tuning::get().postHitInvincibilityPercent;
    link->mDamageTimer = static_cast<s16>(std::max<int64_t>(0, link->mDamageTimer * percent / 100));
}

}  // namespace

ModResult install() {
    ModResult result = mods::hook::add_pre<DamageMagnification>(magnification_pre);
    if (result == MOD_OK) result = mods::hook::add_pre<LinkSetDamagePoint>(on_set_damage_pre);
    if(result==MOD_OK)result=mods::hook::add_post<LinkSetDamagePoint>(damage_end);
    if(result==MOD_OK)result=mods::hook::add_pre<hooks::LifeCount>(armor_life_pre);
    if (result == MOD_OK) {
        result = mods::hook::add_pre<LinkCheckDamageAction>(on_damage_action_pre);
    }
    if (result == MOD_OK) {
        result = mods::hook::add_post<LinkCheckDamageAction>(on_damage_action_post);
    }
    return result;
}

}  // namespace remaster::combat::damage
