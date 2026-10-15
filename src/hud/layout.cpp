#include "hud/layout.hpp"
#include "core/hooks.hpp"
#include "core/tuning.hpp"
#include "d/d_pane_class.h"
#include "d/d_meter2.h"
#include "d/d_meter2_info.h"
#include "m_Do/m_Do_graphic.h"
#include <array>
#include <algorithm>

namespace remaster::hud::layout {
namespace {
DEFINE_HOOK(&dMeterMap_c::getMapDispEdgeBottomY_Layout, MapBottom);
struct Snapshot {
    CPaneMgr* pane = nullptr;
    float x=0, y=0, sx=1, sy=1;
};
std::array<Snapshot,3> g_saved{};
dMeter2Draw_c* g_meter=nullptr;

void restore() {
    for (auto& s:g_saved) {
        if (s.pane) { s.pane->scale(s.sx,s.sy); s.pane->translate(s.x,s.y); }
        s={};
    }
}
void place(CPaneMgr* pane, Snapshot& saved, float scale, bool right, bool bottom, float targetY) {
    if (!pane || !pane->getPanePtr()) return;
    saved={pane,pane->getTranslateX(),pane->getTranslateY(),pane->getScaleX(),pane->getScaleY()};
    Mtx before, after;
    const u8 corner=(right?1:0)+(bottom?2:0);
    const Vec old=pane->getGlobalVtx(&before,corner,false,0);
    pane->scale(saved.sx*scale,saved.sy*scale);
    const Vec now=pane->getGlobalVtx(&after,corner,false,0);
    // These native HUD groups are unrotated. Convert screen displacement to
    // parent coordinates, preserving each group's original screen corner.
    const float px=std::abs(saved.sx)>0.001f?before[0][0]/saved.sx:1;
    const float py=std::abs(saved.sy)>0.001f?before[1][1]/saved.sy:1;
    if (std::abs(px)>0.001f && std::abs(py)>0.001f)
        pane->translate(saved.x+(old.x-now.x)/px,
            saved.y+((targetY<0?old.y:targetY)-now.y)/py);
}
HookAction meter_pre(ModContext*,void* args,void*,void*) {
    g_meter=is_active()?mods::arg<dMeter2Draw_c*>(args,0):nullptr;
    return HOOK_CONTINUE;
}
HookAction screen_pre(ModContext*,void* args,void*,void*) {
    if (!g_meter || mods::arg<J2DScreen*>(args,0)!=g_meter->mpScreen || g_saved[0].pane) return HOOK_CONTINUE;
    const float scale=std::clamp<int64_t>(tuning::get().hudPromptScalePercent,50,100)/100.0f;
    place(g_meter->mpButtonParent,g_saved[0],scale,true,false,-1);
    // Keep map controls together above the bottom-left minimap, including in
    // tall windows where the native fixed 448-line layout drifts to mid-screen.
    auto* meter=dMeter2Info_getMeterClass();
    const float mapTop=meter && meter->mpMap ? meter->mpMap->getMapDispEdgeTop() : mDoGph_gInf_c::getSafeMaxYF()-133;
    place(g_meter->mpButtonCrossParent,g_saved[1],scale,false,true,mapTop-12);
    place(g_meter->mpRupeeKeyParent,g_saved[2],scale,true,true,-1);
    return HOOK_CONTINUE;
}
void meter_post(ModContext*,void*,void*,void*) { restore(); g_meter=nullptr; }
HookAction map_bottom(ModContext*,void*,void* result,void*) {
    if (!is_active()) return HOOK_CONTINUE;
    *static_cast<f32*>(result)=mDoGph_gInf_c::getSafeMaxYF()-22;
    return HOOK_SKIP_ORIGINAL;
}
}
ModResult install() {
    HookOptions early=HOOK_OPTIONS_INIT,late=HOOK_OPTIONS_INIT;
    early.priority=15000; late.priority=-15000;
    auto r=mods::hook::add_pre<hooks::Shared1>(meter_pre,&early);
    if(r==MOD_OK)r=mods::hook::add_pre<hooks::ScreenDraw>(screen_pre,&late);
    if(r==MOD_OK)r=mods::hook::add_post<hooks::Shared1>(meter_post,&late);
    if(r==MOD_OK)r=mods::hook::add_pre<MapBottom>(map_bottom);
    return r;
}
}
