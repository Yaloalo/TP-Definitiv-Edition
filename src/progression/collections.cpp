#include "progression/collections.hpp"
#include "progression/collection_model.hpp"
#include "progression/growth.hpp"
#include "progression/rewards.hpp"
#include "economy/rare.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/actor/d_a_npc4.h"
#include <charconv>
#include <set>
#include <fmt/format.h>
namespace remaster::progression::collections {
namespace {
std::set<std::string> g_pending;
bool delivered(int i) { return daNpcF_chkEvtBit(0x191+i); }
bool caught(int i) { return dComIfGs_isItemFirstBit(dItemNo_M_BEETLE_e+i); }
std::string bug_name(int i) { return fmt::format("bug:{}",dItemNo_M_BEETLE_e+i); }
bool notice(const char* text) {
    UiToastDesc desc=UI_TOAST_DESC_INIT;desc.title_rml="A gift of Hyrule";desc.body_rml=text;
    return svc_ui->push_toast(mod_ctx,&desc)==MOD_OK;
}
void reset(ModContext*,uint32_t,void*) { g_pending.clear(); }
void give(ModContext*,const ItemGiveInfo* info,void*) {
    if (!info || !info->check_name || !ensure()) return;
    const std::string_view name(info->check_name);
    if (name.starts_with("bug:") && info->item==economy::rare::kBlessing) {
        if (model().claim(info->check_name,0x14)) { growth::apply_health();persist(); }
    }
    const char* canonical=info->check_name;
    if (name=="jovani_reward_1") canonical="collection:poes:20";
    if (name=="jovani_reward_2") canonical="collection:poes:60";
    if (const auto* reward=milestone(canonical); reward && info->item==reward->item) {
        // Relic ownership is committed by the merchant observer; growth
        // fragments already go through the progression observer.
        bool changed=false;
        if (reward->item==economy::rare::kBlessing || reward->item==economy::rare::kFlaskBlessing)
            changed=apply(model(),*reward,tuning::get().flaskStartingCharges,tuning::get().flaskMaximumCharges);
        if (changed) {growth::apply_health();persist();notice(reward->label);}
    }
    g_pending.erase(info->check_name);
}
}
bool ensure() {
    if (!economy::rare::ensure()) return false;
    if (!model().claimed.contains("migration:collection-gifts-v1")) {
        auto candidate=model();
        if (!candidate.claim("migration:collection-gifts-v1",0xff)) return false;
        for(int i=0;i<24;++i) if(delivered(i)) {
            const auto name=bug_name(i);
            if (!candidate.claimed.contains(name) && !candidate.claim(name.c_str(),0x14)) return false;
        }
        model()=std::move(candidate);growth::apply_health();persist();
    }
    return true;
}
int pairs() {int result=0;for(int i=0;i<12;++i) result+=delivered(i*2)&&delivered(i*2+1);return result;}
int souls() {return std::min<int>(60,dComIfGs_getPohSpiritNum());}
int unpresented() {int result=0;for(int i=0;i<24;++i) result+=caught(i)&&!delivered(i);return result;}
bool present_bugs() {
    if (!ensure()) return false;
    bool changed=false;
    for(int i=0;i<24;++i) if(caught(i)&&!delivered(i)) {
        const auto name=bug_name(i);
        if (!model().claimed.contains(name) && !model().claim(name.c_str(),0x14)) continue;
        daNpcF_onEvtBit(0x191+i);changed=true;
    }
    if(changed){growth::apply_health();persist();}
    claim_rewards(true);return changed;
}
bool rewards_available(bool bugs) {
    if (!economy::rare::enabled() || !ready() || model().claimed.size()>=512) return false;
    const int count=bugs?pairs():souls();
    const auto eligible=[&](const auto& table){for(const auto& r:table) if(count>=int(r.threshold)&&!model().claimed.contains(r.check)&&!g_pending.contains(r.check))return true;return false;};
    return bugs?eligible(kBugs):eligible(kPoes);
}
bool claim_rewards(bool bugs) {
    if (!ensure() || model().claimed.size()>=512) return false;
    const int count=bugs?pairs():souls();bool queued=false;
    const auto queue=[&](const auto& table){for(const auto& r:table) if(count>=int(r.threshold)&&!model().claimed.contains(r.check)&&!g_pending.contains(r.check)) {
        if(svc_item->give_item(mod_ctx,r.check,r.item,ITEM_GIVE_RESOLVE|ITEM_GIVE_SILENT)==MOD_OK){g_pending.insert(r.check);queued=true;}
    }};
    if(bugs)queue(kBugs);else queue(kPoes);return queued;
}
bool resolve(const ItemCheckInfo* info,ItemCheckResolution* out) {
    if (!economy::rare::enabled() || !info || !info->name || !out) return false;
    const std::string_view name(info->name);
    if (name.starts_with("bug:")) {
        int id=0;const auto text=name.substr(4);const auto parse=std::from_chars(text.data(),text.data()+text.size(),id);
        if(parse.ec!=std::errc{} || parse.ptr!=text.data()+text.size() || id<dItemNo_M_BEETLE_e || id>dItemNo_F_MAYFLY_e)return false;
        const bool valid=ready()&&!model().claimed.contains(std::string(name))&&model().claimed.size()<512;
        *out={uint8_t(valid?economy::rare::kBlessing:dItemNo_NONE_e),uint8_t(valid?dItemNo_KAKERA_HEART_e:dItemNo_NONE_e),true};return true;
    }
    const Milestone* reward=milestone(name);
    if(name=="jovani_reward_1")reward=&kPoes[2];
    if(name=="jovani_reward_2")reward=&kPoes[4];
    if(!reward)return false;
    const bool bugs=std::string_view(reward->check).starts_with("collection:bugs:");
    const bool valid=((bugs?pairs():souls())>=int(reward->threshold)&&ready()&&!model().claimed.contains(reward->check)&&model().claimed.size()<512);
    const uint8_t display=reward->item==0x38||reward->item==0x39?dItemNo_ZORAS_JEWEL_e:
        reward->shards?dItemNo_RED_BOTTLE_e:dItemNo_UTAWA_HEART_e;
    *out={uint8_t(valid?reward->item:dItemNo_NONE_e),uint8_t(valid?display:dItemNo_NONE_e),true};return true;
}
void tick() {if(player::input_allowed())ensure();}
ModResult install() {
    auto r=svc_item->observe_gives(mod_ctx,give,nullptr,nullptr);
    if(r==MOD_OK)r=svc_save->observe_saves(mod_ctx,reset,reset,nullptr,nullptr,nullptr);
    return r;
}
}
