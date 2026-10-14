#include "progression/growth.hpp"
#include "progression/rewards.hpp"
#include "core/player.hpp"
#include "services.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_meter2.h"
namespace remaster::progression::growth {
bool ensure() {
    if (!is_active() || !ready() || dComIfGs_getMaxLife()<15) return false;
    if (!model().growthReady) { model().migrate_growth(dComIfGs_getMaxLife()); persist(); }
    return true;
}
void apply_health() {
    if (!ensure()) return;
    dComIfGs_setMaxLife(model().native_max_life());
    // Raising maximum health is not a free heal. Respec cannot refill health.
    dComIfGs_setLife(std::min<int>(dComIfGs_getLife(),dComIfGs_getMaxLifeGauge()));
}
bool allocate(bool stamina) {
    if (!ensure() || !model().allocate(stamina)) return false;
    apply_health(); persist(); return true;
}
void tick() {
    if (!player::gameplay_running() || !ensure() || dComIfGp_getItemMaxLifeCount()) return;
    apply_health();
}
namespace {
DEFINE_HOOK(&dMeter2_c::moveLife, MaximumLifeUpdate);
HookAction maximum_pre(ModContext*,void*,void*,void*) {
    const int pending=dComIfGp_getItemMaxLifeCount();
    if (pending>0 && ensure()) {
        // Fallback for native sources without an ItemService check name. Their
        // original world/quest flags still enforce one-time collection.
        dComIfGp_clearItemMaxLifeCount(); model().add_growth(pending); apply_health(); persist();
    }
    return HOOK_CONTINUE;
}
void reward_given(ModContext*,const ItemGiveInfo* info,void*) {
    if (!info || !info->check_name || !*info->check_name || !ensure() ||
        (info->item!=dItemNo_KAKERA_HEART_e && info->item!=dItemNo_UTAWA_HEART_e)) return;
    // getItemFunc has already run, retaining item-first and boss-container
    // stage-life flags. Replace only its pending maximum-health increment.
    dComIfGp_clearItemMaxLifeCount();
    if (model().claim(info->check_name,0xff)) {
        model().add_growth(info->item==dItemNo_UTAWA_HEART_e?5:1); persist();
    }
    apply_health();
}
}
ModResult install() {
    auto r=svc_item->observe_gives(mod_ctx,reward_given,nullptr,nullptr);
    if (r==MOD_OK) r=mods::hook::add_pre<MaximumLifeUpdate>(maximum_pre);
    return r;
}
}
