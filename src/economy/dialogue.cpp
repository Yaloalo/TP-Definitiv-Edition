#include "economy/dialogue.hpp"
#include "economy/dialogue_model.hpp"
#include "economy/dialogue_table.hpp"
#include "core/tuning.hpp"
#include "services.hpp"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_npc4.h"
#include "d/actor/d_a_alink.h"
#include "d/d_msg_class.h"
#include "f_op/f_op_actor_mng.h"
#include <set>
#include <map>
#include <tuple>
namespace remaster::economy::dialogue {
namespace {
DEFINE_HOOK_SYMBOL("dusk::flow::bind_resource",bool(const void*,uint16_t),BindMessages);
DEFINE_HOOK(&jmessage_tRenderingProcessor::do_tag, RenderTag);
DEFINE_HOOK(&jmessage_tMeasureProcessor::do_tag, MeasureTag);
std::set<std::pair<uint16_t,uint16_t>> g_registered;
std::vector<uint8_t> g_text;
std::map<std::pair<const void*,uint16_t>,uint64_t> g_resources;
bool enabled() {return is_active()&&tuning::get().removeCurrency&&tuning::get().economyDialogue;}
Topic speaker_topic(int profile) {
    switch(profile) {
    case fpcNm_NPC_MARO_e:case fpcNm_NPC_SMARO_e:return Topic::Shield;
    case fpcNm_NPC_CLERKA_e:case fpcNm_NPC_CLERKB_e:case fpcNm_NPC_CLERKT_e:return Topic::Armor;
    case fpcNm_NPC_BANS_e:return Topic::Barnes;
    case fpcNm_NPC_KKRI_e:return Topic::Coro;
    case fpcNm_NPC_SEIRA_e:case fpcNm_NPC_SHOP0_e:case fpcNm_NPC_GRM_e:return Topic::Supplies;
    case fpcNm_NPC_GRA_e:case fpcNm_NPC_GRMC_e:return Topic::Support;
    case fpcNm_NPC_PRAYER_e:return Topic::Prayer;
    case fpcNm_NPC_HENNA_e:case fpcNm_NPC_HENNA0_e:case fpcNm_NPC_CHIN_e:case fpcNm_NPC_HOZ_e:case fpcNm_NPC_RACA_e:return Topic::Challenge;
    case fpcNm_NPC_TOBY_e:case fpcNm_NPC_TKJ_e:case fpcNm_NPC_TKJ2_e:return Topic::Service;
    default:return Topic::None;
    }
}
Topic current_topic() {
    auto* actor=dComIfGp_event_getPt1();auto* link=daAlink_getAlinkActorClass();
    if(!actor||actor==link)actor=dComIfGp_event_getPt2();
    const auto topic=actor?speaker_topic(fopAcM_GetName(actor)):Topic::None;
    if(topic!=Topic::None)return topic;
    // Iza's scripted boat conversation may have Link as the event partner.
    if(std::string_view(dComIfGp_getStartStageName())=="F_SP112")return Topic::Challenge;
    return Topic::None;
}
const char* body(Topic topic) {
    if(topic==Topic::Armor&&!daNpcF_chkEvtBit(124))return "This shop's future depends on Malo's work.\nHelp the Kakariko community restore it.\nPermanent armor becomes available afterward.";
    switch(topic) {
    case Topic::Ward:return "Magic Armor holds a rechargeable ward.\nEach charge reduces one damaging hit.\nSafe travel and rest restore it; no rupees are spent.";
    case Topic::Shield:return "A Hylian Shield costs sixty rare rupees.\nSpeak to Malo for the permanent offer.\nRelics also unlock lasting artisan work.";
    case Topic::Hawkeye:return "Your Hawkeye comes with the Bow.\nThere is no purchase to make here.";
    case Topic::Armor:return "Magic Armor carries a rechargeable ward.\nThe permanent offer is 180 rare rupees.\nSigil, Sunward and Mirror relics improve it.";
    case Topic::Prayer:return "Share a prayer for Hyrule.\nYour time will suffice; no donation is needed.\nA: offer a prayer. B: another time.";
    case Topic::Support:return "Hyrule needs willing hands.\nLend your support; no payment is needed.\nA: help. B: another time.";
    case Topic::Coro:return "Keep a bottle for useful supplies.\nLantern oil returns while you travel safely.\nThere is no fuel bill to pay.";
    case Topic::Barnes:return "Bomb supplies return while you travel safely.\nYour adventures unlock bags and bomb types.\nBring relics to improve their lasting use.";
    case Topic::Supplies:return "Healing comes from your Sacred Flask.\nAmmo and oil return as you travel safely.\nSpeak to an artisan for permanent gear.";
    case Topic::Challenge:return "You can take part without an entry fee.\nKeep your earned prizes; replay for practice.";
    case Topic::Service:return "Your journey need not wait on a payment.\nWe can arrange this service without a fee.";
    default:return "Your help is what matters. No payment is needed.";
    }
}
bool override_text(ModContext*,const MessageOverrideContext* info,MessageTextData* out,void*) {
    if(!info||!out||!info->original_text)return false;
    const bool wardText=known(info->group,info->message_id)==Topic::Ward;
    if(wardText?!is_active():!enabled())return false;
    const std::span original(info->original_text,info->original_text_size);
    auto topic=known(info->group,info->message_id);const bool explicitPrompt=topic!=Topic::None;
    const auto plain=visible(original);if(plain.empty()&&!payment_tag(original))return false;
    if(!explicitPrompt) {
        topic=current_topic();
        if(input_prompt(original)&&topic!=Topic::Prayer)topic=Topic::Support;
        if(topic==Topic::None || (!money(lower(plain))&&!payment_tag(original)))return false;
    }
    const auto replacement=std::string(body(topic));
    const char* choices[]={topic==Topic::Prayer?"Offer a prayer":topic==Topic::Support?"Lend a hand":topic==Topic::Challenge?"Take part":"Continue","Another time","Leave"};
    bool changed=false;
    if(explicitPrompt || (!money(lower(plain))&&payment_tag(original)))changed=reword(original,replacement,choices,g_text);
    else changed=unprice(original,topic==Topic::Support?"your help":topic==Topic::Prayer?"a prayer":"no fee",g_text);
    if(!changed)return false;
    *out={g_text.data(),g_text.size()};return true;
}
ModResult register_prompt(uint16_t group,uint16_t id) {
    if(g_registered.contains({group,id}))return MOD_OK;
    std::array<MessageOverrideHandle,6> handles{};const uint8_t languages[]={0,1,2,3,4,6};
    for(size_t i=0;i<handles.size();++i) {
        if(svc_message->override_message_fn(mod_ctx,group,id,languages[i],override_text,nullptr,&handles[i])!=MOD_OK) {
            for(auto handle:handles)if(handle)svc_message->remove_override(mod_ctx,handle);
            mods::log::warn("Could not register economy dialogue {}:{}",group,id);return MOD_ERROR;
        }
    }
    g_registered.insert({group,id});return MOD_OK;
}
void bound_post(ModContext*,void* args,void* result,void*) {
    if(!result||!*static_cast<bool*>(result))return;
    const auto* data=static_cast<const uint8_t*>(mods::arg<const void*>(args,0));
    const auto group=mods::arg<uint16_t>(args,1);
    if(!data||group==0||group>8)return; // native item/story resources are not rewritten broadly
    const auto size=be32(data+8);if(size<0x20||size>8*1024*1024)return;
    uint64_t hash=14695981039346656037ull;
    for(size_t i=0;i<size;++i){hash^=data[i];hash*=1099511628211ull;}
    const auto key=std::make_pair(static_cast<const void*>(data),group);
    if(const auto it=g_resources.find(key);it!=g_resources.end()&&it->second==hash)return;
    if(g_resources.size()>=64)g_resources.clear();
    // bind_resource already validated this allocation; this is a read-only bounded view.
    std::vector<NativeMessage> entries;if(!messages({data,size},entries))return;
    bool complete=true;
    for(const auto& entry:entries) {
        const auto text=lower(visible(entry.text));
        if(known(group,entry.id)!=Topic::None||money(text)||payment_tag(entry.text))
            complete=(register_prompt(group,entry.id)==MOD_OK)&&complete;
    }
    if(complete)g_resources[key]=hash;
}
HookAction render_pre(ModContext*,void* args,void* result,void*) {
    if(!enabled()||mods::arg<uint32_t>(args,1)!=MSGTAG_INPUT_VALUE)return HOOK_CONTINUE;
    // Keep the sequence processor's input/quest tag, but hide numeric glyphs.
    *static_cast<bool*>(result)=true;return HOOK_SKIP_ORIGINAL;
}
}
ModResult install() {
    auto r=mods::hook::add_post<BindMessages>(bound_post);
    if(r==MOD_OK)r=mods::hook::add_pre<RenderTag>(render_pre);
    if(r==MOD_OK)r=mods::hook::add_pre<MeasureTag>(render_pre);
    if(r==MOD_OK)for(const auto& prompt:kPrompts){r=register_prompt(prompt.group,prompt.id);if(r!=MOD_OK)break;}
    return r;
}
}
