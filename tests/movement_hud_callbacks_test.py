"""Execute production movement, HUD pane restoration and damage-feedback callbacks."""
from pathlib import Path
import re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
def function(source,name):
    match=re.search(r'^(?:HookAction|void|bool|int|float|double|f32) '+name+r'\(',source,re.M)
    if not match:raise ValueError(name)
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
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include "movement/turn_model.hpp"
#include "hud/feedback_model.hpp"
using s16=int16_t;using u16=uint16_t;using u8=uint8_t;using f32=float;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};bool active=true,running=true,visible=false;
bool is_active(){return active;}
namespace mods {template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
namespace remaster::player {bool input_allowed(){return running&&!visible;}}
namespace remaster::tuning {
struct Values{bool humanTurning=true,wolfTurning=true,quickPivots=true,fastCrawling=true,idleHudFade=true,damageFeedback=true,flaskEnabled=true,staminaEnabled=true;
 int64_t humanTurnSpeedPercent=200,wolfTurnSpeedPercent=200,crawlSpeedPercent=175,hudIdleDelayMs=5000,hudFadeDurationMs=800,hudIdleOpacityPercent=0;
 int64_t damageFeedbackDurationMs=500,damageFeedbackOpacityPercent=25;} cfg;auto& get(){return cfg;}}
struct cXyz{float x=0,y=0,z=0;float abs(const cXyz& p)const{return std::sqrt((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));}
 float abs2()const{return x*x+y*y+z*z;}cXyz operator-(const cXyz& p)const{return{x-p.x,y-p.y,z-p.z};}};
struct daPy_py_c{enum{ERFLG1_UNK_200000=1};};constexpr int dBgW_SPCODE_ICE=9;
struct daAlink_c{
 enum{PROC_WAIT,PROC_MOVE,PROC_WOLF_WAIT,PROC_WOLF_MOVE,PROC_WOLF_DASH,PROC_CRAWL_MOVE,PROC_CRAWL_AUTO_MOVE,PROC_ATTACK,
 PROC_SERVICE_WAIT,PROC_TIRED_WAIT,PROC_WOLF_SERVICE_WAIT,PROC_WOLF_TIRED_WAIT};
 struct {cXyz pos;struct{s16 y=0;}angle;}current;struct{s16 y=0;}shape_angle;
 int mProcID=PROC_MOVE,mGndPolySpecialCode=0,room=0;u16 mItemTrigger=0;void* mTargetedActor=nullptr;
 s16 mMoveAngle=1000,field_0x3180=0;float speedF=35,mNormalSpeed=35;
 bool wolf=false,event=false,locked=false,heavy=false,magnetic=false,input=true,attention=false,wall=false,reversed=false;
 struct {bool grounded=true;bool ChkGroundHit(){return grounded;}}mLinkAcch;
 bool checkWolf(){return wolf;}bool checkEventRun(){return event;}bool checkAttentionLock(){return locked;}
 bool checkBootsOrArmorHeavy(){return heavy;}bool checkMagneBootsOn(){return magnetic;}bool checkInputOnR(){return input;}
 bool checkAttentionState(){return attention;}bool checkEndResetFlg1(int){return wall;}bool checkWolfShapeReverse(){return reversed;}
};
'''
turn=(root/'src/movement/responsiveness.cpp').read_text()
turn_fixture=r'''
namespace remaster::movement::responsiveness {
struct Capture{void* args;daAlink_c* link;s16 angle,shape;bool eligible;};std::vector<Capture>g_human,g_wolf;
'''
turn_main=r'''
}
int main(){using namespace remaster;using namespace movement::responsiveness;
 daAlink_c link;auto* p=&link;void* args[]={&p};
 human_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=link.shape_angle.y=100;human_post(nullptr,args,nullptr,nullptr);
 assert(link.current.angle.y==200&&link.shape_angle.y==200&&link.speedF==35&&link.mNormalSpeed==35&&g_human.empty());
 link.current.angle.y=link.shape_angle.y=0;link.mMoveAngle=-30000;human_pre(nullptr,args,nullptr,nullptr);human_post(nullptr,args,nullptr,nullptr);
 assert(link.current.angle.y==-30000&&link.speedF==17.5&&link.mNormalSpeed==17.5);
 for(int gate=0;gate<7;++gate){link={};if(gate==0)link.locked=true;if(gate==1)link.heavy=true;if(gate==2)link.magnetic=true;
 if(gate==3)link.mGndPolySpecialCode=dBgW_SPCODE_ICE;if(gate==4)link.event=true;if(gate==5)link.mLinkAcch.grounded=false;if(gate==6)link.mProcID=daAlink_c::PROC_ATTACK;
 human_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=100;human_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==100);}
 link={};active=false;human_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=100;human_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==100);active=true;
 // Wrong-owner/skipped post callbacks cannot pop a nested call's snapshot.
 link={};daAlink_c other;auto* q=&other;void* inner[]={&q};human_pre(nullptr,args,nullptr,nullptr);human_pre(nullptr,inner,nullptr,nullptr);
 human_post(nullptr,args,nullptr,nullptr);assert(g_human.size()==2);
 other.current.angle.y=100;human_post(nullptr,inner,nullptr,nullptr);link.current.angle.y=100;human_post(nullptr,args,nullptr,nullptr);
 assert(g_human.empty()&&link.current.angle.y==200&&other.current.angle.y==200);
 link={};link.wolf=true;link.mProcID=daAlink_c::PROC_WOLF_MOVE;wolf_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=100;wolf_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==200&&link.speedF==35);
 link.current.angle.y=0;link.wall=true;wolf_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=100;wolf_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==100);
 link.wall=false;link.current.angle.y=0;link.mProcID=daAlink_c::PROC_WOLF_DASH;wolf_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=100;wolf_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==100);
 link.mProcID=daAlink_c::PROC_WOLF_MOVE;link.current.angle.y=0;link.reversed=true;link.mMoveAngle=31000;
 wolf_pre(nullptr,args,nullptr,nullptr);wolf_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==31000&&link.shape_angle.y==s16(31000+0x8000));
 tuning::cfg.wolfTurning=false;link.current.angle.y=0;wolf_pre(nullptr,args,nullptr,nullptr);link.current.angle.y=100;wolf_post(nullptr,args,nullptr,nullptr);assert(link.current.angle.y==100);
 std::cout<<"Production turns: sprint speed preserved, pivots, wall/ice/heavy/action gates, nested calls, Wolf burst/reversal and mode isolation passed\n";
}
'''
run(common+turn_fixture+''.join(function(turn,n) for n in ['allowed','capture_before','turn_after','human_pre','wolf_pre','human_post','wolf_post'])+turn_main)

crawl=(root/'src/movement/crawl.cpp').read_text()
native=(root/'dusklight/src/d/actor/d_a_alink_crawl.inc').read_text()
crawl_fixture=r'''
namespace remaster::movement::crawl {
'''
crawl_main=r'''
}
// The actual native movement function uses the scaled animation rate once.
struct daPy_frameCtrl_c{float frame=5,rate=1.75;float getFrame(){return frame;}float getRate(){return rate;}};
struct NativeLink{daPy_frameCtrl_c* mUnderFrameCtrl;struct Hio{struct{struct{float mCrawlMoveRate=10;}m;}mCrouch;}*mpHIO;
 float getCrawlMoveSpeed();};
