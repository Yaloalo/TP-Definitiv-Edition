#include "healing/snowpeak_hearts.hpp"
#include "services.hpp"
#include "d/actor/d_a_npc_ykw.h"
#include "d/actor/d_a_obj_item.h"
#include "d/d_com_inf_game.h"
#include "d/d_item.h"
#include <set>
#include <vector>
#include <cstring>
namespace remaster::healing::snowpeak {
namespace {
DEFINE_HOOK(&daNpc_ykW_c::cutGoIntoBossRoom, EndingCut);
DEFINE_HOOK(&daNpc_ykW_c::cutHug, HugCut);
DEFINE_HOOK(&execItemGet, GrantItem);
std::vector<bool> g_scope;
unsigned g_sources = 0;
std::set<fpc_ProcID> g_hearts;
std::vector<bool> g_grants;
unsigned g_grantDepth = 0;
bool snowpeak_stage() {
    const char* stage = dComIfGp_getStartStageName();
    return stage && std::strncmp(stage, "D_MN11", 6) == 0;
}
HookAction enter_cut(void* args, int heartStep) {
    const bool allow = is_active() && snowpeak_stage() && dComIfGs_isStageBossEnemy();
    const auto staff = mods::arg<int>(args, 1);
    int* param = allow ? dComIfGp_evmng_getMyIntegerP(staff, "prm") : nullptr;
    // Both native ending cuts contain the couple's heart bursts. Limit this to their heart-emission steps.
    const bool source = allow && param && *param == heartStep;
    g_scope.push_back(source); if (source) ++g_sources;
    return HOOK_CONTINUE;
}
HookAction ending_pre(ModContext*, void* args, void*, void*) { return enter_cut(args, 13); }
HookAction hug_pre(ModContext*, void* args, void*, void*) { return enter_cut(args, 1); }
void cut_post(ModContext*, void*, void*, void*) {
    if (!g_scope.empty()) { if (g_scope.back()) --g_sources; g_scope.pop_back(); }
}
HookAction grant_pre(ModContext*, void* args, void*, void*) {
    const auto* giver = mods::arg<fopAc_ac_c*>(args, 2);
    const bool allow = is_active() && giver && snowpeak_stage() &&
        mods::arg<u8>(args, 0) == 0 && g_hearts.contains(fopAcM_GetID(giver));
    g_grants.push_back(allow); if (allow) ++g_grantDepth;
    return HOOK_CONTINUE;
}
void grant_post(ModContext*, void* args, void*, void*) {
    if (!g_grants.empty()) {
        if (g_grants.back()) { --g_grantDepth; if (auto* giver = mods::arg<fopAc_ac_c*>(args, 2)) g_hearts.erase(fopAcM_GetID(giver)); }
        g_grants.pop_back();
    }
}
}
bool allows_spawn() { return is_active() && g_sources && snowpeak_stage(); }
bool allows_actor(daItem_c* actor) {
    if (!is_active() || !actor || !snowpeak_stage() || daItem_prm::getItemNo(actor) != 0) return false;
    const auto id = fopAcM_GetID(actor);
    if (allows_spawn()) { g_hearts.insert(id); return true; }
    return g_hearts.contains(id);
}
bool allows_grant() { return is_active() && g_grantDepth > 0; }
void reset() { g_scope.clear(); g_sources = g_grantDepth = 0; g_hearts.clear(); g_grants.clear(); }
ModResult install() {
    auto r = mods::hook::add_pre<EndingCut>(ending_pre);
    if (r == MOD_OK) r = mods::hook::add_post<EndingCut>(cut_post);
    if (r == MOD_OK) r = mods::hook::add_pre<HugCut>(hug_pre);
    if (r == MOD_OK) r = mods::hook::add_post<HugCut>(cut_post);
    if (r == MOD_OK) r = mods::hook::add_pre<GrantItem>(grant_pre);
    if (r == MOD_OK) r = mods::hook::add_post<GrantItem>(grant_post);
    return r;
}
}
