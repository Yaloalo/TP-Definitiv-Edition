#include "progression/spring.hpp"
#include "progression/growth.hpp"
#include "progression/rewards.hpp"
#include "healing/rest_stones.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <cstring>
#include <fmt/format.h>
namespace remaster::progression::spring {
namespace {
UiDialogHandle g_dialog=0;
uint32_t g_slot=255;
bool here() {
    auto* link=daAlink_getAlinkActorClass();
    return growth::ensure() && link && dComIfGs_isCollectCrystal(0) &&
        std::strcmp(dComIfGp_getStartStageName(),"F_SP104")==0 && fopAcM_GetRoomNo(link)==1 &&
        healing::stones::rest_available(link);
}
bool valid_dialog() { return g_dialog && g_slot==dComIfGs_getDataNum() && here(); }
std::string description() {
    const auto& m=model();
    return fmt::format("Ordonna's blessing lets you reshape the strength you have earned.<br/><br/>"
        "Hearts: {} &nbsp; Stamina: {}<br/>One earned heart exchanges for 20 stamina.<br/>"
        "Your starting strength remains yours. There is no payment.",3+m.healthPoints,tuning::get().staminaMax+m.stamina_bonus());
}
bool exchange_disabled(ModContext*,void* data) {
    if (!valid_dialog()) return true;
    const bool toStamina=data!=nullptr;
    return toStamina ? !model().healthPoints || model().staminaPoints>=5 : !model().staminaPoints || model().healthPoints>=17;
}
void exchange_pressed(ModContext*,UiDialogHandle,void* data) {
    if (exchange_disabled(nullptr,data)) return;
    if (model().exchange(data!=nullptr)) {
        growth::apply_health(); persist(); const auto text=description();
        svc_ui->dialog_set_body(mod_ctx,g_dialog,text.c_str());
    }
}
void dismissed(ModContext*,UiDialogHandle,void*) { g_dialog=0; g_slot=255; }
void reset(ModContext*,uint32_t,void*) {
    if (g_dialog) svc_ui->dialog_close(mod_ctx,g_dialog);
    g_dialog=0; g_slot=255;
}
HookAction interact_pre(ModContext*,void* args,void* result,void*) {
    if (g_dialog || !player::input_allowed() || !mDoCPd_c::getHoldLockL(PAD_1) ||
        !mDoCPd_c::getTrigY(PAD_1)) return HOOK_CONTINUE;
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link || link!=daAlink_getAlinkActorClass())return HOOK_CONTINUE;
    // Validate rest safety without mistaking our own interaction edge for an item attack.
    const auto triggers=link->mItemTrigger;
    link->mItemTrigger&=~daAlink_c::BTN_Y;
    const bool available=here();
    link->mItemTrigger=triggers;
    if(!available)return HOOK_CONTINUE;
    UiDialogAction actions[3]={UI_DIALOG_ACTION_INIT,UI_DIALOG_ACTION_INIT,UI_DIALOG_ACTION_INIT};
    actions[0].label="One heart → 20 stamina"; actions[0].on_pressed=exchange_pressed;
    actions[0].user_data=&g_slot; actions[0].keep_open=true; actions[0].is_disabled=exchange_disabled;
    actions[1].label="20 stamina → one heart"; actions[1].on_pressed=exchange_pressed;
    actions[1].keep_open=true; actions[1].is_disabled=exchange_disabled;
    actions[2].label="Leave the blessing"; actions[2].on_pressed=dismissed;
    auto desc=UiDialogDesc UI_DIALOG_DESC_INIT; const auto text=description();
    desc.title="Ordonna's Blessing"; desc.body_rml=text.c_str(); desc.actions=actions;
    desc.action_count=3; desc.on_dismiss=dismissed;
    if (svc_ui->dialog_push(mod_ctx,&desc,&g_dialog)!=MOD_OK) { g_dialog=0; return HOOK_CONTINUE; }
    g_slot=dComIfGs_getDataNum(); link->mItemTrigger&=~daAlink_c::BTN_Y;
    mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags&=~PAD_BUTTON_Y;
    return HOOK_CONTINUE;
}
void hud_post(ModContext*,void*,void*,void*) {
    if (!player::input_allowed() || !here()) return;
    auto* font=mDoExt_getSubFont(); if (!font) font=mDoExt_getMesgFont(); if (!font) return;
    font->setGX(); font->setCharColor(JUtility::TColor(236,220,173,255));
    font->drawString_scale(26,175,12,15,"Ordonna's Blessing: hold L + press Y to exchange health/stamina",true);
    if (auto* port=dComIfGp_getCurrentGrafPort()) port->setup2D();
}
}
void tick() {
    if (g_dialog && !valid_dialog()) reset(nullptr,0,nullptr);
    if (!is_active() || !ready() || !player::input_allowed() || !dComIfGs_isCollectCrystal(0)) return;
    if (!model().claimed.contains("intro:growth-spring")) {
        UiToastDesc notice=UI_TOAST_DESC_INIT;
        notice.title_rml="Ordonna's Blessing";
        notice.body_rml="Return to the rest stone at Ordon Spring to exchange earned hearts and stamina. Hold L + press Y.";
        if (svc_ui->push_toast(mod_ctx,&notice)==MOD_OK) { model().claim("intro:growth-spring",0xff); persist(); }
    }
}
ModResult install() {
    HookOptions early=HOOK_OPTIONS_INIT; early.priority=20000;
    auto r=mods::hook::add_pre<hooks::StickData>(interact_pre,&early);
    if (r==MOD_OK) r=mods::hook::add_post<hooks::Shared1>(hud_post);
    if (r==MOD_OK) r=svc_save->observe_saves(mod_ctx,reset,reset,nullptr,nullptr,nullptr);
    return r;
}
}
