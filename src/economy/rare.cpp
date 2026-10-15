#include "economy/rare.hpp"
#include "progression/rewards.hpp"
#include "progression/growth.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "hud/idle_fade.hpp"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_npc4.h"
#include "d/d_item_data.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <set>
#include <cstring>
#include <fmt/format.h>
namespace remaster::economy::rare {
namespace {
std::set<std::string> g_pending;
MessageId g_cacheMessage=0,g_blessingMessage=0,g_flaskMessage=0,g_storyMessage=0;
void reset(ModContext*,uint32_t,void*) { g_pending.clear(); }
bool owned(const Offer& item) {
    return item.item==dItemNo_HYLIA_SHIELD_e?dComIfGs_isCollectShield(COLLECT_HYLIAN_SHIELD):dComIfGs_isCollectClothes(2);
}
void give(ModContext*,const ItemGiveInfo* info,void*) {
    if (!info || !info->check_name) return;
    g_pending.erase(info->check_name);
    if (!ensure()) return;
    auto& m=progression::model();
    if (info->item==kRupeeCache && award_cache(m,info->check_name)) progression::persist();
    if (const auto* item=offer(info->check_name)) {
        if (info->item==item->item && pay(m,*item)) {
            if (item->item==dItemNo_HYLIA_SHIELD_e) dComIfGs_setCollectShield(COLLECT_HYLIAN_SHIELD);
            else dComIfGs_setCollectClothes(2);
            progression::persist();
        }
        g_pending.erase(info->check_name);
    }
}
ModResult text(const char* value,MessageId* id) {
    std::array<MessageVariantData,6> variants{}; constexpr uint8_t languages[]={0,1,2,3,4,6};
    for (size_t i=0;i<variants.size();++i) {
        auto& v=variants[i];v.language=languages[i];v.entry.bytes[9]=MESSAGE_BOX_ITEM_GET;
        v.entry.bytes[10]=MESSAGE_DRAW_TYPED;v.entry.bytes[11]=MESSAGE_POSITION_MIDDLE;
        v.entry.bytes[12]=255;v.entry.bytes[13]=1;v.entry.bytes[18]=4;
        v.text=reinterpret_cast<const uint8_t*>(value);v.text_size=std::strlen(value)+1;
    }
    MessageHandle handle=0;return svc_message->register_message(mod_ctx,0,variants.data(),variants.size(),id,&handle);
}
}
bool enabled() { return is_active() && tuning::get().removeCurrency; }
bool ensure() {
    if (!enabled() || !progression::growth::ensure()) return false;
    if (!progression::model().claimed.contains("migration:rare-economy-v1")) {
        if (!migrate_economy(progression::model())) return false;
        progression::persist();
    }
    // A previously redeemed sigil remains a permanent benefit across intermediate saves.
    auto& m=progression::model();
    if(m.claimed.contains("trade:sigil")&&!(m.equipmentUpgrades&1)){m.equipmentUpgrades|=1;progression::persist();}
    return true;
}
bool custom_item(uint8_t item) { return item==kRupeeCache || item==kBlessing || item==kFlaskBlessing || item==kStoryAcknowledgment; }
bool can_purchase(const Offer& item) {
    auto* link=daAlink_getAlinkActorClass();
    if (!enabled() || !progression::ready() || !link || progression::model().claimed.size()>=512 || owned(item) ||
        g_pending.contains(item.check) || progression::model().claimed.contains(item.check) ||
        std::strcmp(dComIfGp_getStartStageName(),item.stage)!=0 || fopAcM_GetRoomNo(link)!=item.room) return false;
    const bool unlocked=item.item==dItemNo_HYLIA_SHIELD_e?bool(dComIfGs_isLightDropGetFlag(1)):bool(dComIfGs_isCollectCrystal(2)&&daNpcF_chkEvtBit(124));
    int reserved=0;for (const auto& check:g_pending) if (const auto* pending=offer(check)) reserved+=pending->cost;
    return unlocked && progression::model().rareRupees>=uint32_t(item.cost+reserved);
}
bool purchase(const Offer& item) {
    if (!ensure() || !can_purchase(item) || svc_item->give_item(mod_ctx,item.check,item.item,ITEM_GIVE_RESOLVE|ITEM_GIVE_SILENT)!=MOD_OK) return false;
    g_pending.insert(item.check);return true;
}
bool resolve(const ItemCheckInfo* info,ItemCheckResolution* out) {
    if (!info || !info->name || !out) return false;
    // Synthetic grants must cancel rather than fall through after a toggle/mode change.
    if(offer(info->name)&&!enabled()) { *out={dItemNo_NONE_e,dItemNo_NONE_e,true};return true; }
    if (!enabled()) return false;
    if (cache(info->name) && ((info->vanilla_item>=1 && info->vanilla_item<=7) || info->vanilla_item==dItemNo_LINKS_SAVINGS_e)) {
        *out={kRupeeCache,dItemNo_PURPLE_RUPEE_e,true};return true;
    }
    if (const auto* item=offer(info->name)) {
        const bool unlocked=item->item==dItemNo_HYLIA_SHIELD_e?bool(dComIfGs_isLightDropGetFlag(1)):bool(dComIfGs_isCollectCrystal(2)&&daNpcF_chkEvtBit(124));
        const bool allowed=progression::ready() && unlocked && progression::model().claimed.size()<512 && !owned(*item) && !progression::model().claimed.contains(item->check) && progression::model().rareRupees>=uint32_t(item->cost);
        *out={uint8_t(allowed?item->item:dItemNo_NONE_e),uint8_t(allowed?item->item:dItemNo_NONE_e),true};return true;
    }
    return false;
}
uint32_t message(uint32_t id) {
    if(is_active() && id==kStoryAcknowledgment+0x65)return g_storyMessage;
    if (!enabled()) return id;
    if (id==kRupeeCache+0x65) return g_cacheMessage;
    if (id==kBlessing+0x65) return g_blessingMessage;
    if (id==kFlaskBlessing+0x65) return g_flaskMessage;
    return id;
}
void tick() { if (player::input_allowed()) ensure(); }
ModResult install() {
    auto r=svc_item->observe_gives(mod_ctx,give,nullptr,nullptr);
    if (r==MOD_OK) r=svc_save->observe_saves(mod_ctx,reset,reset,nullptr,nullptr,nullptr);
    if (r==MOD_OK) r=text("You found 15 Hylian rupees!\nRare treasure for permanent equipment. There are no repeatable payouts.",&g_cacheMessage);
    if (r==MOD_OK) r=text("A blessing of Hyrule!\nCheck Growth & Equipment in your pause menu.",&g_blessingMessage);
    if (r==MOD_OK) r=text("Your Sacred Flask grows stronger!\nFour shards add one charge.",&g_flaskMessage);
    if (r==MOD_OK) r=text("Your kindness is remembered in Hyrule.", &g_storyMessage);
    return r;
}
}