float cM_fsin(float value){return std::sin(value);}
'''
# Qualified native method needs its own extraction, with the same body parser.
native_source=native.replace('f32 daAlink_c::getCrawlMoveSpeed()', 'f32 getCrawlMoveSpeed()')
native_speed=function(native_source,'getCrawlMoveSpeed').replace('f32 getCrawlMoveSpeed()', 'f32 NativeLink::getCrawlMoveSpeed()')
run(common+crawl_fixture+function(crawl,'speed_post')+crawl_main+native_speed+r'''
int main(){using namespace remaster;daAlink_c link;link.mProcID=daAlink_c::PROC_CRAWL_MOVE;auto* p=&link;void* args[]={&p};float rate=1;
 movement::crawl::speed_post(nullptr,args,&rate,nullptr);assert(rate==1.75f);
 daPy_frameCtrl_c frame;frame.rate=rate;NativeLink::Hio hio;NativeLink native{&frame,&hio};
 assert(std::abs(native.getCrawlMoveSpeed()-17.5f*std::sin(5*0.18479956686496735f))<0.0001);
 frame.rate=-rate;assert(native.getCrawlMoveSpeed()<0); // backwards crawling survives
 link.mProcID=daAlink_c::PROC_CRAWL_AUTO_MOVE;rate=1;movement::crawl::speed_post(nullptr,args,&rate,nullptr);assert(rate==1);
 link.mProcID=daAlink_c::PROC_CRAWL_MOVE;link.event=true;movement::crawl::speed_post(nullptr,args,&rate,nullptr);assert(rate==1);
 link.event=false;active=false;movement::crawl::speed_post(nullptr,args,&rate,nullptr);assert(rate==1);
 std::cout<<"Production crawl: animation/movement scale once, native backwards motion and scripted/mode isolation passed\n";}
