#include "progression/pause.hpp"
#include "progression/rewards.hpp"
#include "progression/growth.hpp"
#include "progression/collections.hpp"
#include "economy/merchant_model.hpp"
#include "economy/merchants.hpp"
#include "economy/guild_model.hpp"
#include "progression/treasure_model.hpp"
#include "combat/ward_model.hpp"
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
int g_page=0;
dMenu_Collect2D_c* g_owner=nullptr;
void reset(ModContext*,uint32_t,void*) { g_open=false; g_page=0; g_owner=nullptr; }
bool available(dMenu_Collect2D_c* menu) {
    bool ui=true;
    return menu && is_active() && ready() && menu->getSubWindowOpenCheck()==0 &&
        svc_ui->is_any_document_visible(mod_ctx,&ui)==MOD_OK && !ui;
}
HookAction wait_pre(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if (!available(menu)) { g_open=false; return HOOK_CONTINUE; }
    if (g_owner!=menu) { g_owner=menu; g_open=false;g_page=0; }
    if (mDoCPd_c::getTrigZ(PAD_1)) { g_open=!g_open;g_page=0; return HOOK_SKIP_ORIGINAL; }
    if (!g_open) return HOOK_CONTINUE;
    if(mDoCPd_c::getTrigRight(PAD_1)){g_page=(g_page+1)%4;return HOOK_SKIP_ORIGINAL;}
    if(mDoCPd_c::getTrigLeft(PAD_1)){g_page=(g_page+3)%4;return HOOK_SKIP_ORIGINAL;}
    if (mDoCPd_c::getTrigB(PAD_1)) g_open=false;
    else if (g_page==0&&mDoCPd_c::getTrigA(PAD_1)) growth::allocate(false);
    else if (g_page==0&&mDoCPd_c::getTrigX(PAD_1)) growth::allocate(true);
    return HOOK_SKIP_ORIGINAL;
}
HookAction close_pre(ModContext*,void*,void*,void*) { g_open=false; g_page=0; g_owner=nullptr; return HOOK_CONTINUE; }
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
        const auto line=[&](const std::string& text) { font->drawString_scale(38,y,12,15,text.c_str(),true); y+=17; };
        line(fmt::format("GROWTH & EQUIPMENT   {}/4   D-pad left / right: pages",g_page+1));
        line("B / Z: return   A / X: allocate on the growth page");
        if(g_page==0) {
            line(fmt::format("Health: {} hearts (+{})   Stamina: {} (+{})",3+m.healthPoints,m.healthPoints,cfg.staminaMax+m.stamina_bonus(),m.stamina_bonus()));
            line(fmt::format("Growth: {}/5 fragments   Unassigned blessings: {}",m.growthFragments%5,m.unassigned()));
            line("A: add one heart   X: add 20 stamina");
            line("Redistribute at Ordon Spring after Forest Temple.");
            line(fmt::format("Sacred Flask: {}/{} charges, {} hearts per charge",m.charges,m.capacity(cfg.flaskStartingCharges,cfg.flaskMaximumCharges),m.potency(cfg.flaskHealingHearts)));
            line(fmt::format("Flask shards: {}/4   Capacity upgrades: {}",m.flaskShards%4,m.capacity(cfg.flaskStartingCharges,cfg.flaskMaximumCharges)-std::min(cfg.flaskStartingCharges,cfg.flaskMaximumCharges)));
            line(fmt::format("Hylian Shield: {}   Hawkeye: {}",dComIfGs_isCollectShield(COLLECT_HYLIAN_SHIELD)?"owned":"not acquired",dComIfGs_getItem(SLOT_5,false)==dItemNo_HAWK_EYE_e?"owned":"obtain Bow"));
            line(fmt::format("Bomb capacities: {} / {} / {}",dComIfGs_getBombMax(dItemNo_NORMAL_BOMB_e),dComIfGs_getBombMax(dItemNo_WATER_BOMB_e),dComIfGs_getBombMax(dItemNo_POKE_BOMB_e)));
            line(fmt::format("Magic Armor: {}",dComIfGs_isCollectClothes(2)?"rechargeable ward; no currency cost":"not acquired"));
            if(dComIfGs_isCollectClothes(2))line(fmt::format("Ward: {}/{} charges   Damage reduction: {}%",m.wardCharges,combat::ward::capacity(m),int((1.0f-combat::ward::damage_scale(m))*100+0.5f)));
            line(fmt::format("Rare rupees: {}   Agitha: {}/12 pairs   Jovani: {}/60 souls",m.rareRupees,collections::pairs(),collections::souls()));
            if(m.equipmentUpgrades&treasures::kScoutStitch)line("Scout's Stitch: sprint uses 10% less stamina");
            if(m.equipmentUpgrades&treasures::kFletching)line("Bow Fletching: arrow refill interval reduced 25%");
            if(m.equipmentUpgrades&treasures::kBombHarness)line("Bomb Harness: bomb refill intervals reduced 25%");
        } else if(g_page==1) {
            line(fmt::format("Craft relics: {}   Ancient runes: {}   Gilded Thread: {}",m.relics[0],m.relics[1],m.relics[2]));
            line("Sera: Scout's Stitch. Malo: Bow Fletching. Barnes: Bomb Harness.");
            line("Malo Mart: Sigil, Sunward Weave, then Mirror Ward.");
            line(fmt::format("Sunward Emblem: {}   Mirror Heart: {}",m.claimed.contains(treasures::kSunwardRelic)?"found":"not found",m.claimed.contains(treasures::kMirrorRelic)?"found":"not found"));
            for(const auto& recipe:economy::guild::kRecipes) if(!recipe.honor) {
                line(recipe.label);
                line(m.equipmentUpgrades&recipe.benefit?"  Complete":economy::guild::eligible(m,recipe)?"  Components ready; bring to its artisan":"  Collect components and prior upgrades");
            }
            for(const auto& relic:economy::merchants::kRelics)
                if(economy::merchants::owns_relic(m,relic))line(fmt::format("{}: {}",relic.name,economy::merchants::redeemed(m,relic)?"complete":economy::merchants::reward_owned(relic)?"equipment owned":"ready to exchange"));
        } else if(g_page==2) {
            line("RELIC HONORS — optional keepsakes, no combat requirements");
            line(fmt::format("Craft relics: {}   Ancient runes: {}",m.relics[0],m.relics[1]));
            line("Malo preserves craft relics; Malo Mart restores ancient relics.");
            for(const auto& recipe:economy::guild::kRecipes)if(recipe.honor) {
                line(recipe.label);
                line(m.honors&recipe.honor?"  Honor earned":economy::guild::eligible(m,recipe)?"  Components ready; speak to the artisan":"  Collect components and prior honors");
            }
            line("Honors appear on the next page; original collection crests stay yours.");
        } else {
            line(fmt::format("Exploration treasures recorded: {}/{}",treasures::found(m),treasures::kFinds.size()));
            line("HONORS OF HYRULE");
            if(m.equipmentUpgrades&2)line("Agitha's Golden Crest: twelve pairs welcomed");
            if(m.equipmentUpgrades&4)line("Jovani's Spirit Crest: sixty souls returned");
            const char* names[]={"Wayfarer Crest: forty treasures","Archivist Crest: 160 treasures","STAR Champion","Snowpeak Champion","River Keeper","Cave of Ordeals Crest","Craft Crest","Ancient Crest","Hyrule Keeper Crest"};
            for(unsigned i=0;i<9;++i)if(m.honors&(1u<<i))line(names[i]);
            line("Original permanent side-quest prizes remain yours.");
            line("Relics are finite; revisiting a reward never creates another.");
        }
        if(m.equipmentUpgrades&2){J2DFillBox(526,32,20,20,JUtility::TColor(188,149,38,255));font->drawString_scale(531,48,12,15,"G",true);}
        if(m.equipmentUpgrades&4){J2DFillBox(552,32,20,20,JUtility::TColor(130,170,185,255));font->drawString_scale(557,48,12,15,"S",true);}

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
