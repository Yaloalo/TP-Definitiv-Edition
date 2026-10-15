#include "hud/prompt.hpp"
#include "economy/conversations.hpp"
#include "economy/rare.hpp"
#include "economy/merchants.hpp"
#include "economy/merchant_model.hpp"
#include "economy/guild.hpp"
#include "progression/treasure_model.hpp"
#include "progression/rewards.hpp"
#include "progression/collections.hpp"
#include "core/player.hpp"
#include "core/hooks.hpp"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_npc4.h"
#include "d/d_item_data.h"
#include "f_op/f_op_actor_iter.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <cmath>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>
#include <fmt/format.h>
namespace remaster::economy::conversations {
namespace {
enum class Kind { None,Malo,Barnes,Armorer,Agitha,Jovani,Coro,Sera,Goron };
struct Context { fpc_ProcID id=fpcM_ERROR_PROCESS_ID_e; int profile=-1,room=-1;uint32_t slot=255;Kind kind=Kind::None;std::string stage; } g_context;
UiDialogHandle g_dialog=0;
fpc_ProcID g_native=fpcM_ERROR_PROCESS_ID_e;
using Clock=std::chrono::steady_clock;
Clock::time_point g_nativeUntil{};
enum class Action { Purchase,Trade,Craft,Present,Claim,Native,Leave };
struct Choice { Action action;const rare::Offer* offer=nullptr;const merchants::Relic* relic=nullptr;const guild::Recipe* recipe=nullptr; };
std::vector<Choice> g_choices;
Kind kind(fopAc_ac_c* actor) {
    if(!actor)return Kind::None;
    const int profile=fopAcM_GetName(actor),room=fopAcM_GetRoomNo(actor);
    const std::string_view stage=dComIfGp_getStartStageName();
    if(stage=="F_SP108"&&room==4&&profile==fpcNm_NPC_KKRI_e&&dComIfGs_isItemFirstBit(dItemNo_KANTERA_e))return Kind::Coro;
    if(stage=="R_SP01"&&room==1&&profile==fpcNm_NPC_SEIRA_e&&progression::model().claimed.contains("quest:sera_helped"))return Kind::Sera;
    if(stage=="R_SP160"&&room==4&&profile==fpcNm_NPC_GRM_e)return Kind::Goron;
    if(stage=="R_SP109"&&room==3&&profile==fpcNm_NPC_MARO_e)return Kind::Malo;
    if(stage=="R_SP109"&&room==1&&profile==fpcNm_NPC_BANS_e)return Kind::Barnes;
    if(stage=="R_SP160"&&room==0&&daNpcF_chkEvtBit(124)&&(profile==fpcNm_NPC_MARO_e||profile==fpcNm_NPC_CLERKA_e||profile==fpcNm_NPC_CLERKB_e||profile==fpcNm_NPC_CLERKT_e))return Kind::Armorer;
    if(stage=="R_SP160"&&profile==fpcNm_NPC_INS_e)return Kind::Agitha;
    if(stage=="R_SP160"&&profile==fpcNm_NPC_POUYA_e)return Kind::Jovani;
    return Kind::None;
}
bool safe(daAlink_c* link) {
    return (rare::enabled()||merchants::enabled())&&progression::ready()&&link&&!link->checkWolf()&&player::gameplay_running()&&
        link->mLinkAcch.ChkGroundHit()&&std::abs(link->mNormalSpeed)<0.1f&&
        (link->mProcID==daAlink_c::PROC_WAIT||link->mProcID==daAlink_c::PROC_SERVICE_WAIT||link->mProcID==daAlink_c::PROC_TIRED_WAIT);
}
void* threat(void* ptr,void* data) {
    auto* actor=static_cast<fopAc_ac_c*>(ptr);auto* link=static_cast<daAlink_c*>(data);
    return actor&&actor->health>0&&fopAcM_GetGroup(actor)==fopAc_ENEMY_e&&actor->current.pos.abs(link->current.pos)<3000?actor:nullptr;
}
struct Search { daAlink_c* link;fopAc_ac_c* found=nullptr;float distance=280; };
void* find(void* ptr,void* data) {
    auto* actor=static_cast<fopAc_ac_c*>(ptr);auto& query=*static_cast<Search*>(data);
    if(kind(actor)==Kind::None||fopAcM_GetRoomNo(actor)!=fopAcM_GetRoomNo(query.link))return nullptr;
    const float distance=actor->current.pos.abs(query.link->current.pos);
    if(distance<query.distance&&fopAcM_seenActorAngleY(query.link,actor)<0x3000){query.found=actor;query.distance=distance;}
    return nullptr;
}
fopAc_ac_c* nearby() {
    auto* link=daAlink_getAlinkActorClass();if(!safe(link)||fopAcIt_Judge(threat,link))return nullptr;
    Search query{link};fopAcIt_Judge(find,&query);return query.found;
}
bool valid() {
    auto* link=daAlink_getAlinkActorClass();auto* actor=fopAcM_SearchByID(g_context.id);
    return safe(link)&&actor&&g_context.slot==dComIfGs_getDataNum()&&g_context.stage==dComIfGp_getStartStageName()&&
        g_context.room==fopAcM_GetRoomNo(link)&&g_context.profile==fopAcM_GetName(actor)&&kind(actor)==g_context.kind&&
        actor->current.pos.abs(link->current.pos)<280&&!fopAcIt_Judge(threat,link);
}
const char* title(Kind value) {
    switch(value){case Kind::Malo:return "Malo";case Kind::Barnes:return "Barnes";case Kind::Armorer:return "Malo Mart";case Kind::Agitha:return "Agitha's Ball";case Kind::Jovani:return "Jovani";case Kind::Coro:return "Coro";case Kind::Sera:return "Sera";case Kind::Goron:return "Goron Craftsman";default:return "Hyrule";}
}
std::string body() {
    const char* text="";
    switch(g_context.kind){
    case Kind::Malo:text="A dependable shield is worth keeping. Sixty rupees buys the Hylian Shield.<br/>Your Hawkeye comes with the Bow. Bring crafting relics and Gilded Thread for better fletching.";break;
    case Kind::Barnes:text="Bring me Agitha's Deepwater Craft Relic and I'll enlarge your bomb reserves.<br/>Ancient runes and Gilded Thread let me make a harness that restores bombs faster.";break;
    case Kind::Armorer:text="Magic Armor costs 180 rupees. It carries a rechargeable ward and never spends your rupees.<br/>Jovani's Sigil strengthens its capacity. The Sunward Emblem and Mirror Heart unlock further weaves.";break;
    case Kind::Agitha:text="Every golden guest deserves a place at my ball! Bring their partners too.<br/>Each guest brings Gilded Thread for equipment. I reward the first pair, then three, six and twelve pairs.";break;
    case Kind::Jovani:text="Each lost soul you return loosens the curse.<br/>Gifts await at five, ten, twenty, forty and sixty souls.<br/>Speak about the curse at twenty and sixty to continue my story.";break;
    case Kind::Coro:text="Keep your lantern close. Its oil returns as you travel safely, and a rest stone restores your supplies.<br/>You won't need to buy fuel from me.";break;
    case Kind::Sera:text="Thank you for bringing my little friend home! Your Slingshot is my gift.<br/>Bring craft relics and Gilded Thread after your first great trial, and I'll stitch a traveling charm for you.";break;
    case Kind::Goron:text="Good equipment lasts, brother. Malo can fit you with a Hylian Shield.<br/>Bring Agitha's crafting relic to Barnes for larger bomb reserves. Your supplies return as you travel safely.";break;
    default:break;
    }
    std::string status=fmt::format("<br/><br/>Hylian rupees: {}",progression::model().rareRupees);
    if(merchants::enabled())status+=fmt::format("<br/>Craft relics: {}. Ancient runes: {}. Gilded Thread: {}.",progression::model().relics[0],progression::model().relics[1],progression::model().relics[2]);
    if(g_context.kind==Kind::Agitha)status+=fmt::format("<br/>Pairs welcomed: {}/12. New guests carried: {}.",progression::collections::pairs(),progression::collections::unpresented());
    if(g_context.kind==Kind::Jovani)status+=fmt::format("<br/>Poe souls returned: {}/60.",progression::collections::souls());
    return std::string(text)+status;
}
bool disabled(ModContext*,void* data) {
    if(!g_dialog||!valid())return true;
    const auto& c=*static_cast<Choice*>(data);
    switch(c.action){case Action::Purchase:return !rare::can_purchase(*c.offer);case Action::Trade:return !merchants::can_trade(*c.relic);case Action::Craft:return !guild::can_craft(*c.recipe);
    case Action::Present:return !progression::collections::unpresented();case Action::Claim:return !progression::collections::rewards_available(g_context.kind==Kind::Agitha);
    default:return false;}
}
void closed(ModContext*,UiDialogHandle,void*) { g_dialog=0; }
void pressed(ModContext*,UiDialogHandle,void* data) {
    if(disabled(nullptr,data))return;
    const auto c=*static_cast<Choice*>(data);
    switch(c.action){case Action::Purchase:rare::purchase(*c.offer);break;case Action::Trade:merchants::trade(*c.relic);break;
    case Action::Craft:guild::craft(*c.recipe);break;
    case Action::Present:progression::collections::present_bugs();break;case Action::Claim:progression::collections::claim_rewards(g_context.kind==Kind::Agitha);break;
    case Action::Native:g_native=g_context.id;g_nativeUntil=Clock::now()+std::chrono::seconds(15);closed(nullptr,0,nullptr);return;
    case Action::Leave:closed(nullptr,0,nullptr);return;}
    const auto text=body();svc_ui->dialog_set_body(mod_ctx,g_dialog,text.c_str());
}
bool open(fopAc_ac_c* actor) {
    if(g_dialog || (rare::enabled()&&!rare::ensure()) ||
        (progression::collections::enabled()&&!progression::collections::ensure()))return false;
    auto* link=daAlink_getAlinkActorClass();
    g_context={fopAcM_GetID(actor),fopAcM_GetName(actor),fopAcM_GetRoomNo(link),dComIfGs_getDataNum(),kind(actor),dComIfGp_getStartStageName()};
    if(!valid())return false;
    g_choices.clear();g_choices.reserve(10);
    std::vector<const char*> labels;
    const auto add=[&](Choice choice,const char* label){g_choices.push_back(choice);labels.push_back(label);};
    for(const auto& offer:rare::kOffers)if(rare::enabled()&&g_context.stage==offer.stage&&g_context.room==offer.room)add({Action::Purchase,&offer},offer.label);
    for(const auto& relic:merchants::kRelics)if(merchants::enabled()&&g_context.stage==relic.stage&&g_context.room==relic.room&&merchants::owns_relic(progression::model(),relic))add({Action::Trade,nullptr,&relic},relic.label);
    for(const auto& recipe:guild::kRecipes)if(merchants::enabled()&&g_context.stage==recipe.stage&&g_context.room==recipe.room)
        add({Action::Craft,nullptr,nullptr,&recipe},recipe.label);
    if(progression::collections::enabled()&&g_context.kind==Kind::Agitha)add({Action::Present},"Welcome the bugs I've found");
    if(progression::collections::enabled()&&(g_context.kind==Kind::Agitha||g_context.kind==Kind::Jovani))add({Action::Claim},"Receive my collection gifts");
    add({Action::Native},g_context.kind==Kind::Jovani?"Discuss the curse":"Talk about other matters");
    add({Action::Leave},"Farewell");
    std::vector<UiDialogAction> actions(g_choices.size(),UiDialogAction UI_DIALOG_ACTION_INIT);
    for(size_t i=0;i<actions.size();++i){auto& action=actions[i];action.label=labels[i];action.on_pressed=pressed;action.user_data=&g_choices[i];
        action.keep_open=g_choices[i].action!=Action::Native&&g_choices[i].action!=Action::Leave;action.is_disabled=disabled;}
    auto desc=UiDialogDesc UI_DIALOG_DESC_INIT;const auto text=body();desc.title=title(g_context.kind);desc.body_rml=text.c_str();
    desc.actions=actions.data();desc.action_count=actions.size();desc.on_dismiss=closed;
    if(svc_ui->dialog_push(mod_ctx,&desc,&g_dialog)!=MOD_OK){g_dialog=0;return false;}
    return true;
}
HookAction interact_pre(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(link!=daAlink_getAlinkActorClass()||g_dialog||!player::input_allowed())return HOOK_CONTINUE;
    const bool requested=mDoCPd_c::getTrigA(PAD_1)||(mDoCPd_c::getHoldLockL(PAD_1)&&mDoCPd_c::getTrigY(PAD_1));
    if(!requested)return HOOK_CONTINUE;
    auto* actor=nearby();if(!actor)return HOOK_CONTINUE;
    if(g_native==fopAcM_GetID(actor)&&Clock::now()<g_nativeUntil){g_native=fpcM_ERROR_PROCESS_ID_e;g_nativeUntil={};return HOOK_CONTINUE;}
    if(open(actor)){
        // Consume only the handled conversation buttons before native stick/
        // action dispatch. Native quests remain available through passthrough.
        mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags&=~(PAD_BUTTON_A|PAD_BUTTON_Y);
        link->mItemTrigger&=~(daAlink_c::BTN_A|daAlink_c::BTN_Y);
    }
    return HOOK_CONTINUE;
}
void reset(ModContext*,uint32_t,void*) {
    if(g_dialog)svc_ui->dialog_close(mod_ctx,g_dialog);
    g_dialog=0;g_native=fpcM_ERROR_PROCESS_ID_e;g_nativeUntil={};g_context={};g_choices.clear();
}
void hud_post(ModContext*,void*,void*,void*) {
    if(!player::input_allowed())return;
    auto* actor=nearby();if(!actor)return;
    auto* font=mDoExt_getSubFont();if(!font)font=mDoExt_getMesgFont();if(!font)return;
    const auto text=fmt::format("{}: {}",title(kind(actor)),g_native==fopAcM_GetID(actor)?"press A to continue the conversation":"A: speak / L + Y: gifts and equipment");
    hud::draw_prompt(text);

}
}
void tick() {
    if(g_dialog&&!valid())reset(nullptr,0,nullptr);
    if(g_dialog&&valid()){const auto text=body();svc_ui->dialog_set_body(mod_ctx,g_dialog,text.c_str());}
    if(Clock::now()>=g_nativeUntil)g_native=fpcM_ERROR_PROCESS_ID_e;
}
ModResult install() {
    HookOptions early=HOOK_OPTIONS_INIT;early.priority=20000;
    auto r=mods::hook::add_pre<hooks::StickData>(interact_pre,&early);
    if(r==MOD_OK)r=mods::hook::add_post<hooks::Shared1>(hud_post);
    if(r==MOD_OK)r=svc_save->observe_saves(mod_ctx,reset,reset,nullptr,nullptr,nullptr);
    return r;
}
}