''')

fade=(root/'src/hud/idle_fade.cpp').read_text()
fade_fixture=r'''
bool enemyNear=false,wolfHealing=false,resting=false;int life=16,maxLife=16,charges=3;double now=0;
int dComIfGs_getLife(){return life;}int dComIfGs_getMaxLifeGauge(){return maxLife;}
bool dComIfGp_isEnableNextStage(){return false;}
daAlink_c* engineLink=nullptr;daAlink_c* daAlink_getAlinkActorClass(){return engineLink;}
constexpr int PAD_1=0;int buttons=0;struct mDoCPd_c{static int getHold(int){return buttons;}};
namespace remaster::healing::wolf{bool busy(){return wolfHealing;}}
namespace remaster::healing::stones{bool busy(){return resting;}}
namespace remaster::progression {struct Save{int charges=3;}save;bool ready(){return true;}Save& model(){return save;}}
namespace remaster::combat::stamina {struct State{float current=100,maximum=100;bool exhausted=false;}st;State state(){return st;}}
void* fopAcIt_Judge(void*(*)(void*,void*),void*){return enemyNear?reinterpret_cast<void*>(1):nullptr;}
struct J2DPane {u8 alpha=200,mColorAlpha=180;bool influenced=true;J2DPane* child=nullptr,*next=nullptr;
 bool isInfluencedAlpha(){return influenced;}u8 getAlpha(){return alpha;}void setAlpha(u8 value){alpha=value;}
 J2DPane* getFirstChildPane(){return child;}J2DPane* getNextChildPane(){return next;}};
