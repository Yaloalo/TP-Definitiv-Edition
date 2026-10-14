"""Exercise production rest/recovery callbacks and actor cleanup without game assets."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/healing/rest_stones.cpp').read_text()
def function(name):
    match=re.search(r'^(?:void|bool|int) '+name+r'\(',source,re.M)
    if not match: raise ValueError(name)
    start=match.start();at=source.index('{',start)+1;depth=1
    while depth:
        if source[at]=='{':depth+=1
        elif source[at]=='}':depth-=1
        at+=1
    return source[start:at]+'\n'
def run(code):
    with tempfile.TemporaryDirectory() as folder:
        cpp=Path(folder)/'test.cpp';exe=Path(folder)/'test';cpp.write_text(code)
        subprocess.run(['c++','-std=c++20','-O0','-UNDEBUG','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
common=r'''
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <array>
#include "healing/rest_model.hpp"
#include "healing/rest_sites.hpp"
#include "resources/recharge_model.hpp"
#include "progression/model.hpp"
bool active=true,visible=false,running=true,wolfBusy=false;int hp=4,refills=0,persists=0,syncs=0,point=0;float enemyDistance=100000;
bool is_active(){return active;}int dComIfGs_getLife(){return hp;}
namespace remaster::player {bool gameplay_running(){return running;}bool input_allowed(){return running&&!visible;}}
namespace remaster::healing::wolf {bool busy(){return wolfBusy;}}
namespace remaster::tuning {struct Values{bool flaskEnabled=true,restStones=true,passiveFlaskRecovery=true;int restNoHitDelayMs=10000,restChannelMs=3000,passiveFlaskDelayMs=90000,passiveFlaskRestMs=30000;} cfg;auto& get(){return cfg;}}
namespace remaster::progression {Model save;bool ready(){return true;}Model& model(){return save;}void persist(){++persists;}}
namespace remaster::healing {void refill(bool){++refills;hp=12;progression::save.charges=3;}}
constexpr int PAD_1=0;bool heldL=false,heldR=false;
struct mDoCPd_c {static bool getHoldL(int){return heldL;}static bool getHoldR(int){return heldR;}};
struct daAlink_c {
 enum {PROC_WAIT,PROC_SERVICE_WAIT,PROC_TIRED_WAIT,PROC_CROUCH,PROC_WOLF_WAIT,PROC_WOLF_SERVICE_WAIT,PROC_WOLF_TIRED_WAIT,PROC_WOLF_SIT,PROC_ATTACK,PROC_SWIM};
 enum {BTN_A=1,BTN_B=2,BTN_X=4,BTN_Y=8};
 struct {bool grounded=true;bool ChkGroundHit(){return grounded;}}mLinkAcch;
 float mMoveValue=0,mNormalSpeed=0,speedF=0;int mItemTrigger=0,mProcID=PROC_WAIT,room=4;
};
int fopAcM_GetRoomNo(daAlink_c* link){return link->room;}std::string currentStage="R_SP01";
const char* dComIfGp_getStartStageName(){return currentStage.c_str();}
namespace fmt {template<class...T>std::string format(const char*,T...){return "prompt";}}
namespace remaster::healing::stones {
RestTimers g_timers;daAlink_c* g_owner=nullptr;int g_lastLife=-1,g_room=-1,g_nearest=-1,g_channelSite=-1,g_requested=-1;
std::string g_stage,g_prompt;bool g_menuChannel=false;
void sync_stones(){++syncs;}int nearest(daAlink_c*){return tuning::cfg.restStones?point:-1;}
bool threat(daAlink_c*,float radius){return enemyDistance<radius;}
void retire(size_t){}
'''
main=r'''
}
int main(){using namespace remaster;using namespace healing::stones;daAlink_c link;
 auto ticks=[&](int n){for(int i=0;i<n;++i)tick(&link);};
 ticks(300);assert(refills==0&&hp==4); // entering a former rest room never refills by itself
 heldL=heldR=true;ticks(89);assert(refills==0&&g_timers.channel==89);ticks(1);assert(refills==1&&hp==12);
 ticks(1000);assert(refills==1); // holding shoulders cannot retrigger endlessly
 heldL=heldR=false;ticks(1);heldL=heldR=true;ticks(45);assert(g_timers.channel==45);
 visible=true;const int before=g_timers.quiet;ticks(500);assert(g_timers.channel==0&&g_timers.quiet==before&&refills==1);
 visible=false;ticks(89);assert(refills==1);ticks(1);assert(refills==2);
 heldL=heldR=false;ticks(1);heldL=heldR=true;ticks(45);damage_received();
 ticks(200);assert(refills==2&&g_timers.channel==0);ticks(190);assert(refills==3);
 heldL=heldR=false;ticks(1);enemyDistance=2000;heldL=heldR=true;ticks(1000);assert(refills==3&&g_timers.channel==0);
 enemyDistance=100000;link.mMoveValue=1;ticks(1000);assert(refills==3);link.mMoveValue=0;
 link.mProcID=daAlink_c::PROC_ATTACK;ticks(1000);assert(refills==3);link.mProcID=daAlink_c::PROC_WAIT;
 link.mLinkAcch.grounded=false;ticks(1000);assert(refills==3);link.mLinkAcch.grounded=true;
 wolfBusy=true;ticks(1000);assert(refills==3);wolfBusy=false;
 point=-1;ticks(1000);assert(refills==3);point=0;heldL=heldR=false;
 // Menu button queues the same vulnerable channel; closing the menu is required.
 visible=true;request_rest(&link);ticks(100);assert(refills==3&&g_requested==0);
 visible=false;ticks(89);assert(refills==3);ticks(1);assert(refills==4&&g_requested==-1);
 // Scene/room changes discard pending channels and do not preserve recovery time.
 damage_received();point=1;link.room=7;tick(&link);assert(g_timers.quiet==1&&!rest_available(&link));
 // Optional emergency recovery is only one charge when empty, with no direct health gain.
 currentStage="F_SP103";link.room=0;point=-1;progression::save.charges=0;hp=4;tick(&link);
 ticks(2698);assert(progression::save.charges==0&&g_timers.emergency==0);
 tick(&link);assert(g_timers.emergency==1);ticks(898);assert(progression::save.charges==0);
 tick(&link);assert(progression::save.charges==1&&hp==4&&refills==4&&persists==1);
 ticks(20000);assert(progression::save.charges==1&&persists==1); // never fills the entire flask
 progression::save.charges=0;damage_received();ticks(2699);link.mMoveValue=1;ticks(1000);assert(progression::save.charges==0&&g_timers.emergency==0);
 link.mMoveValue=0;ticks(899);assert(progression::save.charges==0);enemyDistance=4000;ticks(2000);assert(g_timers.peace==0&&progression::save.charges==0);
 enemyDistance=100000;visible=true;ticks(10000);assert(g_timers.peace==0&&progression::save.charges==0);visible=false;
 currentStage="D_MN05";tick(&link);ticks(10000);assert(progression::save.charges==0); // no dungeon/boss/Cave regeneration
 currentStage="F_SP103";tick(&link);tuning::cfg.passiveFlaskRecovery=false;ticks(10000);assert(progression::save.charges==0);
 tuning::cfg.passiveFlaskRecovery=true;active=false;ticks(10000);assert(progression::save.charges==0&&g_prompt.empty());
 std::cout<<"Rest callbacks: deliberate channel, interruption, menus, safe proximity, scene isolation and one-charge overworld recovery passed\n";
}
'''
run(common+''.join(function(n) for n in ['enabled','markers_enabled','idle','safe','damage_received','rest_available','request_rest','tick'])+main)

# ActorService can refuse immediate deletion. Preserve the owned ID for retries,
# but clear a reused foreign ID without deleting the foreign actor.
cleanup=r'''
#include <cassert>
#include <array>
#include <iostream>
using ActorId=unsigned;struct ModContext{};ModContext* mod_ctx=nullptr;constexpr int MOD_OK=0,MOD_UNAVAILABLE=1;
struct fopAc_ac_c{int profile;unsigned parameters;};fopAc_ac_c own{17,0},foreign{18,0};
bool creating=false,exists=true,reused=false,refuse=true;int deletes=0,g_profile=17;
bool fpcM_IsCreating(ActorId){return creating;}
fopAc_ac_c* fopAcM_SearchByID(ActorId){return exists?(reused?&foreign:&own):nullptr;}
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}unsigned fopAcM_GetParam(fopAc_ac_c* a){return a->parameters;}
struct ActorService{int delete_actor(ModContext*,ActorId){++deletes;return refuse?MOD_UNAVAILABLE:MOD_OK;}}service;auto* svc_actor=&service;
std::array<ActorId,1>g_ids{7};
'''
run(cleanup+function('own_actor')+function('retire')+r'''
int main(){retire(0);assert(deletes==1&&g_ids[0]==7);refuse=false;retire(0);assert(deletes==2&&g_ids[0]==0);
 g_ids[0]=7;reused=true;retire(0);assert(deletes==2&&g_ids[0]==0);
 g_ids[0]=7;exists=false;retire(0);assert(deletes==2&&g_ids[0]==0);
 g_ids[0]=7;creating=true;refuse=true;retire(0);assert(deletes==3&&g_ids[0]==7);
 std::cout<<"Actor cleanup: deferred deletion retries and foreign/reused ID protection passed\n";}
''')

# Coverage must contain distinct sites, avoid boss/Cave rooms, and include new regions.
sites=(root/'src/healing/rest_sites.hpp').read_text()
entries=re.findall(r'\{"([A-Z0-9_]+)",(\d+),"([^"]+)"',sites)
assert len(entries)==32 and len({(s,r) for s,r,_ in entries})==32
assert all(not s.endswith(('A','B','C')) and not s.startswith('D_SB') for s,_,_ in entries)
for required in ['F_SP103','F_SP108','F_SP116','F_SP126','F_SP113','F_SP115','F_SP124','F_SP114','F_SP128']:
    assert any(s==required for s,_,_ in entries)
print('Thirty-two distinct site placements cover the main regions and exclude boss arenas/Cave of Ordeals')

# Production placement checks must reject water, missing floors and obstructed
# approaches. PLYR anchors stay fixed even when Link moves elsewhere in the room.
geometry=r'''
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
#include <vector>
#include <cstdint>
#include "healing/rest_sites.hpp"
using namespace remaster::healing::stones;
struct cXyz {float x,y,z;};
float groundHeight=0,waterHeight=-10000;bool walls=false;int probes=0;
struct dBgS_ObjGndChk {cXyz point;void SetPos(cXyz* p){point=*p;}};
struct dBgS_SplGrpChk {cXyz point;float height;void Set(cXyz& p,float roof){point=p;height=roof;}float GetHeight(){return waterHeight;}};
struct dBgS_LinChk {void Set(cXyz*,cXyz*,void*){}};
struct Background {
 float GroundCross(dBgS_ObjGndChk*){++probes;return groundHeight;}
 bool WaterChk(dBgS_SplGrpChk* p){return waterHeight>p->point.y&&waterHeight<p->height;}
 bool LineCross(dBgS_LinChk*){return walls;}
} bg;auto& dComIfG_Bgsp(){return bg;}
struct stage_actor_data_class{struct{uint32_t parameters;cXyz position;}base;};
struct stage_actor_class{int num;stage_actor_data_class* m_entries;};
struct Stage {stage_actor_class* players=nullptr;auto* getPlayer(){return players;}}room,stage;
auto* dComIfGp_roomControl_getStatusRoomDt(int){return &room;}auto* dComIfGp_getStage(){return &stage;}
'''
run(geometry+function('floor_position')+function('placement')+r'''
int main(){cXyz out{},anchor{100,0,100};
 assert(floor_position(anchor,out)&&out.x==320&&out.y==0&&out.z==100);
 waterHeight=1000;assert(!floor_position(anchor,out)); // deeply submerged floors must also fail
 waterHeight=-10000;groundHeight=500;assert(!floor_position(anchor,out));
 groundHeight=std::numeric_limits<float>::quiet_NaN();assert(!floor_position(anchor,out));
 groundHeight=0;walls=true;assert(!floor_position(anchor,out));walls=false;
 stage_actor_data_class records[]={{{2,{100,0,100}}},{{4,{900,0,900}}}};
 stage_actor_class players{2,records};stage.players=&players;
 Site native{"R_SP01",4,"house",true};assert(placement(native,out)&&out.x==1120&&out.z==900);
 room.players=&players;assert(placement(native,out)&&out.x==320); // room-scoped records need no stage room filter
 players.num=129;assert(!placement(native,out));players.num=0;assert(!placement(native,out));
 room.players=nullptr;stage.players=nullptr;assert(!placement(native,out));
 Site fixed{"F_SP103",0,"village",false,500,0,800};assert(placement(fixed,out)&&out.x==720&&out.z==800);
 std::cout<<"Production placement: fixed/native room anchors, stage room filters, missing data, floor height, deep water and wall rejection passed\n";}
''')

proximity=r'''
struct daAlink_c {struct {cXyz pos;}current;int room=4;};
int fopAcM_GetRoomNo(daAlink_c* p){return p->room;}
struct RestStone {struct {cXyz pos;}current;}marker{{{100,0,100}}};
bool markers=true,loaded=true;std::string g_stage="R_SP01";
bool markers_enabled(){return markers;}
RestStone* stone(size_t i){return loaded&&i==0?&marker:nullptr;}
'''
# Add the native cXyz distance helper needed by the actual proximity callback.
nearest_geometry=geometry.replace('struct cXyz {float x,y,z;};',
    'struct cXyz {float x,y,z;float abs(const cXyz& p)const{return std::sqrt((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));}};')
run(nearest_geometry+'\n#include <string>\n'+proximity+function('nearest')+r'''
int main(){daAlink_c link{{{100,0,100}},4};assert(nearest(&link)==0);
 link.current.pos.x=361;assert(nearest(&link)==-1);link.current.pos.x=100;
 link.current.pos.y=101;assert(nearest(&link)==-1);link.current.pos.y=0;
 walls=true;assert(nearest(&link)==-1);walls=false;
 link.room=7;assert(nearest(&link)==-1);link.room=4;
 loaded=false;assert(nearest(&link)==-1);loaded=true;markers=false;assert(nearest(&link)==-1);
 std::cout<<"Production proximity: physical range, height, line of sight, room, loaded model and feature toggle gates passed\n";}
''')
