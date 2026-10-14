// Scoped pane-alpha snapshots adapted from Dawnlight src/hud_fade.cpp.
// Shared native targets cooperate with our controller UI, compass and stamina
// meter. Only HUD draw scopes are faded; inventory/menu screens stay unchanged.
#include "hud/idle_fade.hpp"
#include "hud/feedback_model.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "combat/stamina.hpp"
#include "healing/wolf.hpp"
#include "healing/rest_stones.hpp"
#include "progression/rewards.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_meter2.h"
#include "f_op/f_op_actor_iter.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "m_Do/m_Do_controller_pad.h"
#include <chrono>
#include <vector>
namespace remaster::hud::idle_fade {
namespace {
DEFINE_HOOK(static_cast<void(J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw), PictureDraw);
DEFINE_HOOK(&dMeter2_c::_delete, MeterDelete);
IdleEnvelope g_envelope;
daAlink_c* g_owner=nullptr;
cXyz g_position;
s16 g_angle=0;
int g_life=-1,g_charges=-1;
bool g_idle=false;
float g_alpha=1;
unsigned g_depth=0;
std::vector<void*> g_calls;
struct PaneAlpha { J2DPane* pane; u8 alpha,colorAlpha; bool changed; };
struct Frame { void* call; std::vector<PaneAlpha> panes; };
std::vector<Frame> g_screens,g_pictures;
double seconds() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void* nearby_enemy(void* ptr,void* data) {
    auto* actor=static_cast<fopAc_ac_c*>(ptr); auto* link=static_cast<daAlink_c*>(data);
    return actor && fopAcM_GetGroup(actor)==fopAc_ENEMY_e && actor->health>0 &&
        actor->current.pos.abs(link->current.pos)<3000 ? actor : nullptr;
}
bool idle_proc(u16 proc) {
    return proc==daAlink_c::PROC_WAIT || proc==daAlink_c::PROC_SERVICE_WAIT || proc==daAlink_c::PROC_TIRED_WAIT ||
        proc==daAlink_c::PROC_WOLF_WAIT || proc==daAlink_c::PROC_WOLF_SERVICE_WAIT || proc==daAlink_c::PROC_WOLF_TIRED_WAIT;
}
void fade_tree(J2DPane* pane,float alpha,std::vector<PaneAlpha>& saved,bool root=true) {
    if (!pane) return;
    const bool changed=root || !pane->isInfluencedAlpha();
    saved.push_back({pane,pane->getAlpha(),pane->mColorAlpha,changed});
    if (changed) pane->setAlpha(u8(pane->getAlpha()*alpha));
    for (auto* child=pane->getFirstChildPane();child;child=child->getNextChildPane()) fade_tree(child,alpha,saved,false);
}
void restore_frame(Frame& frame) {
    for (auto& saved:frame.panes) {
        if (saved.changed) saved.pane->setAlpha(saved.alpha);
        saved.pane->mColorAlpha=saved.colorAlpha;
    }
}
void restore(std::vector<Frame>& stack,void* call) {
    if (stack.empty() || stack.back().call!=call) return;
    restore_frame(stack.back()); stack.pop_back();
}
HookAction hud_pre(ModContext*,void* args,void*,void*) {
    g_calls.push_back(args);
    if (g_depth++==0) {
        const auto& v=tuning::get();
        const bool enabled=is_active() && v.idleHudFade && daAlink_getAlinkActorClass() &&
            player::input_allowed() && !dComIfGp_isEnableNextStage();
        g_alpha=g_envelope.update(seconds(),enabled,g_idle,
            double(std::clamp<int64_t>(v.hudIdleDelayMs,0,60000))/1000,
            double(std::clamp<int64_t>(v.hudFadeDurationMs,100,60000))/1000,
            float(std::clamp<int64_t>(v.hudIdleOpacityPercent,0,75))/100);
    }
    return HOOK_CONTINUE;
}
void hud_post(ModContext*,void* args,void*,void*) {
    if (g_calls.empty() || g_calls.back()!=args) return;
    g_calls.pop_back(); if (g_depth) --g_depth;
}
HookAction screen_pre(ModContext*,void* args,void*,void*) {
    g_screens.push_back({args,{}});
    if (g_depth && opacity()<1) fade_tree(mods::arg<J2DScreen*>(args,0),opacity(),g_screens.back().panes);
    return HOOK_CONTINUE;
}
void screen_post(ModContext*,void* args,void*,void*) { restore(g_screens,args); }
HookAction picture_pre(ModContext*,void* args,void*,void*) {
    g_pictures.push_back({args,{}});
    // Pictures inside a screen already inherit its alpha; never multiply twice.
    if (g_depth && g_screens.empty() && opacity()<1) {
        auto* pane=mods::arg<J2DPicture*>(args,0);
        if (pane) {
            g_pictures.back().panes.push_back({pane,pane->getAlpha(),pane->mColorAlpha,true});
            pane->setAlpha(u8(pane->getAlpha()*opacity()));
        }
    }
    return HOOK_CONTINUE;
}
void picture_post(ModContext*,void* args,void*,void*) { restore(g_pictures,args); }
HookAction delete_pre(ModContext*,void*,void*,void*) { reset(); return HOOK_CONTINUE; }
void save_reset(ModContext*,uint32_t,void*) { reset(); }
}
float opacity() { return is_active() && tuning::get().idleHudFade ? g_alpha : 1; }
void tick(daAlink_c* link) {
    if (!is_active() || !link || !player::input_allowed()) { g_idle=false; g_owner=nullptr; g_alpha=1; return; }
    const int life=dComIfGs_getLife();
    const int charges=progression::ready()?progression::model().charges:-1;
    const auto stamina=combat::stamina::state();
    const bool moved=link!=g_owner || (link->current.pos-g_position).abs2()>0.01f || link->current.angle.y!=g_angle;
    g_idle=!moved && life==g_life && charges==g_charges && idle_proc(link->mProcID) &&
        !mDoCPd_c::getHold(PAD_1) && !link->mItemTrigger && !link->checkAttentionLock() && !link->mTargetedActor &&
        !healing::wolf::busy() && !healing::stones::busy() && life*4>dComIfGs_getMaxLifeGauge() &&
        (!tuning::get().flaskEnabled || charges>0) &&
        (!tuning::get().staminaEnabled || (!stamina.exhausted && stamina.current>=stamina.maximum)) &&
        !fopAcIt_Judge(nearby_enemy,link);
    g_owner=link; g_position=link->current.pos; g_angle=link->current.angle.y; g_life=life; g_charges=charges;
    if (!g_idle) { g_alpha=1; g_envelope={}; }
}
void reset() {
    while (!g_pictures.empty()) { restore_frame(g_pictures.back()); g_pictures.pop_back(); }
    while (!g_screens.empty()) { restore_frame(g_screens.back()); g_screens.pop_back(); }
    g_calls.clear(); g_depth=0; g_envelope={}; g_owner=nullptr; g_idle=false; g_alpha=1; g_life=g_charges=-1;
}
ModResult install() {
    HookOptions first=HOOK_OPTIONS_INIT,last=HOOK_OPTIONS_INIT; first.priority=10000; last.priority=-10000;
    auto r=mods::hook::add_pre<hooks::Shared1>(hud_pre,&first);
    if (r==MOD_OK) r=mods::hook::add_post<hooks::Shared1>(hud_post,&last);
    if (r==MOD_OK) r=mods::hook::add_pre<hooks::MapDraw>(hud_pre,&first);
    if (r==MOD_OK) r=mods::hook::add_post<hooks::MapDraw>(hud_post,&last);
    if (r==MOD_OK) r=mods::hook::add_pre<hooks::ButtonDraw>(hud_pre,&first);
    if (r==MOD_OK) r=mods::hook::add_post<hooks::ButtonDraw>(hud_post,&last);
    if (r==MOD_OK) r=mods::hook::add_pre<hooks::ScreenDraw>(screen_pre,&last);
    if (r==MOD_OK) r=mods::hook::add_post<hooks::ScreenDraw>(screen_post,&first);
    if (r==MOD_OK) r=mods::hook::add_pre<PictureDraw>(picture_pre,&last);
    if (r==MOD_OK) r=mods::hook::add_post<PictureDraw>(picture_post,&first);
    if (r==MOD_OK) r=mods::hook::add_pre<MeterDelete>(delete_pre);
    if (r==MOD_OK) r=svc_save->observe_saves(mod_ctx,save_reset,save_reset,nullptr,nullptr,nullptr);
    return r;
}
}