struct J2DScreen:J2DPane{};struct J2DPicture:J2DPane{};
namespace remaster::hud::idle_fade {
IdleEnvelope g_envelope;daAlink_c* g_owner=nullptr;cXyz g_position;s16 g_angle=0;int g_life=-1,g_charges=-1;
bool g_idle=false;float g_alpha=1;unsigned g_depth=0;std::vector<void*>g_calls;
struct PaneAlpha{J2DPane* pane;u8 alpha,colorAlpha;bool changed;};struct Frame{void* call;std::vector<PaneAlpha>panes;};
std::vector<Frame>g_screens,g_pictures;
double seconds(){return now;}void* nearby_enemy(void*,void*){return nullptr;}
float opacity();void reset();
'''
fade_names=['idle_proc','fade_tree','restore_frame','restore','hud_pre','hud_post','screen_pre','screen_post','picture_pre','picture_post','opacity','tick','reset','save_reset']
fade_main=r'''
}
int main(){using namespace remaster;using namespace hud::idle_fade;
 daAlink_c link;link.mProcID=daAlink_c::PROC_WAIT;engineLink=&link;
 tick(&link);assert(!g_idle);tick(&link);assert(g_idle);
 int token=0;void* hudArgs=&token;
 for(int i=0;i<=60;++i){now=double(i)/10;hud_pre(nullptr,hudArgs,nullptr,nullptr);hud_post(nullptr,hudArgs,nullptr,nullptr);}
 assert(opacity()<0.001);link.current.pos.x=1;tick(&link);assert(opacity()==1&&!g_idle);tick(&link);assert(g_idle);
 buttons=1;tick(&link);assert(!g_idle);buttons=0;
 life=4;tick(&link);tick(&link);assert(!g_idle);life=16;tick(&link);tick(&link);assert(g_idle);
 progression::save.charges=0;tick(&link);tick(&link);assert(!g_idle);progression::save.charges=3;
 resting=true;tick(&link);assert(!g_idle);resting=false;wolfHealing=true;tick(&link);assert(!g_idle);wolfHealing=false;
 enemyNear=true;tick(&link);assert(!g_idle);enemyNear=false;
 combat::stamina::st.current=50;tick(&link);assert(!g_idle);combat::stamina::st.current=100;
 visible=true;tick(&link);assert(!g_idle&&opacity()==1);visible=false;
 // Native menus outside a HUD scope retain their alphas even with cached fade.
 J2DScreen screen;J2DPane inherited,independent;screen.child=&inherited;inherited.next=&independent;
 inherited.alpha=100;independent.alpha=120;independent.influenced=false;
 auto* ps=&screen;void* screenArgs[]={&ps};g_alpha=0.5;g_depth=0;
 screen_pre(nullptr,screenArgs,nullptr,nullptr);assert(screen.alpha==200&&inherited.alpha==100&&independent.alpha==120);
 screen_post(nullptr,screenArgs,nullptr,nullptr);
 g_depth=1;screen_pre(nullptr,screenArgs,nullptr,nullptr);assert(screen.alpha==100&&inherited.alpha==100&&independent.alpha==60);
 // Pictures inside a faded screen inherit once; no second opacity multiplication.
 J2DPicture picture;auto* pp=&picture;void* pictureArgs[]={&pp};picture_pre(nullptr,pictureArgs,nullptr,nullptr);assert(picture.alpha==200);
 picture_post(nullptr,pictureArgs,nullptr,nullptr);
 screen.mColorAlpha=50;inherited.mColorAlpha=40;independent.mColorAlpha=30;
 int wrongToken=0;screen_post(nullptr,&wrongToken,nullptr,nullptr);assert(g_screens.size()==1&&screen.alpha==100);
 screen_post(nullptr,screenArgs,nullptr,nullptr);assert(screen.alpha==200&&inherited.alpha==100&&independent.alpha==120);
 assert(screen.mColorAlpha==180&&inherited.mColorAlpha==180&&independent.mColorAlpha==180);
 picture_pre(nullptr,pictureArgs,nullptr,nullptr);assert(picture.alpha==100);picture_post(nullptr,pictureArgs,nullptr,nullptr);assert(picture.alpha==200);
 // A mode change while a snapshot exists restores its original values.
 screen_pre(nullptr,screenArgs,nullptr,nullptr);assert(screen.alpha==100);reset();assert(screen.alpha==200&&g_depth==0&&g_screens.empty());
 active=false;g_alpha=0;assert(opacity()==1);active=true;tuning::cfg.idleHudFade=false;assert(opacity()==1);
 save_reset(nullptr,1,nullptr);assert(g_owner==nullptr&&g_alpha==1&&g_life==-1);
 std::cout<<"Production HUD fade: safe idle/activity, healing/rest/combat/low-health gates, menu isolation, alpha inheritance and restoration passed\n";
}
'''
run(common+fade_fixture+''.join(function(fade,n) for n in fade_names)+fade_main)

damage=(root/'src/hud/damage_feedback.cpp').read_text()
gpu_tokens=set(re.findall(r'\bGX_[A-Z0-9_]+',function(damage,'draw_pre')))
gpu_constants='enum{'+','.join(sorted(gpu_tokens))+'};\n'
damage_fixture=r'''
int life=12,setups=0;double now=10;std::string currentStage="F_SP103";
int dComIfGs_getLife(){return life;}const char* dComIfGp_getStartStageName(){return currentStage.c_str();}
int fopAcM_GetRoomNo(daAlink_c* link){return link->room;}
struct Port{void setup2D(){++setups;}}port;Port* dComIfGp_getCurrentGrafPort(){return &port;}
struct mDoGph_gInf_c{static float getMinXF(){return -100;}static float getMaxXF(){return 900;}
 static float getMinYF(){return 0;}static float getMaxYF(){return 600;}};
