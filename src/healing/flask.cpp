#include "hud/draw_context.hpp"
#include "d/d_pane_class.h"
#include "healing/flask.hpp"
#include "combat/ward.hpp"
#include "healing/wolf.hpp"
#include "healing/rest_stones.hpp"
#include "hud/idle_fade.hpp"
#include "healing/snowpeak_hearts.hpp"
#include "resources/ammo.hpp"
#include "economy/merchants.hpp"
#include "resources/recharge_model.hpp"
#include "progression/rewards.hpp"
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "core/difficulty.hpp"
#include "core/hooks.hpp"
#include "services.hpp"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_obj_yousei.h"
#include "d/actor/d_a_obj_item.h"
#include "d/actor/d_a_shop_item.h"
#include "d/d_shop_system.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "f_op/f_op_actor_iter.h"
#include "JSystem/JUtility/JUTFont.h"
#include "d/d_pane_class.h"
#include "d/d_menu_ring.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "m_Do/m_Do_ext.h"
#include <cstring>
#include <fmt/format.h>

namespace remaster::healing {
namespace {
DEFINE_HOOK(&daAlink_c::procBottleDrinkInit, DrinkInit);
DEFINE_HOOK(&daAlink_c::procBottleDrink, DrinkProc);
DEFINE_HOOK(&daAlink_c::commonBottleDrink, CommonDrink);
using LifeCount=hooks::LifeCount;
DEFINE_HOOK(&daAlink_c::procCoDeadInit, DeathInit);
DEFINE_HOOK(static_cast<void(*)()>(&dComIfGs_onStageBossEnemy), BossVictory);
DEFINE_HOOK(static_cast<void(*)(int)>(&dComIfGs_onStageBossEnemy), IndexedBossVictory);
DEFINE_HOOK(&daObjYOUSEI_c::create, FairyCreate);
DEFINE_HOOK(&fopAcM_createItem, DropCreate);
DEFINE_HOOK(&fopAcM_fastCreateItem2, FastDropCreate);
DEFINE_HOOK(&fopAcM_fastCreateItem, GeneralDropCreate);
DEFINE_HOOK(&daItem_c::_daItem_create, FieldItemCreate);
DEFINE_HOOK(&daAlink_c::procHorseBottleDrinkInit, HorseDrink);
DEFINE_HOOK(&daAlink_c::procCanoeBottleDrinkInit, CanoeDrink);
DEFINE_HOOK(&daShopItem_c::getShopArcname, ShopArc);
DEFINE_HOOK(&dShopSystem_c::isSoldOutItemFlag, ShopSoldOut);
DEFINE_HOOK(&dMenu_Ring_c::getItemNum, RingCount);
DEFINE_HOOK(&dMenu_Ring_c::getItemMaxNum, RingMaximum);
daAlink_c* g_drinking = nullptr;
bool g_healed = false, g_retry = false;
unsigned g_bottleDepth = 0;
uint32_t g_saveSlot = 255, g_retrySlot = 255;
UiElementHandle g_info = 0;
bool enabled() { return is_active() && tuning::get().flaskEnabled && progression::ready(); }
auto& progress() { return progression::model(); }
uint32_t capacity() { return progress().capacity(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges); }
bool healing_content(uint8_t item) {
    return item == dItemNo_RED_BOTTLE_e || item == dItemNo_RED_BOTTLE_2_e || item == dItemNo_GREEN_BOTTLE_e ||
        item == dItemNo_BLUE_BOTTLE_e || item == dItemNo_MILK_BOTTLE_e || item == dItemNo_HALF_MILK_BOTTLE_e ||
        item == dItemNo_FAIRY_e || item == dItemNo_FAIRY_DROP_e || (item >= dItemNo_CHUCHU_RARE_e && item <= dItemNo_CHUCHU_PURPLE_e);
}
void set_bottle(int slot, uint8_t item) { dComIfGs_setItem(slot, item); dComIfGp_setItem(slot, item); }
HookAction drink_init(ModContext*, void* args, void* result, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const auto item = mods::arg<u16>(args, 1);
    if (!link) return HOOK_CONTINUE;
    const bool flask = item == dItemNo_RED_BOTTLE_e &&
        dComIfGs_getSelectItemIndex(link->mSelectItemId) == progress().bottleSlot;
    if (!flask) {
        // Scripted soup scenes retain their animations; positive healing is suppressed in CommonDrink.
        if (healing_content(item) && !dComIfGp_event_runCheck()) {
            *static_cast<int*>(result) = 0; return HOOK_SKIP_ORIGINAL;
        }
        return HOOK_CONTINUE;
    }
    *static_cast<int*>(result) = 0;
    if (!player::gameplay_running() || link->checkWolf() || link->checkModeFlg(0x400 | 0x40000) ||
        dComIfGs_getLife() >= dComIfGs_getMaxLifeGauge() ||
        !progress().spend(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges)) return HOOK_SKIP_ORIGINAL;
    // Native bottle animation and model, without compulsory events, camera ownership or invulnerability.
    link->commonProcInit(daAlink_c::PROC_BOTTLE_DRINK);
    link->current.angle.y = link->shape_angle.y;
    link->mNormalSpeed = 0.0f;
    link->field_0x2f98 = 4;
    link->setSingleAnimeParam(daAlink_c::ANM_BOTTLE_DRINK_START, &link->mpHIO->mItem.mBottle.m.mStartDrinkAnm);
    link->keepItemData(); link->setBottleModel(item);
    link->mProcVar0.field_0x3008 = 0; link->mProcVar2.field_0x300c = 0; link->mProcVar3.field_0x300e = 0;
    g_drinking = link; g_healed = false;
    progression::persist();
    *static_cast<int*>(result) = 1; return HOOK_SKIP_ORIGINAL;
}
HookAction drink_proc(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!enabled() || link != g_drinking) return HOOK_CONTINUE;
    if (link->mEquipItem == dItemNo_NONE_e || link->mEquipItem == dItemNo_KANTERA_e) {
        // Finish before native commonBottleDrink's final branch releases global event/camera state.
        g_drinking = nullptr; link->returnKeepItemData(); link->procWaitInit();
    } else if (link->commonBottleDrink(0)) { g_drinking = nullptr; link->procWaitInit(); }
    *static_cast<int*>(result) = 1; return HOOK_SKIP_ORIGINAL;
}
HookAction common_pre(ModContext*, void*, void*, void*) { if (enabled()) ++g_bottleDepth; return HOOK_CONTINUE; }
void common_post(ModContext*, void*, void*, void*) { if (g_bottleDepth) --g_bottleDepth; }
HookAction life_pre(ModContext*, void* args, void*, void*) {
    if (is_active() && mods::arg<f32>(args, 0) < 0) {
        resources::ammo::damage_received();
        damage_interrupted(daAlink_getAlinkActorClass());
    }
    if (!enabled() || !g_bottleDepth || mods::arg<f32>(args, 0) <= 0) return HOOK_CONTINUE;
    auto* link = daAlink_getAlinkActorClass();
    if (!link || g_drinking != link || g_healed || link->mProcID != daAlink_c::PROC_BOTTLE_DRINK) return HOOK_SKIP_ORIGINAL;
    mods::arg_ref<f32>(args, 0) = progress().potency(tuning::get().flaskHealingHearts) * 4.0f;
    g_healed = true; return HOOK_CONTINUE;
}
void death_post(ModContext*, void*, void* result, void*) {
    if (is_active() && *static_cast<int*>(result)) {resources::ammo::death();combat::ward::death();}
    if (enabled() && *static_cast<int*>(result)) { g_retry = true; g_retrySlot = g_saveSlot; g_drinking = nullptr; }
}
void boss_victory_post(ModContext*, void*, void*, void*) {
    if (!is_active()) return;
    const char* stage = dComIfGp_getStartStageName();
    if (!stage || std::strncmp(stage, "D_MN", 4) != 0) return;
    combat::ward::boss_victory(stage);
    if(!enabled())return;
    const auto check = std::string("victory:") + std::string(stage).substr(0, 6);
    if (progress().claim(check.c_str(), 0xff)) refill(true);
}
HookAction fairy_pre(ModContext*, void*, void* result, void*) {
    if (!is_active() || !tuning::get().flaskEnabled) return HOOK_CONTINUE;
    *static_cast<int*>(result) = cPhs_ERROR_e; return HOOK_SKIP_ORIGINAL;
}
bool forbidden(int item) {
    return (tuning::get().removeHeartDrops && (item == dItemNo_HEART_e || item == dItemNo_TRIPLE_HEART_e) && !snowpeak::allows_spawn()) ||
        (tuning::get().removeCurrency && item >= 1 && item <= 7) ||
        (tuning::get().flaskEnabled && item == dItemNo_RECOVERY_FAILY_e) ||
        resources::ammo::removes_pickup(item);
}
HookAction drop_pre(ModContext*, void* args, void* result, void*) {
    // Persistent world checks need to reach ItemService for conversion; only disposable spawns are rejected here.
    int bit = mods::arg<int>(args, 2);
    if (!is_active() || (bit >= 0 && bit != 255) || !forbidden(mods::arg<int>(args, 1))) return HOOK_CONTINUE;
    *static_cast<fpc_ProcID*>(result) = fpcM_ERROR_PROCESS_ID_e; return HOOK_SKIP_ORIGINAL;
}
HookAction fast_drop_pre(ModContext*, void* args, void* result, void*) {
    int bit = mods::arg<int>(args, 2);
    if (!is_active() || (bit >= 0 && bit != 255) || !forbidden(mods::arg<int>(args, 1))) return HOOK_CONTINUE;
    *static_cast<fopAc_ac_c**>(result) = nullptr; return HOOK_SKIP_ORIGINAL;
}
HookAction general_drop_pre(ModContext*, void* args, void* result, void*) {
    const int bit = mods::arg<int>(args, 7);
    if (!is_active() || (bit >= 0 && bit != 255) || !forbidden(mods::arg<int>(args, 1))) return HOOK_CONTINUE;
    *static_cast<fopAc_ac_c**>(result) = nullptr; return HOOK_SKIP_ORIGINAL;
}
HookAction field_item_pre(ModContext*, void* args, void* result, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    auto* actor = mods::arg<daItem_c*>(args, 0); if (!actor) return HOOK_CONTINUE;
    const int item = daItem_prm::getItemNo(actor);
    if (item == dItemNo_HEART_e && snowpeak::allows_actor(actor)) return HOOK_CONTINUE;
    if (!forbidden(item)) return HOOK_CONTINUE;
    // Persistent coin actors are converted by the host resolver during create(). Hearts never carry upgrades.
    if (((item >= 1 && item <= 7) || resources::ammo::removes_pickup(item)) && daItem_prm::getItemBitNo(actor) != 255) return HOOK_CONTINUE;
    *static_cast<int*>(result) = cPhs_ERROR_e; return HOOK_SKIP_ORIGINAL;
}
HookAction mounted_drink_pre(ModContext*, void* args, void* result, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    const auto item = mods::arg<u16>(args, 1);
    if (!healing_content(item) || dComIfGp_event_runCheck()) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 0; return HOOK_SKIP_ORIGINAL;
}
HookAction shop_arc_before(ModContext*, void* args, void*, void*) {
    if (!enabled() && !economy::merchants::enabled() && !resources::ammo::enabled()) return HOOK_CONTINUE;
    auto* stock = mods::arg<daShopItem_c*>(args, 0);
    if (stock && (economy::merchants::enabled() || healing_content(stock->m_itemNo) || resources::ammo::removes_pickup(stock->m_itemNo))) stock->m_itemNo = dItemNo_NONE_e;
    // Native NONE creates the shop's sold-out sign, preserving shelf indices and equipment stock.
    return HOOK_CONTINUE;
}
HookAction shop_sold_out_before(ModContext*, void* args, void* result, void*) {
    if (!enabled()) return HOOK_CONTINUE;
    auto* shop = mods::arg<dShopSystem_c*>(args, 0);
    const int index = mods::arg<int>(args, 1);
    if (!shop || index < 0 || index >= dShopSystem_c::ITEM_MAX_e) return HOOK_CONTINUE;
    auto* actor = fopAcM_SearchByID(shop->getShopItemCtrlP()->getItemIndex(index));
    if (!actor || fopAcM_GetName(actor) != fpcNm_ShopItem_e || static_cast<daShopItem_c*>(actor)->m_itemNo != dItemNo_NONE_e) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = TRUE; return HOOK_SKIP_ORIGINAL;
}
HookAction ring_count(ModContext*, void* args, void* result, void*) {
    if (!enabled() || mods::arg<u8>(args, 1) != progress().bottleSlot) return HOOK_CONTINUE;
    *static_cast<u8*>(result) = progress().charges; return HOOK_SKIP_ORIGINAL;
}
HookAction ring_max(ModContext*, void* args, void* result, void*) {
    if (!enabled() || mods::arg<u8>(args, 1) != progress().bottleSlot) return HOOK_CONTINUE;
    *static_cast<u8*>(result) = capacity(); return HOOK_SKIP_ORIGINAL;
}
void hud_post(ModContext*, void* args, void*, void*) {
    if (!enabled() || !player::input_allowed() || dComIfGp_isEnableNextStage()) return;
    auto* meter=mods::arg<dMeter2Draw_c*>(args,0);
    if (!meter || !meter->mpLifeParent || meter->mpLifeParent->getAlphaRate()<=0) return;
    float left=10000, bottom=-10000;
    for (auto* heart:meter->mpLifeParts) {
        if (!heart || !heart->getPanePtr() || !heart->getPanePtr()->isVisible()) continue;
        Mtx matrix;
        const auto a=heart->getGlobalVtx(&matrix,0,false,0), b=heart->getGlobalVtx(&matrix,3,false,0);
        left=std::min(left,std::min(a.x,b.x)); bottom=std::max(bottom,std::max(a.y,b.y));
    }
    if (bottom<0) return;
    hud::DrawContext context;
    const float x=left+4, y=bottom+29*std::max(1.0f,std::abs(meter->mpLifeParent->getScaleY())); // leave two native meter rows for stamina/oxygen
    const u8 alpha=u8(hud::idle_fade::opacity()*meter->mpLifeParent->getAlphaRate()*255);
    const JUtility::TColor gold(225,204,150,alpha), dim(59,61,54,alpha);
    // Small bottle silhouette and one lit segment per usable charge.
    J2DFillBox(x+3,y-3,4,3,gold);
    J2DFillBox(x,y,10,12,gold);
    J2DFillBox(x+1,y+1,8,10,dim);
    if (progress().charges) J2DFillBox(x+2,y+5,6,5,gold);
    for (int i=0;i<capacity();++i) {
        J2DFillBox(x+16+i*8,y+2,6,8,gold);
        J2DFillBox(x+17+i*8,y+3,4,6,i<progress().charges?gold:dim);
    }
}
// Mods UI itself pauses gameplay, so evaluate rest's underlying safety without requiring a closed pane.
bool rest_disabled(ModContext*, void*) {
    return !stones::rest_available(daAlink_getAlinkActorClass());
}
bool flask_text(ModContext*, const MessageOverrideContext* msg, MessageTextData* out, void*) {
    if (!enabled()) return false;
    static const char* name = "Sacred Flask";
    static const char* description = "Reusable healing. Assign to X or Y.\nRest or defeat a boss to refill.\nFour Flask Shards add one charge.";
    const char* text = msg->message_id == dItemNo_RED_BOTTLE_e + 0x165 ? name : description;
    *out = {reinterpret_cast<const uint8_t*>(text), std::strlen(text) + 1}; return true;
}
void rest_ui_pressed(ModContext*, void*) { stones::request_rest(daAlink_getAlinkActorClass()); }
}
void damage_interrupted(daAlink_c* link) {
    stones::damage_received();
    wolf::interrupt(link);
    if (link && link == g_drinking) {
        g_drinking = nullptr;
        link->returnKeepItemData();
        link->procWaitInit();
    }
}
void reset() {
    wolf::reset();
    auto* link = daAlink_getAlinkActorClass();
    if (g_drinking == link && link && link->mProcID == daAlink_c::PROC_BOTTLE_DRINK) damage_interrupted(link);
    g_drinking = nullptr; g_healed = false; g_bottleDepth = 0;
}
void save_slot_changed(uint32_t slot, bool fresh) {
    stones::reset();
    snowpeak::reset();
    g_saveSlot = slot;
    resources::ammo::slot_changed(slot, fresh);
    if (fresh || slot != g_retrySlot) g_retry = false;
    reset();
}
void normalize_bottles() {
    if (!enabled()) return;
    auto& m = progress();
    // Migration never overwrites water, oil, soup or other quest contents. Claim an unused/empty utility slot.
    if (m.bottleSlot == 255) {
        for (int i = 14; i >= 11; --i) {
            const auto item = dComIfGs_getItem(i, false);
            if (item == dItemNo_NONE_e || item == dItemNo_EMPTY_BOTTLE_e) { m.bottleSlot = i; progression::persist(); break; }
        }
    }
    for (int i = 11; i <= 14; ++i) {
        if (i == int(m.bottleSlot)) { if (dComIfGs_getItem(i, false) != dItemNo_RED_BOTTLE_e) set_bottle(i, dItemNo_RED_BOTTLE_e); }
        else if (healing_content(dComIfGs_getItem(i, false))) set_bottle(i, dItemNo_EMPTY_BOTTLE_e);
    }
}
void refill(bool heal) {
    resources::ammo::refill();
    combat::ward::refill();
    if (!enabled()) return;
    progress().refill(tuning::get().flaskStartingCharges, tuning::get().flaskMaximumCharges);
    if (heal) {
        const int missing = dComIfGs_getMaxLifeGauge() - dComIfGs_getLife();
        if (missing > 0) dComIfGp_setItemLifeCount(missing, 0);
    }
    progression::persist();
}
void tick(daAlink_c* link) {
    wolf::tick(link);
    stones::tick(link);
    if (!enabled()) { reset(); return; }
    if (progress().charges > capacity()) { progress().charges = capacity(); progression::persist(); }
    normalize_bottles();
    if (g_drinking && (g_drinking != link || link->mProcID != daAlink_c::PROC_BOTTLE_DRINK)) g_drinking = nullptr;
    if (g_saveSlot == 255) g_saveSlot = dComIfGs_getDataNum();
    const uint32_t bosses = difficulty::dungeons_cleared();
    if (bosses > progress().bosses) { progress().bosses = std::min<uint32_t>(9, bosses); refill(true); }
    // Only a genuine death sets retry; normal loading and changing scenes never restore spent charges.
    if (g_retry && player::gameplay_running() && link->mProcID != daAlink_c::PROC_DEAD && dComIfGs_getLife() > 0) {
        g_retry = false; refill(true);
    }
    if (g_info) {
        const auto text = fmt::format("Sacred Flask: {}/{} charges, {} hearts each. Flask shards {}/4; growth fragments {}/5.\nStand beside a Sacred Rest Stone and hold L + R. Rest takes 3 seconds by default. Death and boss victories refill.{}",
            progress().charges, capacity(), progress().potency(tuning::get().flaskHealingHearts), progress().flaskShards % 4,
            progress().growthFragments % 5, progress().bottleSlot == 255 ? " Empty a bottle to make room for the flask." : " Human: assign flask to X/Y. Wolf: R + X to recover.");
        if (svc_ui->elem_set_text(mod_ctx, g_info, text.c_str()) != MOD_OK) g_info = 0;
    }
}
ModResult build_controls(UiElementHandle pane) {
    auto result = svc_ui->pane_add_section(mod_ctx, pane, "Sacred Flask");
    if (result == MOD_OK) result = svc_ui->pane_add_text(mod_ctx, pane, "Select Definitive Overhaul to inspect your flask.", &g_info);
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.label = "Rest at nearby stone (close menu to begin)"; control.on_pressed = rest_ui_pressed; control.is_disabled = rest_disabled;
    if (result == MOD_OK) result = svc_ui->pane_add_control(mod_ctx, pane, &control, nullptr);
    return result;
}
ModResult install() {
    auto r = mods::hook::add_pre<DrinkInit>(drink_init);
    if (r == MOD_OK) r = mods::hook::add_pre<DrinkProc>(drink_proc);
    if (r == MOD_OK) r = mods::hook::add_pre<CommonDrink>(common_pre);
    if (r == MOD_OK) r = mods::hook::add_post<CommonDrink>(common_post);
    if (r == MOD_OK) r = mods::hook::add_pre<LifeCount>(life_pre);
    if (r == MOD_OK) r = mods::hook::add_post<DeathInit>(death_post);
    if (r == MOD_OK) r = mods::hook::add_post<BossVictory>(boss_victory_post);
    if (r == MOD_OK) r = mods::hook::add_post<IndexedBossVictory>(boss_victory_post);
    if (r == MOD_OK) r = mods::hook::add_pre<FairyCreate>(fairy_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<DropCreate>(drop_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<FastDropCreate>(fast_drop_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<GeneralDropCreate>(general_drop_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<FieldItemCreate>(field_item_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<HorseDrink>(mounted_drink_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<CanoeDrink>(mounted_drink_pre);
    if (r == MOD_OK) r = mods::hook::add_pre<ShopArc>(shop_arc_before);
    if (r == MOD_OK) r = mods::hook::add_pre<ShopSoldOut>(shop_sold_out_before);
    if (r == MOD_OK) r = mods::hook::add_pre<RingCount>(ring_count);
    if (r == MOD_OK) r = mods::hook::add_pre<RingMaximum>(ring_max);
    if (r == MOD_OK) r = mods::hook::add_post<hooks::Shared1>(hud_post);
    for (uint8_t language : {0, 1, 2, 3, 4, 6}) {
        for (uint16_t id : {uint16_t(dItemNo_RED_BOTTLE_e + 0x165), uint16_t(dItemNo_RED_BOTTLE_e + 0x265)}) {
            MessageOverrideHandle handle = 0;
            if (r == MOD_OK) r = svc_message->override_message_fn(mod_ctx, 0, id, language, flask_text, nullptr, &handle);
        }
    }
    return r;
}
}
