#include "economy/merchants.hpp"
#include "economy/merchant_model.hpp"
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "progression/rewards.hpp"
#include "resources/recharge_model.hpp"
#include "services.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_shop_system.h"
#include "d/actor/d_a_shop_item.h"
#include "d/actor/d_a_alink.h"
#include <set>
#include <cstring>
#include <fmt/format.h>
namespace remaster::economy::merchants {
namespace {
DEFINE_HOOK(&dShopSystem_c::isFlag, ShopFlag);
std::set<std::string> g_pending;
std::array<MessageId, 4> g_messages{};
UiElementHandle g_status = 0;
bool owns_reward(const Relic& r) {
    if (r.reward == dItemNo_HYLIA_SHIELD_e) return dComIfGs_isCollectShield(COLLECT_HYLIAN_SHIELD);
    if (r.reward == dItemNo_HAWK_EYE_e) return dComIfGs_getItem(SLOT_5, false) == dItemNo_HAWK_EYE_e;
    if (r.reward == dItemNo_ARMOR_e) return dComIfGs_isCollectClothes(2);
    return dComIfGs_isItemFirstBit(dItemNo_BOMB_BAG_LV2_e);
}
bool near_merchant(const Relic& r) {
    auto* link = daAlink_getAlinkActorClass();
    return link && std::strcmp(dComIfGp_getStartStageName(), r.stage) == 0 && fopAcM_GetRoomNo(link) == r.room;
}
bool trade_available(const Relic& r) {
    auto* link = daAlink_getAlinkActorClass();
    return enabled() && progression::ready() && link && !dComIfGp_event_runCheck() && link->mNormalSpeed == 0 &&
        can_redeem(progression::model(), r, dComIfGp_getStartStageName(), fopAcM_GetRoomNo(link), owns_reward(r), g_pending.contains(r.trade));
}
bool queue(const char* name, uint8_t item) {
    if (g_pending.contains(name) || progression::model().claimed.contains(name)) return false;
    if (svc_item->give_item(mod_ctx, name, item, 0) != MOD_OK) return false;
    g_pending.insert(name); return true;
}
void trade_pressed(ModContext*, void* data) {
    auto& relic = *static_cast<const Relic*>(data);
    if (trade_available(relic)) queue(relic.trade, relic.reward);
}
bool trade_disabled(ModContext*, void* data) { return !trade_available(*static_cast<const Relic*>(data)); }
HookAction shop_flag_pre(ModContext*, void* args, void* result, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    const int index = mods::arg<int>(args, 1);
    // Item flags 0..6 drive native browsing/purchase selection. Internal sequence flags >=7 stay native.
    if (index < 0 || index >= dShopSystem_c::ITEM_MAX_e) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = TRUE; return HOOK_SKIP_ORIGINAL;
}
ModResult register_relic_message(size_t i) {
    const auto& r = kRelics[i];
    const std::string text = fmt::format("You found the {}!\nTake it to {} for a permanent upgrade.", r.name, r.room == 1 ? "Barnes" : "Malo");
    std::array<MessageVariantData, 6> variants{};
    constexpr uint8_t langs[] = {0, 1, 2, 3, 4, 6};
    for (size_t n = 0; n < variants.size(); ++n) {
        auto& v = variants[n]; v.language = langs[n];
        v.entry.bytes[9] = MESSAGE_BOX_ITEM_GET; v.entry.bytes[10] = MESSAGE_DRAW_TYPED;
        v.entry.bytes[11] = MESSAGE_POSITION_MIDDLE; v.entry.bytes[12] = 255; v.entry.bytes[13] = 1; v.entry.bytes[18] = 4;
        v.text = reinterpret_cast<const uint8_t*>(text.c_str()); v.text_size = text.size() + 1;
    }
    MessageHandle handle = 0;
    return svc_message->register_message(mod_ctx, 0, variants.data(), variants.size(), &g_messages[i], &handle);
}
}
bool enabled() { return is_active() && tuning::get().removeCurrency && tuning::get().merchantRewards; }
bool custom_item(uint8_t item) { return relic_for_item(item) != nullptr; }
bool resolve(const ItemCheckInfo* info, ItemCheckResolution* out) {
    if (!enabled() || !info || !info->name) return false;
    if (const auto* relic = relic_for_check(info->name)) {
        *out = {relic->id, relic->display, true}; return true;
    }
    return false;
}
uint32_t message(daAlink_c* link, uint32_t id) {
    if (!is_active() || !link || link->mProcID != daAlink_c::PROC_GET_ITEM) return id;
    for (size_t i = 0; i < kRelics.size(); ++i)
        if (link->mProcVar2.field_0x300c == kRelics[i].id && id == uint32_t(kRelics[i].id + 0x65)) return g_messages[i];
    return id;
}
void on_give(const ItemGiveInfo* info) {
    if (!is_active() || !progression::ready() || !info) return;
    if (const auto* relic = relic_for_item(info->item)) {
        if (progression::model().claim(info->check_name, relic->id)) progression::persist();
    }
    if (info->check_name && (std::strncmp(info->check_name, "trade:", 6) == 0 || std::strncmp(info->check_name, "quest:", 6) == 0)) {
        // The host's item functions are intentionally empty for several equipment IDs; set native permanent ownership too.
        if (info->item == dItemNo_HYLIA_SHIELD_e) dComIfGs_setCollectShield(COLLECT_HYLIAN_SHIELD);
        if (info->item == dItemNo_ARMOR_e) dComIfGs_setCollectClothes(2);
        if (info->item == dItemNo_BOMB_BAG_LV2_e) {
            dComIfGs_setBombMax(dItemNo_NORMAL_BOMB_e, std::max<int>(60, dComIfGs_getBombMax(dItemNo_NORMAL_BOMB_e)));
            dComIfGs_setBombMax(dItemNo_WATER_BOMB_e, std::max<int>(30, dComIfGs_getBombMax(dItemNo_WATER_BOMB_e)));
            dComIfGs_setBombMax(dItemNo_POKE_BOMB_e, std::max<int>(20, dComIfGs_getBombMax(dItemNo_POKE_BOMB_e)));
        }
        if (info->item == dItemNo_PACHINKO_e) dComIfGs_onEventBit(0x4902); // native Sera acquisition milestone
        progression::model().claim(info->check_name, 0xff); progression::persist();
        g_pending.erase(info->check_name);
    }
    if (info->check_name && std::strcmp(info->check_name, "sera_reward") == 0) {
        progression::model().claim("quest:sera_helped", 0xff); progression::persist();
    }
}
void reset() { g_pending.clear(); }
void tick() {
    if (!enabled() || !progression::ready()) return;
    if (player::input_allowed()) {
        auto& model = progression::model();
        const bool catHelped = model.claimed.contains("quest:sera_helped") ||
            (dComIfGs_isEventBit(0x1408) && dComIfGs_isItemFirstBit(dItemNo_HALF_MILK_BOTTLE_e));
        if (catHelped && dComIfGs_getItem(SLOT_23, false) != dItemNo_PACHINKO_e) queue("quest:sera_slingshot", dItemNo_PACHINKO_e);
        bool bag = false, water = false, bombling = false;
        for (int i = 0; i < 3; ++i) {
            const u8 type = dComIfGs_getItem(SLOT_15 + i, false);
            bag |= resources::bomb_type(type) || type == dItemNo_BOMB_BAG_LV1_e;
            water |= type == dItemNo_WATER_BOMB_e; bombling |= type == dItemNo_POKE_BOMB_e;
        }
        if (dComIfGs_isCollectCrystal(1) && !bag) queue("quest:fyrus_bomb_bag", dItemNo_BOMB_BAG_LV1_e);
        if (bag && dComIfGs_isCollectClothes(1) && !water && !dComIfGs_isItemFirstBit(dItemNo_WATER_BOMB_e)) queue("quest:zora_water_bombs", dItemNo_WATER_BOMB_e);
        if (bag && dComIfGs_isCollectCrystal(2) && !bombling && !dComIfGs_isItemFirstBit(dItemNo_POKE_BOMB_e)) queue("quest:lakebed_bomblings", dItemNo_POKE_BOMB_e);
    }
    if (g_status) {
        std::string text = "Essential gear comes from quests: help Sera's cat for the Slingshot; defeat Fyrus for a bomb bag; Zora Armor unlocks water bombs; Lakebed victory unlocks Bomblings.\nVisit the merchant, then use the matching relic below. Each trade is permanent and can be completed once.\n";
        for (const auto& relic : kRelics) text += fmt::format("{}: {}\n", relic.name, redeemed(progression::model(), relic) ? "upgrade complete" : owns_relic(progression::model(), relic) ? "ready to trade" : "find through exploration");
        svc_ui->elem_set_text(mod_ctx, g_status, text.c_str());
    }
}
ModResult build_controls(UiElementHandle pane) {
    auto r = svc_ui->pane_add_section(mod_ctx, pane, "Quest rewards and relic upgrades");
    if (r == MOD_OK) r = svc_ui->pane_add_text(mod_ctx, pane, "Select Definitive Overhaul to view your quest and relic rewards.", &g_status);
    for (const auto& relic : kRelics) {
        UiControlDesc control = UI_CONTROL_DESC_INIT;
        control.label = relic.label; control.on_pressed = trade_pressed; control.is_disabled = trade_disabled; control.user_data = const_cast<Relic*>(&relic);
        if (r == MOD_OK) r = svc_ui->pane_add_control(mod_ctx, pane, &control, nullptr);
    }
    return r;
}
ModResult install() {
    auto r = mods::hook::add_pre<ShopFlag>(shop_flag_pre);
    for (size_t i = 0; i < kRelics.size(); ++i) if (r == MOD_OK) r = register_relic_message(i);
    return r;
}
}
