#include "progression/rewards.hpp"
#include "core/tuning.hpp"
#include "healing/flask.hpp"
#include "healing/snowpeak_hearts.hpp"
#include "resources/ammo.hpp"
#include "economy/merchants.hpp"
#include "services.hpp"
#include "d/d_item.h"
#include "d/actor/d_a_alink.h"
#include "d/d_item_data.h"
#include <array>
#include <cstring>
#include <string_view>

namespace remaster::progression {
namespace {
Model g_model;
bool g_ready = false;
DEFINE_HOOK_SYMBOL("getItemFunc", void(u8), GetItem);
DEFINE_HOOK_SYMBOL("dusk::mods::item_check_message", u32(u16, u32), CheckMessage);
DEFINE_HOOK(&daAlink_c::checkEndMessage, ItemMessage);
MessageId g_shardMessage = 0, g_staminaMessage = 0;
// Hand-picked currency checks: five sets of four shards, spread through the adventure.
// Check coordinates come from the community location catalogue; no randomizer dependency.
constexpr std::array<std::string_view, 20> kFlaskChecks = {
    "chest:R_SP01:1", "chest:D_SB10:31", "chest:F_SP108:29", "chest:D_MN05:37",
    "chest:R_SP109:23", "chest:F_SP111:24", "chest:D_MN04:21", "chest:F_SP121:6",
    "chest:F_SP115:1", "chest:F_SP113:26", "chest:D_MN01:11", "chest:D_SB03:5",
    "chest:F_SP124:7", "chest:D_MN10:25", "chest:F_SP114:0", "chest:D_MN11:20",
    "chest:D_MN06:48", "chest:D_MN06:44", "chest:D_MN07:25", "chest:D_MN08:25",
};
bool rupee(uint8_t i) { return (i >= 1 && i <= 7) || i == dItemNo_LINKS_SAVINGS_e; }
bool resolve(ModContext*, const ItemCheckInfo* info, ItemCheckResolution* out, void*) {
    if (!is_active() || !info || !info->name) return false;
    if (economy::merchants::resolve(info, out)) return true;
    const std::string_view name(info->name);
    // Shop stock must preserve equipment/quest checks, and can never award farmable shards.
    if (!(tuning::get().removeCurrency && rupee(info->vanilla_item)) && !resources::ammo::removes_pickup(info->vanilla_item)) return false;
    if (name.starts_with("shop:") || name.ends_with(":255") || name.ends_with(":-1")) return false;
    const bool flask = tuning::get().flaskEnabled &&
        std::find(kFlaskChecks.begin(), kFlaskChecks.end(), name) != kFlaskChecks.end();
    *out = {flask ? kFlaskShard : kStaminaFragment, uint8_t(flask ? dItemNo_RED_BOTTLE_e : dItemNo_KAKERA_HEART_e), true};
    return true;
}
void give(ModContext*, const ItemGiveInfo* info, void*) {
    if (!is_active() || !g_ready || !info) return;
    if (info->item == kFlaskShard || info->item == kStaminaFragment) {
        const auto oldCapacity = g_model.capacity(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges);
        if (g_model.claim(info->check_name, info->item)) {
            auto newCapacity = g_model.capacity(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges);
            if (newCapacity > oldCapacity) g_model.charges = std::min(newCapacity, g_model.charges + newCapacity - oldCapacity);
            persist();
        }
    }
    if (tuning::get().flaskEnabled && info->check_name && std::string_view(info->check_name).starts_with("boss:")) healing::refill(true);
    economy::merchants::on_give(info);
    healing::normalize_bottles();
}
HookAction item_pre(ModContext*, void* args, void*, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    const uint8_t item = mods::arg<uint8_t>(args, 0);
    if (item == kFlaskShard || item == kStaminaFragment || economy::merchants::custom_item(item) || resources::ammo::removes_pickup(item) ||
        (tuning::get().removeCurrency && rupee(item)) ||
        (tuning::get().removeHeartDrops && (item == dItemNo_HEART_e || item == dItemNo_TRIPLE_HEART_e) && !healing::snowpeak::allows_grant()) ||
        (tuning::get().flaskEnabled && item == dItemNo_RECOVERY_FAILY_e))
        return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
}
void on_new(ModContext*, uint32_t slot, void*) {
    economy::merchants::reset();
    healing::save_slot_changed(slot, true);
    g_ready = is_active();
    if (!g_ready) return;
    g_model = {}; g_model.refill(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges);
    healing::reset(); persist();
}
void on_load(ModContext*, uint32_t slot, void*) { economy::merchants::reset(); healing::save_slot_changed(slot, false); load(); }
void check_message_post(ModContext*, void*, void* result, void*) {
    if (!is_active()) return;
    auto& id = *static_cast<u32*>(result);
    if (id == kFlaskShard + 0x65) id = g_shardMessage;
    else if (id == kStaminaFragment + 0x65) id = g_staminaMessage;
}
HookAction item_message(ModContext*, void* args, void*, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    auto& id = mods::arg_ref<u32>(args, 1);
    id = economy::merchants::message(mods::arg<daAlink_c*>(args, 0), id);
    if (id == kFlaskShard + 0x65) id = g_shardMessage;
    if (id == kStaminaFragment + 0x65) id = g_staminaMessage;
    return HOOK_CONTINUE;
}
ModResult register_text(const char* text, MessageId* id) {
    std::array<MessageVariantData, 6> variants{};
    const uint8_t languages[] = {0, 1, 2, 3, 4, 6};
    for (size_t i = 0; i < variants.size(); ++i) {
        auto& v = variants[i]; v.language = languages[i];
        v.entry.bytes[9] = MESSAGE_BOX_ITEM_GET;
        v.entry.bytes[10] = MESSAGE_DRAW_TYPED;
        v.entry.bytes[11] = MESSAGE_POSITION_MIDDLE;
        v.entry.bytes[12] = 255; v.entry.bytes[13] = 1; v.entry.bytes[18] = 4;
        v.text = reinterpret_cast<const uint8_t*>(text); v.text_size = std::strlen(text) + 1;
    }
    MessageHandle handle = 0;
    return svc_message->register_message(mod_ctx, 0, variants.data(), variants.size(), id, &handle);
}
}
Model& model() { return g_model; }
bool ready() { return g_ready; }
int stamina_bonus() { return is_active() && g_ready ? g_model.stamina_bonus() : 0; }
void persist() {
    if (!is_active() || !g_ready) return;
    const auto bytes = encode(g_model);
    if (svc_save->set_blob(mod_ctx, "progression-v1", bytes.data(), bytes.size()) != MOD_OK)
        mods::log::warn("Could not update Definitive Edition save progress");
}
void load() {
    g_ready = is_active(); if (!g_ready) return;
    size_t size = 0;
    g_model = {}; g_model.refill(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges);
    if (svc_save->get_blob(mod_ctx, "progression-v1", nullptr, &size) == MOD_OK && size <= SAVE_BLOB_BUDGET_BYTES) {
        std::vector<uint8_t> bytes(size);
        if (svc_save->get_blob(mod_ctx, "progression-v1", bytes.data(), &size) != MOD_OK || !decode(bytes, g_model)) {
            g_ready = false;
            mods::log::error("Invalid Definitive Edition progress blob; left untouched");
        }
    }
}
ModResult install() {
    ModResult result = svc_save->observe_saves(mod_ctx, on_new, on_load, nullptr, nullptr, nullptr);
    if (result == MOD_OK) result = svc_item->set_check_resolver(mod_ctx, nullptr, resolve, nullptr, nullptr);
    if (result == MOD_OK) result = svc_item->observe_gives(mod_ctx, give, nullptr, nullptr);
    if (result == MOD_OK) result = mods::hook::add_pre<GetItem>(item_pre);
    if (result == MOD_OK) result = mods::hook::add_pre<ItemMessage>(item_message);
    if (result == MOD_OK) result = mods::hook::add_post<CheckMessage>(check_message_post);
    if (result == MOD_OK) result = register_text("You found a Flask Shard!\nFour shards add one Sacred Flask charge.", &g_shardMessage);
    if (result == MOD_OK) result = register_text("You found a Growth Fragment!\nFive fragments form a blessing. Choose health or stamina in your equipment menu.", &g_staminaMessage);
    return result;
}
}
