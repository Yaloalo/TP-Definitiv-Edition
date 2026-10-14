// Vertex-colour edge rendering uses the native GX 2D path (also used by
// Dawnlight integration/dusklight_edges.inc). No framebuffer sampling,
// slow-motion, camera shake or damage numbers are introduced.
#include "hud/damage_feedback.hpp"
#include "hud/feedback_model.hpp"
#include "core/hooks.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_graphic.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <chrono>
#include <string>
namespace remaster::hud::damage_feedback {
namespace {
DamagePulse g_pulse;
daAlink_c* g_owner=nullptr;
std::string g_stage;
int g_room=-1,g_life=-1;
double seconds() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
void save_reset(ModContext*,uint32_t,void*) { reset(); }
HookAction draw_pre(ModContext*,void*,void*,void*) {
    if (!is_active() || !tuning::get().damageFeedback || !player::input_allowed()) {
        g_pulse={}; return HOOK_CONTINUE;
    }
    const auto& v=tuning::get();
    const float alpha=g_pulse.opacity(seconds(),double(std::clamp<int64_t>(v.damageFeedbackDurationMs,100,60000))/1000,
        float(std::clamp<int64_t>(v.damageFeedbackOpacityPercent,0,60))/100);
    if (alpha<=0) return HOOK_CONTINUE;
    auto* port=dComIfGp_getCurrentGrafPort(); if (!port) return HOOK_CONTINUE;
    const float left=mDoGph_gInf_c::getMinXF(),right=mDoGph_gInf_c::getMaxXF();
    const float top=mDoGph_gInf_c::getMinYF(),bottom=mDoGph_gInf_c::getMaxYF();
    const float width=std::min((bottom-top)*0.09f,(right-left)*0.25f);
    port->setup2D();
    GXClearVtxDesc(); GXSetVtxDesc(GX_VA_POS,GX_DIRECT); GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
    GXSetNumChans(1); GXSetNumTexGens(0); GXSetNumTevStages(1);
    GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR); GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
    GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_NOOP);
    GXSetZMode(GX_FALSE,GX_ALWAYS,GX_FALSE); GXSetCullMode(GX_CULL_NONE);
    const u8 a=u8(alpha*255);
    const auto vertex=[](float x,float y,u8 opacity) { GXPosition3f32(x,y,0); GXColor4u8(130,22,18,opacity); };
    const float x0=left+width,x1=right-width,y0=top+width,y1=bottom-width;
    GXBegin(GX_QUADS,GX_VTXFMT0,16);
    vertex(left,top,a); vertex(right,top,a); vertex(x1,y0,0); vertex(x0,y0,0);
    vertex(right,top,a); vertex(right,bottom,a); vertex(x1,y1,0); vertex(x1,y0,0);
    vertex(right,bottom,a); vertex(left,bottom,a); vertex(x0,y1,0); vertex(x1,y1,0);
    vertex(left,bottom,a); vertex(left,top,a); vertex(x0,y0,0); vertex(x0,y1,0);
    GXEnd(); port->setup2D();
    return HOOK_CONTINUE;
}
}
void reset() { g_pulse={}; g_owner=nullptr; g_stage.clear(); g_room=g_life=-1; }
void tick(daAlink_c* link) {
    if (!is_active() || !link || !tuning::get().damageFeedback) { reset(); return; }
    const char* stage=dComIfGp_getStartStageName(); const int room=fopAcM_GetRoomNo(link),life=dComIfGs_getLife();
    if (link!=g_owner || g_stage!=stage || g_room!=room || life<=0 || !player::input_allowed()) g_pulse={};
    else if (g_life>=0 && life<g_life && life>0) g_pulse.hit(seconds());
    g_owner=link; g_stage=stage; g_room=room; g_life=life;
}
ModResult install() {
    HookOptions early=HOOK_OPTIONS_INIT; early.priority=20000;
    // Draw before the native HUD so hearts, item counters and prompts stay crisp.
    auto result=mods::hook::add_pre<hooks::Shared1>(draw_pre,&early);
    if (result==MOD_OK) result=svc_save->observe_saves(mod_ctx,save_reset,save_reset,nullptr,nullptr,nullptr);
    return result;
}
}
