// Read-only evidence for transitions that remain black while gameplay advances.
// Never forces fades, scene completion, camera state or story flags.
#include "pacing/transition_diagnostics.hpp"
#include "core/hooks.hpp"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_overlap_mng.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "JSystem/JUtility/JUTFader.h"
#include "m_Do/m_Do_graphic.h"
#include <chrono>
#include <fmt/format.h>
#include <string>

namespace remaster::pacing::diagnostics {
namespace {
DEFINE_HOOK_SYMBOL("dOvlpFd_FadeIn", int(void*), FadeToBlack);
DEFINE_HOOK_SYMBOL("dOvlpFd_Wait", int(void*), WaitForScene);
DEFINE_HOOK_SYMBOL("dOvlpFd_FadeOut", int(void*), RevealScene);
std::string g_last;
std::chrono::steady_clock::time_point g_reported{};
void record(const char* phase) {
    if (!is_active()) return;
    auto* display=JFWDisplay::getManager();
    auto* fader=display?display->getFader():nullptr;
    auto* link=daAlink_getAlinkActorClass();
    const bool pending=fopOvlpM_IsDoingReq()!=0 || dComIfGp_isEnableNextStage();
    const std::string state=fmt::format(
        "{} stage={} room={} next={}:{} pending={} windows={} fader={} fade={} event={}",
        phase,dComIfGp_getStartStageName(),link?fopAcM_GetRoomNo(link):-1,
        dComIfGp_getNextStageName(),dComIfGp_getNextStageRoomNo(),pending,
        dComIfGp_getWindowNum(),fader?fader->getStatus():-1,
        int(mDoGph_gInf_c::isFade()),bool(dComIfGp_event_runCheck()));
    const auto now=std::chrono::steady_clock::now();
    if (state!=g_last || (pending && now-g_reported>std::chrono::seconds(5))) {
        mods::log::info("[Transition] {} fader_timer={}/{} alpha={} color_fade={:.3f} speed={:.3f} color_alpha={}",state,
            fader?fader->mTimer:0,fader?fader->mDuration:0,fader?int(fader->mColor.a):0,
            mDoGph_gInf_c::getFadeRate(),mDoGph_gInf_c::getFadeSpeed(),int(mDoGph_gInf_c::getFadeColor().a));
        g_last=state; g_reported=now;
    }
}
void fading(ModContext*,void*,void*,void*) { record("fade-to-black"); }
void waiting(ModContext*,void*,void*,void*) { record("waiting-for-scene"); }
void revealing(ModContext*,void*,void*,void*) { record("reveal-scene"); }
void gameplay(ModContext*,void*,void*,void*) {
    // The overlay callbacks provide phase-specific records while loading.
    if (!fopOvlpM_IsDoingReq()) record("gameplay");
}
}
ModResult install() {
    auto r=mods::hook::add_post<FadeToBlack>(fading);
    if(r==MOD_OK)r=mods::hook::add_post<WaitForScene>(waiting);
    if(r==MOD_OK)r=mods::hook::add_post<RevealScene>(revealing);
    if(r==MOD_OK)r=mods::hook::add_post<hooks::LinkExecute>(gameplay);
    return r;
}
}
