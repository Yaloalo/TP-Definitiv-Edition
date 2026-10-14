#include "combat/ward.hpp"
#include "combat/ward_model.hpp"
#include "progression/rewards.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_iter.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <string>
#include <fmt/format.h>
namespace remaster::combat::ward {
namespace {
resources::Recharge g_clock;
daAlink_c* g_owner=nullptr;
std::string g_stage;
int g_room=-1,g_lastLife=-1;
bool g_retry=false;
uint32_t g_retrySlot=255;
bool ensure() {
    if(!enabled()||!progression::ready()||!dComIfGs_isCollectClothes(2))return false;
    if(initialize(progression::model()))progression::persist();return true;
}
void clear_clock() {g_clock={};g_owner=nullptr;g_stage.clear();g_room=g_lastLife=-1;}
void new_save(ModContext*,uint32_t,void*) {reset();}
void loaded(ModContext*,uint32_t slot,void*) {if(slot!=g_retrySlot)g_retry=false;clear_clock();}
void* threat(void* ptr,void* data) {
    auto* actor=static_cast<fopAc_ac_c*>(ptr);auto* link=static_cast<daAlink_c*>(data);
    return actor&&actor->health>0&&fopAcM_GetGroup(actor)==fopAc_ENEMY_e&&
        actor->current.pos.abs(link->current.pos)<3000?actor:nullptr;
}
void hud_post(ModContext*,void*,void*,void*) {
    auto* link=daAlink_getAlinkActorClass();
    if(!enabled()||!progression::ready()||!link||!link->checkMagicArmorWearAbility()||!player::input_allowed())return;
    auto* font=mDoExt_getSubFont();if(!font)font=mDoExt_getMesgFont();if(!font)return;
    const auto& m=progression::model();const auto text=fmt::format("Ward {}/{}",m.wardCharges,capacity(m));
    font->setGX();font->setCharColor(JUtility::TColor(150,190,228,255));
    font->drawString_scale(26,127,12,15,text.c_str(),true);
    if(auto* port=dComIfGp_getCurrentGrafPort())port->setup2D();
}
}
bool enabled() {return is_active()&&tuning::get().magicWard;}
void reset() {clear_clock();g_retry=false;g_retrySlot=255;}
void damage_received() {g_clock.hit();}
float absorb(daAlink_c* link) {
    if(!link||!link->checkMagicArmorWearAbility()||!ensure())return 1.0f;
    auto& m=progression::model();if(!spend(m))return 1.0f;
    progression::persist();return damage_scale(m);
}
void refill() {
    if(!ensure())return;
    auto& m=progression::model();m.wardCharges=capacity(m);clear_clock();progression::persist();
}
void death() {if(enabled()){g_retry=true;g_retrySlot=dComIfGs_getDataNum();g_clock.hit();}}
void boss_victory(const char* stage) {
    if(!stage||std::string_view(stage).substr(0,4)!="D_MN"||!ensure())return;
    const auto name=std::string("ward:victory:")+std::string(stage).substr(0,6);
    if(progression::model().claim(name.c_str(),0xff))refill();
}
void tick(daAlink_c* link) {
    if(!link||!ensure()){clear_clock();return;}
    const std::string stage=dComIfGp_getStartStageName();const auto room=fopAcM_GetRoomNo(link);
    if(link!=g_owner||stage!=g_stage||room!=g_room){clear_clock();g_owner=link;g_stage=stage;g_room=room;}
    if(!player::input_allowed()||link->mProcID==daAlink_c::PROC_DEAD||dComIfGs_getLife()==0){g_clock.hit();return;}
    const int life=dComIfGs_getLife();if(g_lastLife>=0&&life<g_lastLife)damage_received();g_lastLife=life;
    if(g_retry){g_retry=false;if(g_retrySlot==dComIfGs_getDataNum())refill();}
    if(fopAcIt_Judge(threat,link)){g_clock.hit();return;}
    auto& m=progression::model();const auto& cfg=tuning::get();
    const auto gained=g_clock.tick(m.wardCharges,capacity(m),resources::ticks_for_ms(recovery_time(m,cfg.wardRecoveryDelayMs)),resources::ticks_for_ms(recovery_time(m,cfg.wardRecoveryIntervalMs)));
    if(gained){m.wardCharges+=gained;progression::persist();}
}
ModResult install() {
    auto r=mods::hook::add_post<hooks::Shared1>(hud_post);
    if(r==MOD_OK)r=svc_save->observe_saves(mod_ctx,new_save,loaded,nullptr,nullptr,nullptr);
    return r;
}
}
