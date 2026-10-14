"""Execute Epona callbacks and the pinned native chaseF, including nested calls."""
from fixture import ROOT, function, run

source = (ROOT / 'src/movement/horse.cpp').read_text()
native = (ROOT / 'dusklight/src/SSystem/SComponent/c_lib.cpp').read_text()
code = r'''
#include <cassert>
#include <cmath>
#include <vector>
#include <cstdint>
#include "movement/turn_model.hpp"
using s16=int16_t;using f32=float;using BOOL=int;
constexpr int TRUE=1,FALSE=0;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T& arg_ref(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}
template<class T>T arg(void* p,int i){return arg_ref<T>(p,i);}}
bool active=true,running=true;bool is_active(){return active;}
namespace remaster::player {bool input_allowed(){return running;}}
namespace remaster::tuning {struct Values{bool horseHandling=true;int64_t horseTurnSpeedPercent=150,horseAccelerationPercent=140,horseBrakingPercent=150;}cfg;auto& get(){return cfg;}}
struct daHorse_c {
 enum daHorse_FLG0{FLG0_RODEO_MODE=1,FLG0_UNK_10000000=2,FLG0_UNK_100000=4,FLG0_UNK_200000=8,FLG0_UNK_40000=16,FLG0_UNK_4=32};
 enum {PROC_WAIT_e,PROC_MOVE_e,PROC_JUMP_e,PROC_LAND_e,PROC_STOP_e,PROC_TURN_e,PROC_LARGE_DAMAGE_e,PROC_TOOL_DEMO_e};
 struct{struct{s16 y=0;}angle;}current;struct{s16 y=0;}shape_angle;
 struct{bool ground=true,wall=false;bool ChkGroundHit(){return ground;}bool ChkWallHit(){return wall;}}m_acch;
 int m_procID=PROC_MOVE_e,m_demoMode=0,m_cowHit=0,field_0x1702=0,flags=0;
 float speedF=20;bool input=true;s16 m_padStickAngleY=2000;
 bool checkStateFlg0(daHorse_FLG0 f){return flags&f;}bool checkInputOnR(){return input;}
};
struct daAlink_c {bool mounted=true,event=false,aim=false,lock=false,hang=false;void* mTargetedActor=nullptr;daHorse_c* horse=nullptr;
 bool checkHorseRideOn(){return mounted;}void* getRideActor(){return horse;}bool checkEventRun(){return event;}
 bool checkHorseSubjectivity(){return aim;}bool checkAttentionLock(){return lock;}bool checkHorseHangMode(){return hang;}}link;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
'''
code += function(native, 'cLib_chaseF')
code += r'''
namespace remaster::movement::horse {
struct Capture{void* args;daHorse_c* horse;s16 angle,shape;bool eligible;};std::vector<Capture>g_calls;
'''
code += ''.join(function(source, name) for name in ['allowed', 'multiplier', 'movement_pre', 'chase_pre', 'movement_post', 'reset'])
code += r'''
}
int main(){using namespace remaster;using namespace movement::horse;
 daHorse_c h;link.horse=&h;auto* p=&h;void* a[]={&p};int result=0;
 auto chase=[&](float* value,float target,float step){void* args[]={&value,&target,&step};
 chase_pre(nullptr,args,nullptr,nullptr);cLib_chaseF(value,target,step);return step;};
 movement_pre(nullptr,a,nullptr,nullptr);
 assert(std::abs(chase(&h.speedF,30,2)-2.8f)<.001f&&std::abs(h.speedF-22.8f)<.001f);
 assert(chase(&h.speedF,0,2)==3);float unrelated=20;assert(chase(&unrelated,30,2)==2);
 // Native clamping: stopping and accelerating cannot overshoot, even at 300%.
 tuning::cfg.horseAccelerationPercent=300;tuning::cfg.horseBrakingPercent=300;
 h.speedF=29;chase(&h.speedF,30,2);assert(h.speedF==30);
 h.speedF=1;chase(&h.speedF,0,2);assert(h.speedF==0);
 assert(chase(&h.speedF,-10,2)==2&&h.speedF==-2);assert(chase(&h.speedF,0,2)==2&&h.speedF==0);
 h.current.angle.y=h.shape_angle.y=100;movement_post(nullptr,a,&result,nullptr);
 assert(h.current.angle.y==150&&h.shape_angle.y==150&&g_calls.empty());
 // Script, aim, target, rodeo/call/bridge flags, jumps, damage and menu gates.
 for(int gate=0;gate<22;++gate){h={};link={};link.horse=&h;active=running=true;tuning::cfg.horseHandling=true;
 switch(gate){case 0:active=false;break;case 1:running=false;break;case 2:tuning::cfg.horseHandling=false;break;
 case 3:link.mounted=false;break;case 4:link.horse=nullptr;break;case 5:link.event=true;break;case 6:link.aim=true;break;
 case 7:link.lock=true;break;case 8:link.mTargetedActor=&link;break;case 9:link.hang=true;break;
 case 10:h.m_demoMode=4;break;case 11:h.m_acch.ground=false;break;case 12:h.m_cowHit=1;break;
 case 13:h.field_0x1702=100;break;case 14:h.flags=1;break;case 15:h.flags=2;break;case 16:h.flags=4;break;
 case 17:h.flags=8;break;case 18:h.flags=16;break;case 19:h.flags=32;break;case 20:h.m_procID=daHorse_c::PROC_JUMP_e;break;
 case 21:h.m_procID=daHorse_c::PROC_TOOL_DEMO_e;break;}
 movement_pre(nullptr,a,nullptr,nullptr);assert(chase(&h.speedF,30,2)==2);
 h.current.angle.y=h.shape_angle.y=100;movement_post(nullptr,a,&result,nullptr);assert(h.current.angle.y==100&&g_calls.empty());}
 active=running=true;tuning::cfg={};h={};link={};link.horse=&h;
 // Early native obstruction results, wall corrections and reverse steering survive.
 for(int gate=0;gate<4;++gate){h={};result=gate==0?4:0;h.m_acch.wall=gate==1;
 h.m_padStickAngleY=gate==2?-30000:2000;h.input=gate!=3;
 movement_pre(nullptr,a,nullptr,nullptr);h.current.angle.y=h.shape_angle.y=100;
 movement_post(nullptr,a,&result,nullptr);assert(h.current.angle.y==100);}
 // Wrapped angles and no overshoot.
 h={};h.current.angle.y=h.shape_angle.y=32700;h.m_padStickAngleY=-32700;result=0;
 movement_pre(nullptr,a,nullptr,nullptr);h.current.angle.y=h.shape_angle.y=-32720;
 movement_post(nullptr,a,&result,nullptr);assert(h.current.angle.y==-32700);
 // Nested unrelated horses cannot inherit the outer horse's acceleration.
 h={};daHorse_c other;auto* q=&other;void* b[]={&q};link.horse=&h;
 movement_pre(nullptr,a,nullptr,nullptr);movement_pre(nullptr,b,nullptr,nullptr);
 assert(chase(&h.speedF,30,2)==2);movement_post(nullptr,a,&result,nullptr);assert(g_calls.size()==2);
 movement_post(nullptr,b,&result,nullptr);assert(g_calls.size()==1);
 assert(std::abs(chase(&h.speedF,30,2)-2.8f)<.001f);movement_post(nullptr,a,&result,nullptr);assert(g_calls.empty());
 // A mode change during the original call suppresses post-processing.
 movement_pre(nullptr,a,nullptr,nullptr);active=false;h.current.angle.y=100;movement_post(nullptr,a,&result,nullptr);assert(h.current.angle.y==100);
 assert(multiplier(-1)==1&&multiplier(999)==3);
}
'''
run(code)
print('Epona production callbacks: native speed clamps, 22 eligibility gates, backwards movement, wrap, nesting and mode transitions passed')
