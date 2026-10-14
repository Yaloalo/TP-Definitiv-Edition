// Sacred Rest Stones reuse WStoneF's native model (d_a_obj_smw_stone.cpp), without
// its collision/howling/event actor. Actor heap/resource lifecycle follows the native
// actor pattern and Dawnlight's boss_portal_mirrors.cpp; no donor gameplay is loaded.
#include "healing/rest_stones.hpp"
#include "healing/rest_sites.hpp"
#include "healing/rest_model.hpp"
#include "healing/flask.hpp"
#include "hud/idle_fade.hpp"
#include "healing/wolf.hpp"
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "core/hooks.hpp"
#include "progression/rewards.hpp"
#include "resources/recharge_model.hpp"
#include "services.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_bg_s_gnd_chk.h"
#include "d/d_bg_s_lin_chk.h"
#include "d/d_bg_s_spl_grp_chk.h"
#include "f_op/f_op_actor_iter.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include <cmath>
#include <cstring>
#include <fmt/format.h>
namespace remaster::healing::stones {
namespace {
constexpr char kArchive[] = "WStoneF";
ProfileName g_profile = -1;
ActorHandle g_registration = 0;
std::array<ActorId,kSites.size()> g_ids{};
std::array<int,kSites.size()> g_retry{};
uint32_t g_generation = 1;
RestTimers g_timers;
daAlink_c* g_owner = nullptr;
int g_lastLife = -1, g_room = -1, g_nearest = -1, g_channelSite = -1, g_requested = -1;
std::string g_stage, g_prompt;
bool g_menuChannel = false;
bool enabled() { return is_active() && tuning::get().flaskEnabled && progression::ready(); }
bool markers_enabled() { return enabled() && tuning::get().restStones; }
struct RestStone : fopAc_ac_c {
    request_of_phase_process_class phase{};
    J3DModel* model = nullptr;
    bool ready = false;
    ~RestStone() { dComIfG_resDelete(&phase,kArchive); }
    static int heap(fopAc_ac_c* actor) {
        auto* self = static_cast<RestStone*>(actor);
        auto* data = static_cast<J3DModelData*>(dComIfG_getObjectRes(kArchive,4));
        if (!data) return 0;
        self->model = mDoExt_J3DModel__create(data,0x80000,0x11000084);
        return self->model != nullptr;
    }
};
bool own_actor(fopAc_ac_c* actor, size_t index) {
    return actor && fopAcM_GetName(actor) == g_profile && (fopAcM_GetParam(actor) & 255) == index;
}
RestStone* stone(size_t index) {
    if (!g_ids[index] || fpcM_IsCreating(g_ids[index])) return nullptr;
    auto* actor = fopAcM_SearchByID(g_ids[index]);
    if (!own_actor(actor,index) || fopAcM_GetParam(actor) >> 8 != g_generation) return nullptr;
    auto* result = static_cast<RestStone*>(actor);
    return result->ready ? result : nullptr;
}
int create_stone(void* ptr) {
    auto* self = static_cast<RestStone*>(ptr);
    fopAcM_ct(self,RestStone);
    if (!markers_enabled() || fopAcM_GetParam(self) >> 8 != g_generation) return cPhs_ERROR_e;
    if (self->ready) return cPhs_COMPLEATE_e;
    const auto phase = dComIfG_resLoad(&self->phase,kArchive);
    if (phase != cPhs_COMPLEATE_e) return phase;
    if (!fopAcM_entrySolidHeap(self,RestStone::heap,0x6000)) return cPhs_ERROR_e;
    mDoMtx_stack_c::transS(self->current.pos);
    mDoMtx_stack_c::YrotM(self->shape_angle.y);
    self->model->setBaseTRMtx(mDoMtx_stack_c::get());
    self->model->setBaseScale(cXyz(0.65f,0.65f,0.65f));
    self->attention_info.flags = 0;
    fopAcM_SetMtx(self,self->model->getBaseTRMtx());
    self->ready = true;
    return cPhs_COMPLEATE_e;
}
int delete_stone(void* ptr) { static_cast<RestStone*>(ptr)->~RestStone(); return 1; }
int execute_stone(void*) { return 1; }
int can_delete_stone(void*) { return 1; }
int draw_stone(void* ptr) {
    auto* self = static_cast<RestStone*>(ptr);
    if (!markers_enabled() || !self->ready || fopAcM_GetParam(self) >> 8 != g_generation) return 1;
    g_env_light.settingTevStruct(8,&self->current.pos,&self->tevStr);
    g_env_light.setLightTevColorType_MAJI(self->model,&self->tevStr);
    dComIfGd_setListBG(); mDoExt_modelUpdateDL(self->model); dComIfGd_setList();
    return 1;
}
bool floor_position(const cXyz& anchor, cXyz& out) {
    // Fixed offsets keep the stone off the NPC/door/player spawn itself. Try alternatives
    // rather than invent a placement when the nearby terrain is submerged or obstructed.
    constexpr float offsets[][2] = {{220,0},{-220,0},{0,220},{0,-220},{160,160},{-160,-160}};
    for (const auto& offset : offsets) {
        cXyz probe(anchor.x+offset[0],anchor.y+180,anchor.z+offset[1]);
        dBgS_ObjGndChk ground; ground.SetPos(&probe);
        const float y = dComIfG_Bgsp().GroundCross(&ground);
        if (!std::isfinite(y) || std::abs(y-anchor.y)>160) continue;
        cXyz candidate(probe.x,y,probe.z);
        dBgS_SplGrpChk water; water.Set(candidate,y+100000);
        if (dComIfG_Bgsp().WaterChk(&water) && water.GetHeight()>y+5) continue;
        cXyz from(anchor.x,anchor.y+60,anchor.z), to(candidate.x,y+60,candidate.z);
        dBgS_LinChk wall; wall.Set(&from,&to,nullptr);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        out = candidate; return true;
    }
    return false;
}
bool placement(const Site& site, cXyz& out) {
    if (!site.nativeSpawn) return floor_position(cXyz(site.x,site.y,site.z),out);
    auto* room = dComIfGp_roomControl_getStatusRoomDt(site.room);
    // PLYR records are the native player's actual spawn coordinates, not Link's current
    // position: running into this room cannot move a checkpoint to a convenient new spot.
    const auto try_spawns = [&](stage_actor_class* list, bool filterRoom) {
        if (!list || int(list->num)<=0 || int(list->num)>128) return false;
        stage_actor_data_class* entries = list->m_entries;
        if (!entries) return false;
        for (int i=0;i<int(list->num);++i) {
            if (filterRoom && (uint32_t(entries[i].base.parameters)&63)!=uint32_t(site.room)) continue;
            const cXyz anchor = entries[i].base.position;
            if (std::isfinite(anchor.x) && std::isfinite(anchor.y) && std::isfinite(anchor.z) && floor_position(anchor,out)) return true;
        }
        return false;
    };
    if (room && try_spawns(room->getPlayer(),false)) return true;
    auto* stage = dComIfGp_getStage();
    return stage && try_spawns(stage->getPlayer(),true);
}
void retire(size_t index) {
    if (!g_ids[index]) return;
    auto* actor = fopAcM_SearchByID(g_ids[index]);
    const bool creating = fpcM_IsCreating(g_ids[index]);
    if (!creating && !own_actor(actor,index)) { g_ids[index]=0; return; }
    // Keep failed deletions tracked so they are retried; never delete a reused foreign ID.
    if (svc_actor->delete_actor(mod_ctx,g_ids[index])==MOD_OK) g_ids[index]=0;
}
void sync_stones() {
    for (size_t i=0;i<kSites.size();++i) {
        const bool wanted = markers_enabled() && matches(kSites[i],g_stage,g_room);
        auto* actor = g_ids[i] ? fopAcM_SearchByID(g_ids[i]) : nullptr;
        const bool creating = g_ids[i] && fpcM_IsCreating(g_ids[i]);
        if (g_ids[i] && !creating && !own_actor(actor,i)) g_ids[i]=0;
        if (!wanted || (actor && own_actor(actor,i) && fopAcM_GetParam(actor)>>8 != g_generation)) { retire(i); continue; }
        if (g_ids[i] || !wanted || !player::input_allowed()) continue;
        if (g_retry[i]>0) { --g_retry[i]; continue; }
        cXyz position;
        g_retry[i]=150; // Don't repeatedly request missing terrain/resources every frame.
        if (!placement(kSites[i],position)) continue;
        ActorSpawnParams params{};
        params.parameters=uint32_t(i)|(g_generation<<8); params.argument=-1; params.room_num=kSites[i].room;
        params.position={position.x,position.y,position.z}; params.scale={1,1,1};
        ActorId id=0;
        if (svc_actor->create_actor(mod_ctx,g_profile,&params,&id)==MOD_OK && id!=fpcM_ERROR_PROCESS_ID_e) g_ids[i]=id;
        else mods::log::warn("Sacred Rest Stone could not spawn: {}",kSites[i].name);
    }
}
int nearest(daAlink_c* link) {
    if (!markers_enabled() || !link) return -1;
    for (size_t i=0;i<kSites.size();++i) {
        if (!matches(kSites[i],g_stage,fopAcM_GetRoomNo(link))) continue;
        auto* marker = stone(i);
        if (!marker || std::abs(marker->current.pos.y-link->current.pos.y)>100 ||
            marker->current.pos.abs(link->current.pos)>260) continue;
        cXyz from(link->current.pos.x,link->current.pos.y+70,link->current.pos.z);
        cXyz to(marker->current.pos.x,marker->current.pos.y+70,marker->current.pos.z);
        dBgS_LinChk sight; sight.Set(&from,&to,link);
        if (!dComIfG_Bgsp().LineCross(&sight)) return i;
    }
    return -1;
}
bool idle(daAlink_c* link) {
    if (!link || wolf::busy() || !link->mLinkAcch.ChkGroundHit() || dComIfGs_getLife()==0 ||
        link->mMoveValue>0.05f || std::abs(link->mNormalSpeed)>0.1f || std::abs(link->speedF)>0.1f ||
        (link->mItemTrigger&(daAlink_c::BTN_A|daAlink_c::BTN_B|daAlink_c::BTN_X|daAlink_c::BTN_Y))) return false;
    switch(link->mProcID) {
    case daAlink_c::PROC_WAIT: case daAlink_c::PROC_SERVICE_WAIT: case daAlink_c::PROC_TIRED_WAIT:
    case daAlink_c::PROC_CROUCH: case daAlink_c::PROC_WOLF_WAIT: case daAlink_c::PROC_WOLF_SERVICE_WAIT:
    case daAlink_c::PROC_WOLF_TIRED_WAIT: case daAlink_c::PROC_WOLF_SIT: return true;
    default: return false;
    }
}
struct Threat { daAlink_c* link; float radius; };
void* find_threat(void* ptr,void* data) {
    auto* actor = static_cast<fopAc_ac_c*>(ptr); auto* query = static_cast<Threat*>(data);
    if (actor && fopAcM_GetGroup(actor)==fopAc_ENEMY_e && actor->health>0 &&
        actor->current.pos.abs(query->link->current.pos)<query->radius) return actor;
    return nullptr;
}
bool threat(daAlink_c* link,float radius) { Threat query{link,radius}; return fopAcIt_Judge(find_threat,&query)!=nullptr; }
bool safe(daAlink_c* link) {
    return enabled() && link && player::gameplay_running() && idle(link) &&
        g_timers.quiet>=resources::ticks_for_ms(tuning::get().restNoHitDelayMs) && !threat(link,3000);
}
void hud_post(ModContext*,void*,void*,void*) {
    if (!enabled() || !player::input_allowed() || g_prompt.empty()) return;
    auto* font = mDoExt_getSubFont(); if (!font) font=mDoExt_getMesgFont(); if (!font) return;
    font->setGX(); font->setCharColor(JUtility::TColor(235,210,155,u8(hud::idle_fade::opacity()*255)));
    font->drawString_scale(26,151,12,14,g_prompt.c_str(),true);
    if (auto* port=dComIfGp_getCurrentGrafPort()) port->setup2D();
}
}
void damage_received() { if (enabled()) { g_timers.damage(); g_menuChannel=false; g_requested=-1; } }
void reset() {
    g_generation = (g_generation%0xffffff)+1;
    for (size_t i=0;i<kSites.size();++i) retire(i);
    g_retry.fill(0); g_timers={}; g_owner=nullptr; g_lastLife=-1; g_room=-1; g_stage.clear();
    g_nearest=g_channelSite=g_requested=-1; g_menuChannel=false; g_prompt.clear();
}
bool rest_available(daAlink_c* link) { return markers_enabled() && nearest(link)>=0 && safe(link); }
bool busy() { return enabled() && (g_timers.channel>0 || g_menuChannel); }
void request_rest(daAlink_c* link) { if (rest_available(link)) g_requested=nearest(link); }
void tick(daAlink_c* link) {
    if (!enabled() || !link) {
        g_timers={}; g_prompt.clear(); g_requested=-1; g_menuChannel=false;
        for (size_t i=0;i<kSites.size();++i) retire(i);
        return;
    }
    const char* stage = dComIfGp_getStartStageName();
    const int room = fopAcM_GetRoomNo(link);
    if (link!=g_owner || g_stage!=stage || g_room!=room) {
        g_timers={}; g_lastLife=-1; g_requested=g_channelSite=-1; g_menuChannel=false;
        g_owner=link; g_stage=stage; g_room=room;
    }
    sync_stones();
    const int life=dComIfGs_getLife();
    if (g_lastLife>=0 && life<g_lastLife) damage_received();
    g_lastLife=life;
    const bool running=player::input_allowed();
    const bool danger=running && threat(link,5000);
    g_timers.advance(running,danger);
    g_nearest=nearest(link);
    if (g_nearest!=g_channelSite) { g_timers.channel=0; g_timers.latched=false; g_menuChannel=false; g_channelSite=g_nearest; }
    if (!running) { g_menuChannel=false; g_prompt.clear(); return; }
    if (g_requested>=0) { g_menuChannel=g_requested==g_nearest && safe(link); g_requested=-1; }
    const bool held=mDoCPd_c::getHoldL(PAD_1) && mDoCPd_c::getHoldR(PAD_1);
    const bool valid=g_nearest>=0 && safe(link);
    if (g_timers.rest(running,valid,held||g_menuChannel,resources::ticks_for_ms(tuning::get().restChannelMs))) {
        refill(true); g_menuChannel=false; g_timers.emergency=0;
        g_prompt="Sacred Flask, health and supplies restored";
    } else if (g_nearest>=0) {
        if (!valid) { g_menuChannel=false; g_prompt="Sacred Rest Stone: stand still and stay safe"; }
        else if (g_timers.channel>0) g_prompt=fmt::format("Resting... {:.1f} s",double(g_timers.channel)/30.0);
        else g_prompt=fmt::format("{}: hold L + R to rest",kSites[g_nearest].name);
    } else g_prompt.clear();
    // Optional safety net: one charge only when empty; never heals directly or refills supplies.
    const bool outside=g_stage.starts_with("F_");
    if (tuning::get().passiveFlaskRecovery && g_timers.recover(running,outside && !danger && idle(link),
        progression::model().charges==0,resources::ticks_for_ms(tuning::get().passiveFlaskDelayMs),
        resources::ticks_for_ms(tuning::get().passiveFlaskRestMs))) {
        progression::model().charges=1; progression::persist(); g_prompt="One emergency flask charge restored";
    }
    if (!tuning::get().passiveFlaskRecovery) g_timers.emergency=0;
}
ModResult install() {
    const ActorProfileDesc desc = {
        .name="DFRest", .priority_group=7, .process_size=sizeof(RestStone),
        .draw_priority=fpcDwPi_Obj_SmWStone_e, .status=fopAcStts_UNK_0x40000_e,
        .group=fopAc_ACTOR_e, .cull_type=fopAc_CULLBOX_CUSTOM_e,
        .create_function=create_stone, .delete_function=delete_stone, .execute_function=execute_stone,
        .is_delete_function=can_delete_stone, .draw_function=draw_stone,
    };
    auto result=svc_actor->register_actor(mod_ctx,&desc,&g_profile,&g_registration);
    if (result==MOD_OK) result=mods::hook::add_post<hooks::Shared1>(hud_post);
    return result;
}
void shutdown() {
    reset();
    if (g_registration && svc_actor->unregister_actor(mod_ctx,g_registration)==MOD_OK) { g_registration=0; g_profile=-1; }
}
}
