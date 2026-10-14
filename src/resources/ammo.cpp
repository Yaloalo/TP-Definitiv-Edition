#include "resources/ammo.hpp"
#include "resources/recharge_model.hpp"
#include "core/player.hpp"
#include "core/hooks.hpp"
#include "core/tuning.hpp"
#include "services.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "d/d_item_data.h"
#include <array>
#include <algorithm>
namespace remaster::resources::ammo {
namespace {
DEFINE_HOOK(&dMeter2_c::moveBombNum, BombMeter);
DEFINE_HOOK(&dComIfGs_setEquipBottleItemEmpty, EmptyBaitBottle);
std::array<Recharge, 6> g_clock; // arrows, seeds, three bags, oil; bait bottles have separate clocks
std::array<Recharge, 4> g_bait;
std::array<u8, 3> g_beforeBags{};
daAlink_c* g_owner = nullptr;
int g_lastLife = -1;
bool g_retry = false;
uint32_t g_retrySlot = 255;
void clear_clocks() { for (auto& c : g_clock) c.hit(); for (auto& c : g_bait) c.hit(); }
bool gameplay() {
    bool visible = true;
    return player::gameplay_running() && svc_ui->is_any_document_visible(mod_ctx, &visible) == MOD_OK && !visible;
}
int count(int saved, int pending, int maximum) { return std::clamp(saved + pending, 0, maximum); }
void restore_bag(int index, u8 type) {
    dComIfGs_setItem(SLOT_15 + index, type); dComIfGp_setItem(SLOT_15 + index, type);
}
HookAction bomb_before(ModContext*, void*, void*, void*) {
    if (enabled()) for (int i = 0; i < 3; ++i) g_beforeBags[i] = dComIfGs_getItem(SLOT_15 + i, false);
    return HOOK_CONTINUE;
}
void bomb_after(ModContext*, void*, void*, void*) {
    if (!enabled()) return;
    // Vanilla empties the bag's TYPE when its last bomb is used. Retain it for recharging and saving.
    for (int i = 0; i < 3; ++i)
        if (bomb_type(g_beforeBags[i]) && dComIfGs_getItem(SLOT_15 + i, false) == dItemNo_BOMB_BAG_LV1_e)
            restore_bag(i, g_beforeBags[i]);
}
HookAction empty_bait_pre(ModContext*, void* args, void*, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    const auto selected = mods::arg<u8>(args, 0);
    const int slot = dComIfGs_getSelectItemIndex(selected);
    if (slot < SLOT_11 || slot > SLOT_14 || dComIfGs_getItem(slot, false) != dItemNo_BEE_CHILD_e) return HOOK_CONTINUE;
    dComIfGs_setBottleNum(slot - SLOT_11, 0);
    return HOOK_SKIP_ORIGINAL;
}
void selection_post(ModContext*, void* args, void* result, void*) {
    if (!enabled() || !result) return;
    const auto selected = mods::arg<u8>(args, 1);
    const int slot = dComIfGs_getSelectItemIndex(selected);
    const auto item = dComIfGs_getItem(slot, false);
    if (slot >= SLOT_15 && slot <= SLOT_17 && bomb_type(item) &&
        int(dComIfGs_getBombNum(slot - SLOT_15)) + int(dComIfGp_getItemBombNumCount(slot - SLOT_15)) <= 0)
        *static_cast<int*>(result) = 0;
    if (slot >= SLOT_11 && slot <= SLOT_14 && item == dItemNo_BEE_CHILD_e && dComIfGs_getBottleNum(slot - SLOT_11) == 0)
        *static_cast<int*>(result) = 0;
}
}
bool enabled() { return is_active() && tuning::get().ammoRecharge; }
bool removes_pickup(int item) { return is_active() && tuning::get().removeConsumableDrops && ammo_pickup(item); }
void damage_received() { if (enabled()) clear_clocks(); }
void death() { if (enabled()) { g_retry = true; g_retrySlot = dComIfGs_getDataNum(); clear_clocks(); } }
void slot_changed(uint32_t slot, bool fresh) {
    if (fresh || slot != g_retrySlot) g_retry = false;
    reset();
}
void reset() { clear_clocks(); g_owner = nullptr; g_lastLife = -1; g_beforeBags.fill(255); }
void refill() {
    if (!enabled()) return;
    if (dComIfGs_getItem(SLOT_4, false) == dItemNo_BOW_e || dComIfGs_getItem(SLOT_4, false) == dItemNo_LIGHT_ARROW_e)
        dComIfGp_setItemArrowNumCount(std::max(0, int(dComIfGs_getArrowMax()) - int(dComIfGs_getArrowNum()) - int(dComIfGp_getItemArrowNumCount())));
    if (dComIfGs_getItem(SLOT_23, false) == dItemNo_PACHINKO_e)
        dComIfGp_setItemPachinkoNumCount(std::max(0, int(dComIfGs_getPachinkoMax()) - int(dComIfGs_getPachinkoNum()) - int(dComIfGp_getItemPachinkoNumCount())));
    for (int i = 0; i < 3; ++i) {
        u8 type = dComIfGs_getItem(SLOT_15 + i, false);
        if (type == dItemNo_BOMB_BAG_LV1_e) { type = dItemNo_NORMAL_BOMB_e; restore_bag(i, type); }
        if (bomb_type(type)) dComIfGp_setItemBombNumCount(i, std::max(0, int(dComIfGs_getBombMax(type)) - int(dComIfGs_getBombNum(i)) - int(dComIfGp_getItemBombNumCount(i))));
    }
    if (dComIfGs_getItem(SLOT_1, false) == dItemNo_KANTERA_e)
        dComIfGp_setItemOilCount(std::max(0, int(dComIfGs_getMaxOil()) - int(dComIfGs_getOil()) - int(dComIfGp_getItemOilCount())));
    for (int i = 0; i < 4; ++i) if (dComIfGs_getItem(SLOT_11 + i, false) == dItemNo_BEE_CHILD_e)
        dComIfGs_setBottleNum(i, dComIfGs_getBottleMax());
    clear_clocks();
}
void tick(daAlink_c* link) {
    if (!enabled() || !link) { reset(); return; }
    if (link != g_owner) { reset(); g_owner = link; }
    if (!gameplay() || link->mProcID == daAlink_c::PROC_DEAD || dComIfGs_getLife() == 0) return;
    int life = dComIfGs_getLife();
    if (g_lastLife >= 0 && life < g_lastLife) damage_received();
    g_lastLife = life;
    if (g_retry) { g_retry = false; refill(); }
    const auto& v = tuning::get();
    int gain;
    const u8 bow = dComIfGs_getItem(SLOT_4, false);
    if (bow == dItemNo_BOW_e || bow == dItemNo_LIGHT_ARROW_e) {
        const int max = dComIfGs_getArrowMax();
        gain = g_clock[0].tick(count(dComIfGs_getArrowNum(), dComIfGp_getItemArrowNumCount(), max), max, ticks_for_ms(v.arrowRechargeDelayMs), ticks_for_ms(v.arrowRechargeIntervalMs));
        if (gain) dComIfGp_setItemArrowNumCount(gain);
    } else g_clock[0].hit();
    if (dComIfGs_getItem(SLOT_23, false) == dItemNo_PACHINKO_e) {
        const int max = dComIfGs_getPachinkoMax();
        gain = g_clock[1].tick(count(dComIfGs_getPachinkoNum(), dComIfGp_getItemPachinkoNumCount(), max), max, ticks_for_ms(v.seedRechargeDelayMs), ticks_for_ms(v.seedRechargeIntervalMs));
        if (gain) dComIfGp_setItemPachinkoNumCount(gain);
    } else g_clock[1].hit();
    for (int i = 0; i < 3; ++i) {
        u8 type = dComIfGs_getItem(SLOT_15 + i, false);
        // Existing saves with an empty bag have lost their type in vanilla. Start it as regular bombs.
        if (type == dItemNo_BOMB_BAG_LV1_e) { type = dItemNo_NORMAL_BOMB_e; restore_bag(i, type); }
        if (!bomb_type(type)) { g_clock[2 + i].hit(); continue; }
        int64_t delay = v.bombRechargeDelayMs, interval = v.bombRechargeIntervalMs;
        if (type == dItemNo_WATER_BOMB_e) { delay = v.waterBombRechargeDelayMs; interval = v.waterBombRechargeIntervalMs; }
        if (type == dItemNo_POKE_BOMB_e) { delay = v.bomblingRechargeDelayMs; interval = v.bomblingRechargeIntervalMs; }
        const int max = dComIfGs_getBombMax(type);
        gain = g_clock[2 + i].tick(count(dComIfGs_getBombNum(i), dComIfGp_getItemBombNumCount(i), max), max, ticks_for_ms(delay), ticks_for_ms(interval));
        if (gain) dComIfGp_setItemBombNumCount(i, gain);
    }
    if (dComIfGs_getItem(SLOT_1, false) == dItemNo_KANTERA_e) {
        const int max = dComIfGs_getMaxOil();
        gain = g_clock[5].tick(count(dComIfGs_getOil(), dComIfGp_getItemOilCount(), max), max, ticks_for_ms(v.oilRechargeDelayMs), ticks_for_ms(v.oilRechargeIntervalMs), std::max(1, max / 100));
        if (gain) dComIfGp_setItemOilCount(gain); // 1% of the lantern per refill, clamped to its actual capacity.
    } else g_clock[5].hit();
    for (int i = 0; i < 4; ++i) {
        if (dComIfGs_getItem(SLOT_11 + i, false) != dItemNo_BEE_CHILD_e) { g_bait[i].hit(); continue; }
        gain = g_bait[i].tick(dComIfGs_getBottleNum(i), dComIfGs_getBottleMax(), ticks_for_ms(v.baitRechargeDelayMs), ticks_for_ms(v.baitRechargeIntervalMs));
        if (gain) dComIfGs_addBottleNum(i, gain);
    }
}
ModResult install() {
    auto r = mods::hook::add_pre<BombMeter>(bomb_before);
    if (r == MOD_OK) r = mods::hook::add_post<BombMeter>(bomb_after);
    if (r == MOD_OK) r = mods::hook::add_pre<EmptyBaitBottle>(empty_bait_pre);
    if (r == MOD_OK) r = mods::hook::add_post<hooks::ItemSelection>(selection_post);
    return r;
}
}
