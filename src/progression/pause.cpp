#include "progression/pause.hpp"
#include "progression/rewards.hpp"
#include "progression/growth.hpp"
#include "economy/merchant_model.hpp"
#include "core/tuning.hpp"
#include "core/hooks.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include <fmt/format.h>
namespace remaster::progression::pause {
namespace {
DEFINE_HOOK(&dMenu_Collect2D_c::wait_proc, CollectWait);
DEFINE_HOOK(&dMenu_Collect2D_c::_delete, CollectDelete);
bool g_open=false;
dMenu_Collect2D_c* g_owner=nullptr;
void reset(ModContext*,uint32_t,void*) { g_open=false; g_owner=nullptr; }
bool available(dMenu_Collect2D_c* menu) {
    bool ui=true;
    return menu && is_active() && ready() && menu->getSubWindowOpenCheck()==0 &&
        svc_ui->is_any_document_visible(mod_ctx,&ui)==MOD_OK && !ui;
}
HookAction wait_pre(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if (!available(menu)) { g_open=false; return HOOK_CONTINUE; }
    if (g_owner!=menu) { g_owner=menu; g_open=false; }
    if (mDoCPd_c::getTrigZ(PAD_1)) { g_open=!g_open; return HOOK_SKIP_ORIGINAL; }
    if (!g_open) return HOOK_CONTINUE;
    if (mDoCPd_c::getTrigB(PAD_1)) g_open=false;
    else if (mDoCPd_c::getTrigA(PAD_1)) growth::allocate(false);
    else if (mDoCPd_c::getTrigX(PAD_1)) growth::allocate(true);
    return HOOK_SKIP_ORIGINAL;
}
HookAction close_pre(ModContext*,void*,void*,void*) { g_open=false; g_owner=nullptr; return HOOK_CONTINUE; }
void draw_post(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if (!available(menu) || !growth::ensure()) return;
    auto* port=dComIfGp_getCurrentGrafPort(); auto* font=mDoExt_getSubFont();
    if (!font) font=mDoExt_getMesgFont(); if (!port || !font) return;
    const auto& m=model(); const auto& cfg=tuning::get();
    port->setup2D();
    if (g_open && g_owner==menu) {
        J2DFillBox(24,24,560,376,JUtility::TColor(20,22,26,245));
        font->setGX(); font->setCharColor(JUtility::TColor(236,220,173,255));
        float y=49;
        const auto line=[&](const std::string& text) { font->drawString_scale(38,y,12,15,text.c_str(),true); y+=19; };
        line("GROWTH & EQUIPMENT");
        line(fmt::format("Health: {} hearts (+{})   Stamina: {} (+{})",3+m.healthPoints,m.healthPoints,cfg.staminaMax+m.stamina_bonus(),m.stamina_bonus()));
        line(fmt::format("Growth: {}/5 fragments   Unassigned blessings: {}",m.growthFragments%5,m.unassigned()));
        line("A: add one heart   X: add 20 stamina   B / Z: return");
        line("Redistribute blessings at a Light Spirit spring after Forest Temple.");
        line(fmt::format("Sacred Flask: {}/{} charges, {} hearts per charge",m.charges,m.capacity(cfg.flaskStartingCharges,cfg.flaskMaximumCharges),m.potency(cfg.flaskHealingHearts)));
        line(fmt::format("Flask shards: {}/4   Capacity upgrades: {}",m.flaskShards%4,m.flaskShards/4));
        line(fmt::format("Hylian Shield: {}   Hawkeye: {}",dComIfGs_isCollectShield(COLLECT_HYLIAN_SHIELD)?"owned":"not acquired",dComIfGs_getItem(SLOT_5,false)==dItemNo_HAWK_EYE_e?"owned":"obtain Bow"));
        line(fmt::format("Bomb capacities: {} / {} / {}",dComIfGs_getBombMax(dItemNo_NORMAL_BOMB_e),dComIfGs_getBombMax(dItemNo_WATER_BOMB_e),dComIfGs_getBombMax(dItemNo_POKE_BOMB_e)));
        line(fmt::format("Magic Armor: {}",dComIfGs_isCollectClothes(2)?"owned; protection uses stamina":"not acquired"));
        line(fmt::format("Rare rupees: {}",m.rareRupees));
        for (const auto& relic:economy::merchants::kRelics)
            if (economy::merchants::owns_relic(m,relic)) line(fmt::format("{}: {}",relic.name,economy::merchants::redeemed(m,relic)?"upgrade complete":"ready to exchange"));
        if (m.equipmentUpgrades & 1) line("Twilight Sigil: improved Magic Armor efficiency");
        if (m.equipmentUpgrades & 2) line("Agitha's Golden Crest: honored guest");
        if (m.equipmentUpgrades & 4) line("Jovani's Spirit Crest: curse lifted");
    } else {
        J2DFillBox(28,24,552,33,JUtility::TColor(20,22,26,205));
        font->setGX(); font->setCharColor(JUtility::TColor(236,220,173,255));
        const auto text=fmt::format("{} hearts | Stamina {} | Flask {}/{} | Z: Growth & Equipment",
            3+m.healthPoints,cfg.staminaMax+m.stamina_bonus(),m.charges,m.capacity(cfg.flaskStartingCharges,cfg.flaskMaximumCharges));
        font->drawString_scale(36,45,11,14,text.c_str(),true);
    }
    port->setup2D();
}
}
ModResult install() {
    auto r=mods::hook::add_pre<CollectWait>(wait_pre);
    if (r==MOD_OK) r=mods::hook::add_pre<CollectDelete>(close_pre);
    HookOptions late=HOOK_OPTIONS_INIT; late.priority=-20000;
    if (r==MOD_OK) r=mods::hook::add_post<hooks::CollectDraw>(draw_post,&late);
    if (r==MOD_OK) r=svc_save->observe_saves(mod_ctx,reset,reset,nullptr,nullptr,nullptr);
    return r;
}
}
