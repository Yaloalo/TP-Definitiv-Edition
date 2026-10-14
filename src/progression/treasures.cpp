#include "progression/treasures.hpp"
#include "progression/treasure_model.hpp"
#include "progression/rewards.hpp"
#include "progression/growth.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "resources/recharge_model.hpp"
#include "economy/rare_model.hpp"
#include "d/d_item_data.h"
#include "d/d_com_inf_game.h"
#include <cstring>
#include <fmt/format.h>
namespace remaster::progression::treasures {
namespace {
MessageId g_findMessage=0,g_threadMessage=0,g_upgradeMessage=0;
bool removable(uint8_t item) {
    return (tuning::get().removeCurrency && ((item>=1&&item<=7)||item==dItemNo_LINKS_SAVINGS_e)) ||
        (tuning::get().removeConsumableDrops&&resources::ammo_pickup(item));
}
bool persistent(std::string_view name) {
    return (name.starts_with("chest:")||name.starts_with("freestanding:"))&&
        !name.ends_with(":255")&&!name.ends_with(":-1");
}
void give(ModContext*,const ItemGiveInfo* info,void*) {
    if(!info || !info->check_name || !ensure())return;
    bool changed=false;
    if(info->item==kGildedThread)changed=thread(model(),info->check_name);
    if(info->item==kRelicFind) {
        const auto* entry=find(info->check_name);
        if(entry || persistent(info->check_name) || std::string_view(info->check_name)=="archery_reward:F_SP109")changed=material(model(),info->check_name,entry);
        if(changed && entry) {
            UiToastDesc desc=UI_TOAST_DESC_INIT;
            desc.title_rml=entry->kind==Kind::Sunward?"Sunward Emblem":entry->kind==Kind::Mirror?"Mirror Heart":
                entry->kind==Kind::Ancient?"Ancient Rune":"Hyrule Craft Relic";
            desc.body_rml="A finite keepsake for a permanent upgrade. Speak to Sera, Malo or Barnes; relics and recipes are in your equipment menu.";
            svc_ui->push_toast(mod_ctx,&desc);
        }
    }
    if(changed)persist();
    // Native permanent prizes retain their inventory/story functions and earn an honor once.
    uint32_t honor=0;
    if(std::strcmp(info->check_name,"star_reward_2")==0)honor=4;
    if(std::strcmp(info->check_name,"snowboard_race_reward")==0)honor=8;
    if(std::strcmp(info->check_name,"iza_reward_2")==0)honor=16;
    if(std::strcmp(info->check_name,"fairy_reward:D_SB01")==0)honor=32;
    if(honor && info->item!=dItemNo_NONE_e && !(model().honors&honor)) {model().honors|=honor;persist();}
}
ModResult text(const char* content,MessageId* id) {
    std::array<MessageVariantData,6> variants{};constexpr uint8_t languages[]={0,1,2,3,4,6};
    for(size_t i=0;i<variants.size();++i) {
        auto& v=variants[i];v.language=languages[i];v.entry.bytes[9]=MESSAGE_BOX_ITEM_GET;
        v.entry.bytes[10]=MESSAGE_DRAW_TYPED;v.entry.bytes[11]=MESSAGE_POSITION_MIDDLE;
        v.entry.bytes[12]=255;v.entry.bytes[13]=1;v.entry.bytes[18]=4;
        v.text=reinterpret_cast<const uint8_t*>(content);v.text_size=std::strlen(content)+1;
    }
    MessageHandle handle=0;return svc_message->register_message(mod_ctx,0,variants.data(),variants.size(),id,&handle);
}
uint32_t remap_message(uint32_t id) {
    if(!is_active())return id;
    if(id==kRelicFind+0x65)id=g_findMessage;
    if(id==kGildedThread+0x65)id=g_threadMessage;
    if(id==kUpgradeGift+0x65)id=g_upgradeMessage;
    return id;
}

}
bool ensure() {
    if(!is_active() || !growth::ensure())return false;
    if(!model().claimed.contains("migration:relic-ledger-v1")) {
        if(!migrate(model()))return false;
        persist();
    }
    const std::pair<uint16_t,uint32_t> prizes[]={{0x2301,4},{0x3b10,8},{0x5908,16},{0x3e40,32}};
    uint32_t honors=model().honors;
    for(const auto& [flag,honor]:prizes)if(dComIfGs_isEventBit(flag))honors|=honor;
    if(honors!=model().honors){model().honors=honors;persist();}
    return true;
}
uint32_t message(uint32_t id) {return remap_message(id);}
bool custom_item(uint8_t item) {return item==kRelicFind||item==kGildedThread||item==kUpgradeGift;}
bool resolve(const ItemCheckInfo* info,ItemCheckResolution* out) {
    if(!is_active()||!info||!info->name||!out||!removable(info->vanilla_item))return false;
    const std::string_view name(info->name);
    if(!persistent(name))return false;
    const auto* entry=find(name);
    if(entry && (entry->kind==Kind::Flask || entry->kind==Kind::Rupees))return false;
    if(entry && entry->kind==Kind::Growth) {*out={kStaminaFragment,dItemNo_KAKERA_HEART_e,true};return true;}
    *out={kRelicFind,dItemNo_ZORAS_JEWEL_e,true};return true;
}
float sprint_cost_scale() {return is_active()&&ready()?sprint_scale(model()):1.0f;}
int64_t recharge_interval(int64_t ms,bool arrows) {return is_active()&&ready()?refill_interval(model(),ms,arrows):ms;}
void tick() {
    if(!player::input_allowed()||!ensure())return;
    const auto count=found(model());const auto earned=(count>=40?1u:0u)|(count>=160?2u:0u);
    if((model().honors&earned)!=earned){model().honors|=earned;persist();}
}
ModResult install() {
    auto r=svc_item->observe_gives(mod_ctx,give,nullptr,nullptr);

    if(r==MOD_OK)r=text("A relic of Hyrule!\nKeep it for a permanent upgrade. Your equipment menu records its purpose.",&g_findMessage);
    if(r==MOD_OK)r=text("Agitha gives you Gilded Thread!\nA golden guest's gift for a permanent equipment upgrade.",&g_threadMessage);
    if(r==MOD_OK)r=text("Your equipment has been permanently improved!",&g_upgradeMessage);
    return r;
}
}