struct Vertex{float x,y;u8 alpha;};std::vector<Vertex>vertices;
void GXPosition3f32(float x,float y,float){vertices.push_back({x,y,0});}
void GXColor4u8(int,int,int,u8 alpha){vertices.back().alpha=alpha;}
void GXBegin(int,int,int n){assert(n==16);}void GXEnd(){}
void GXClearVtxDesc(){}void GXSetVtxDesc(int,int){}void GXSetVtxAttrFmt(int,int,int,int,int){}
void GXSetNumChans(int){}void GXSetNumTexGens(int){}void GXSetNumTevStages(int){}
void GXSetTevOp(int,int){}void GXSetTevOrder(int,int,int,int){}void GXSetBlendMode(int,int,int,int){}
void GXSetZMode(int,int,int){}void GXSetCullMode(int){}
namespace remaster::hud::damage_feedback {
DamagePulse g_pulse;daAlink_c* g_owner=nullptr;std::string g_stage;int g_room=-1,g_life=-1;
double seconds(){return now;}
'''
damage_main=r'''
}
int main(){using namespace remaster;using namespace hud::damage_feedback;daAlink_c link;
 tick(&link);draw_pre(nullptr,nullptr,nullptr,nullptr);assert(vertices.empty());
 life=8;tick(&link);assert(g_pulse.start==10);draw_pre(nullptr,nullptr,nullptr,nullptr);assert(vertices.size()==16&&setups==2);
 int opaque=0,transparent=0;for(const auto& v:vertices){if(v.alpha){++opaque;assert(v.alpha==63);assert(v.x==-100||v.x==900||v.y==0||v.y==600);}else++transparent;}
 assert(opaque==8&&transparent==8); // centre stays clear, corners are covered without stacking quads
 now=10.1;tick(&link);assert(g_pulse.start==10); // no health loss, no new hit flash
 life=12;tick(&link);assert(g_pulse.start==10); // healing does not trigger feedback
 now=11;vertices.clear();draw_pre(nullptr,nullptr,nullptr,nullptr);assert(vertices.empty());
 life=7;tick(&link);assert(g_pulse.start==11);visible=true;
 draw_pre(nullptr,nullptr,nullptr,nullptr);assert(g_pulse.start==-1); // also clears when simulation is paused
 tick(&link);assert(g_pulse.start==-1);visible=false;
 life=6;tick(&link);assert(g_pulse.start==11);link.room=1;tick(&link);assert(g_pulse.start==-1);
 life=5;tick(&link);assert(g_pulse.start==11);currentStage="D_MN05";tick(&link);assert(g_pulse.start==-1);
 save_reset(nullptr,1,nullptr);life=3;tick(&link);assert(g_pulse.start==-1); // same actor/room after a load is not damage
 life=0;tick(&link);assert(g_pulse.start==-1);active=false;draw_pre(nullptr,nullptr,nullptr,nullptr);assert(vertices.empty());
 std::cout<<"Production damage feedback: actual health loss, no blocked/healing flash, clear-centre ultrawide geometry, UI/scene/death/mode isolation passed\n";
}
'''
run(common+gpu_constants+damage_fixture+''.join(function(damage,n) for n in ['draw_pre','reset','tick','save_reset'])+damage_main)
