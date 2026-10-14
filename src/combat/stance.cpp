#include "combat/stance.hpp"
#include "combat/stance_model.hpp"
#include "combat/techniques.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "SSystem/SComponent/c_counter.h"
#include <map>
#include <vector>

namespace remaster::combat::stance {
namespace {
DEFINE_HOOK(&daE_OC_c::_delete, DeleteBokoblin);
// Private E_OC_ACTION values from the pinned native implementation.
constexpr int kAttack = 4, kDamage = 5, kDown = 6, kLastOrdinary = 8;
struct Entry {
    daE_OC_c* actor = nullptr;
    State state;
    uint64_t lastParry = 0, attackingBash = 0;
    uint32_t bashFrame = 0;
};
std::map<fpc_ProcID, Entry> g_states;
struct Contact {
    daE_OC_c* actor = nullptr;
    uint64_t serial = 0;
    float impact = 0;
    bool bash = false, attacking = false;
};
std::vector<Contact> g_contacts;

bool active() {
    return is_active() && tuning::get().hiddenStance;
}
bool eligible(daE_OC_c* actor) {
    return active() && player::input_allowed() && actor && actor->mpMorf &&
        actor->health > 1 && actor->mActionMode >= 0 &&
        actor->mActionMode <= kLastOrdinary && actor->mActionMode != kDown &&
        !actor->checkCutDownHitFlg();
}
State& state_for(daE_OC_c* actor) {
    auto& entry = g_states[fopAcM_GetID(actor)];
    if (entry.actor != actor) entry = {actor, {}};
    return entry.state;
}
float impact_for(daAlink_c* link) {
    using techniques::Attack;
    switch (techniques::current_attack(link)) {
    case Attack::Heavy: return 35;
    case Attack::GuardCounter: return 40;
    default: break;
    }
    switch (link->getCutType()) {
    case daPy_py_c::CUT_TYPE_DOWN: return 0; // native execution owns the outcome
    case daPy_py_c::CUT_TYPE_GUARD_ATTACK: return 10; // ordinary bash, not a timed parry
    case daPy_py_c::CUT_TYPE_HEAD_JUMP: return 40;
    case daPy_py_c::CUT_TYPE_TWIRL: return 28;
    case daPy_py_c::CUT_TYPE_MORTAL_DRAW_A:
    case daPy_py_c::CUT_TYPE_MORTAL_DRAW_B: return 35;
    case daPy_py_c::CUT_TYPE_LARGE_JUMP_INIT: return 15;
    case daPy_py_c::CUT_TYPE_LARGE_JUMP:
    case daPy_py_c::CUT_TYPE_LARGE_JUMP_FINISH: return 40;
    case daPy_py_c::CUT_TYPE_JUMP: return 25;
    case daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT:
    case daPy_py_c::CUT_TYPE_LARGE_TURN_RIGHT: return 25;
    case daPy_py_c::CUT_TYPE_TURN_LEFT:
    case daPy_py_c::CUT_TYPE_TURN_RIGHT: return 18;
    default: return 10;
    }
}
float resistance_for(const daE_OC_c* actor) {
    // Native ordinary Bokoblins have 40 HP; sturdy variants have 220. A heavy
    // sword finisher deals 30 before sword modifiers. The ordinary variant must
    // be breakable before health damage kills it; do not inflate its HP.
    const float scale = actor->field_0x560 <= 40 ? 0.6f : 1.0f;
    return static_cast<float>(tuning::get().bokoblinStance) * scale;
}
void break_stance(daE_OC_c* actor) {
    actor->setActionMode(kDown, 0);
    actor->offTgSph();
    // Native executeBigDamage owns fall, down position, Ending Blow eligibility,
    // get-up, damage, death, switches and reward flow. Never synthesize a kill.
}
HookAction before_damage(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_OC_c*>(args, 0);
    g_contacts.push_back({});
    if (!eligible(actor) || actor->field_0x6cc != 0) return HOOK_CONTINUE;
    auto* link = daAlink_getAlinkActorClass();
    if (!link || link->checkWolf()) return HOOK_CONTINUE;
    dCcD_GObjInf* collider = nullptr;
    fopAc_ac_c* owner = nullptr;
    // Mirror native selection: the second hit sphere takes priority. Account
    // for the accepted action once, rather than once per touching hurt sphere.
    for (int i = 0; i < 2; ++i) if (actor->mSphs_cc[i].ChkTgHit()) {
        collider = static_cast<dCcD_GObjInf*>(actor->mSphs_cc[i].GetTgHitObj());
        owner = actor->mSphs_cc[i].GetTgHitAc();
    }
    if (owner != link || !collider ||
        !collider->ChkAtType(AT_TYPE_NORMAL_SWORD | AT_TYPE_MASTER_SWORD | AT_TYPE_SHIELD_ATTACK))
        return HOOK_CONTINUE;
    g_contacts.back() = {actor, techniques::attack_serial(), impact_for(link),
        collider->ChkAtType(AT_TYPE_SHIELD_ATTACK) != 0, actor->mActionMode == kAttack};
    return HOOK_CONTINUE;
}
void after_damage(ModContext*, void* args, void*, void*) {
    if (g_contacts.empty()) return;
    const auto hit = g_contacts.back(); g_contacts.pop_back();
    auto* actor = mods::arg<daE_OC_c*>(args, 0);
    if (!active() || hit.actor != actor || hit.impact <= 0 || actor->health <= 1 ||
        actor->field_0x6cc <= 0 || (actor->mActionMode != kDamage && actor->mActionMode != kDown)) return;
    const bool broken = state_for(actor).contact(hit.serial, hit.impact, resistance_for(actor));
    auto& entry = g_states[fopAcM_GetID(actor)];
    entry.attackingBash = 0;
    if (hit.bash && hit.attacking) {
        // Native actor execution order can process this outgoing bash before
        // Link's incoming guard contact. Keep proof only for this simulation frame.
        entry.attackingBash = hit.serial;
        entry.bashFrame = g_Counter.mCounter0;
    }
    if (broken) break_stance(actor);
    else if (hit.bash && tuning::get().combatTechniques && entry.lastParry != hit.serial) {
        // A plain bash still has recoil, but the Helm Splitter opening is the
        // reward for the actual incoming timed parry on this specific target.
        actor->setActionMode(kDamage, 2);
        actor->offHeadLockFlg();
    }
    else if (actor->mActionMode == kDown) {
        // Native sword finishers normally force knockdown. Stance owns that
        // transition on this adapter, while lethal/item/scripted hits stay native.
        actor->setActionMode(kDamage, 2);
    }
}
HookAction deleted(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_OC_c*>(args, 0);
    if (actor) g_states.erase(fopAcM_GetID(actor));
    return HOOK_CONTINUE;
}
void loaded(ModContext*, uint32_t, void*) { reset(); }
}
bool owns_bokoblin(fopAc_ac_c* actor) {
    return active() && actor && fopAcM_GetName(actor) == fpcNm_E_OC_e;
}
bool parry(fopAc_ac_c* attacker, daAlink_c* link) {
    if (!tuning::get().combatTechniques || !owns_bokoblin(attacker) || !link || link->checkWolf()) return false;
    auto* actor = static_cast<daE_OC_c*>(attacker);
    if (!player::input_allowed() || !actor->mpMorf || actor->health <= 1 || actor->checkCutDownHitFlg()) return false;
    state_for(actor); // Validate actor generation before reading its contact history.
    auto& entry = g_states[fopAcM_GetID(actor)];
    const auto serial = techniques::attack_serial();
    if (!serial || entry.lastParry == serial) return false;
    const bool bashFirst = entry.attackingBash == serial && entry.bashFrame == g_Counter.mCounter0 &&
        ((actor->mActionMode == kDamage && (actor->mOcState == 2 || actor->mOcState == 5)) || actor->mActionMode == kDown);
    if (actor->mActionMode != kAttack && !bashFirst) return false;
    entry.lastParry = serial;
    // A same-frame bash may already have broken stance. Accept the incoming
    // parry without cancelling the native knockdown or starting another break.
    if (bashFirst && actor->mActionMode == kDown) return true;
    const bool broken = state_for(actor).contact(techniques::attack_serial(), 40, resistance_for(actor));
    actor->mAtInfo.mHitDirection.y = fopAcM_searchPlayerAngleY(actor);
    if (broken) break_stance(actor);
    else {
        actor->setActionMode(kDamage, 3);
        actor->onHeadLockFlg(); // Native Helm Splitter targeting/learned gate.
    }
    return true;
}
void tick() {
    if (!active()) { g_states.clear(); return; }
    if (!player::input_allowed()) return;
    for (auto it = g_states.begin(); it != g_states.end();) {
        if (fopAcM_SearchByID(it->first) != it->second.actor) it = g_states.erase(it);
        else { it->second.state.tick(); ++it; }
    }
}
void reset() { g_states.clear(); g_contacts.clear(); }
ModResult install() {
    auto r = mods::hook::add_pre<hooks::BokoblinDamage>(before_damage);
    if (r == MOD_OK) r = mods::hook::add_post<hooks::BokoblinDamage>(after_damage);
    if (r == MOD_OK) r = mods::hook::add_pre<DeleteBokoblin>(deleted);
    if (r == MOD_OK) r = svc_save->observe_saves(mod_ctx, loaded, loaded, nullptr, nullptr, nullptr);
    return r;
}
}
