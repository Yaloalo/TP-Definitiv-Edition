"""Exercise actual combat input/technique callbacks against explicit native doubles."""
from fixture import ROOT, function, run
import re
src=(ROOT/'src/combat/techniques.cpp').read_text()
def extract(name):
    match=re.search(r'^(?:int64_t|uint64_t|Attack) '+name+r'\(',src,re.M)
    if not match:return function(src,name)
    start=match.start();end=src.index('{',start)+1;depth=1
    while depth:
        depth+=(src[end]=='{')-(src[end]=='}');end+=1
    return src[start:end]+'\n'
fixture=r'''
#include <cassert>
#include <algorithm>
#include <string>
#include <cstdint>
#include "combat/technique_model.hpp"
using BOOL=int; constexpr bool TRUE=true;
struct ModContext{}; enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
constexpr int dItemNo_IRONBALL_e=66;
struct daPy_py_c {static constexpr int FLG3_TRANING_CUT_LARGE_TURN=1;static bool checkShieldGet(){return true;}};
struct dSv_event_flag_c {static constexpr int F_0344=1;};
bool active=true, inputAllowed=true, learned=true, battle=true;int life=12;
bool dComIfGs_isEventBit(int){return learned;} int dComIfGs_getLife(){return life;}
struct daAlink_c;
namespace remaster::combat::techniques {enum class Attack {Ordinary,Heavy,GuardCounter};int64_t finish_cost(int64_t);}
float staminaAmount=100;bool exhausted=false;int charges=0;
struct daAlink_c {
 enum {PROC_WAIT,PROC_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,PROC_GUARD_SLIP,PROC_CUT_FINISH,PROC_OTHER};
 int mProcID=PROC_WAIT,mEquipItem=0x103,field_0x2fd5=3,started=-1,breaks=0,clears=0;
 float field_0x3484=5,field_0x347c=20;
 bool wolf=false,event=false,ride=false,grab=false,equip=false,throwing=false,blockedMode=false,training=false,initSucceeds=true;
 struct Ground{bool ground=true;bool ChkGroundHit(){return ground;}} mLinkAcch;
 struct Frame{float rate=1;void setRate(float v){rate=v;}float getRate(){return rate;}}mUnderFrameCtrl[1],mUpperFrameCtrl[1];
 bool checkWolf(){return wolf;}bool checkEventRun(){return event;}bool checkRideOn(){return ride;}
 bool checkGrabAnime(){return grab;}bool checkEquipAnime(){return equip;}bool checkUpperReadyThrowAnime(){return throwing;}
 bool checkModeFlg(int){return blockedMode;}bool checkNoResetFlg3(int){return training;}
 static bool checkNotBattleStage(){return !battle;}
 void clearComboReserb(){++clears;}void resetCombo(bool){}
 void procGuardBreakInit(){++breaks;mProcID=PROC_OTHER;}
 int procCutFinishInit(int kind){if(!initSucceeds)return 0;started=kind;mProcID=PROC_CUT_FINISH;
  staminaAmount=std::max(0.f,staminaAmount-float(remaster::combat::techniques::finish_cost(12)));++charges;return 1;}
};
daAlink_c* nativeLink=nullptr;daAlink_c* daAlink_getAlinkActorClass(){return nativeLink;}
namespace mods {template<class T>T arg(void* a,int){return *static_cast<T*>(a);}}
namespace remaster {
 bool is_active(){return active;}
 namespace player {bool input_allowed(){return inputAllowed;}}
 namespace tuning {struct Values{bool combatTechniques=true;int64_t costHeavy=28,costGuardCounter=25,costBlock=18;}v;const auto& get(){return v;}}
 namespace combat::stamina {struct State{float current;bool exhausted;};State state(){return{staminaAmount,::exhausted};}
 void drain(float n,const char*){staminaAmount=std::max(0.f,staminaAmount-n);exhausted=staminaAmount<=0;}}
 namespace combat::techniques {
 InputState g_input;Attack g_start=Attack::Ordinary,g_attack=Attack::Ordinary;uint64_t g_serial=0;daAlink_c* g_owner=nullptr;
'''
functions=''.join(extract(n) for n in ['enabled','ready','finish_cost','try_action','cut_action_pre','great_spin_pre','ordinary_shield','block_received','guard_pre','guard_post','small_guard_post','cut_type_post','current_attack','attack_serial'])
checks=r'''
}}
int main(){
 using namespace remaster::combat::techniques;
 InputState input;
 input.sample(true,true,1);assert(input.pending==0); // reconnect with LB held never attacks
 input.sample(false,true,1);input.sample(true,true,1);assert(input.pending==6);
 input.sample(true,true,1);assert(input.pending==5); // held cannot repeat
 input.sample(true,false,1);input.sample(true,true,1);assert(!input.pending&&!input.counter);
 input.sample(false,true,1);input.sample(true,true,1);assert(input.pending);
 input.blocked(30);input.sample(false,true,2);assert(!input.pending&&!input.counter);
 daAlink_c link;nativeLink=g_owner=&link;daAlink_c* args=&link;int result=0;
 g_input.pending=6;assert(try_action(&link,result)&&result==1&&link.started==1);
 assert(charges==1&&staminaAmount==72&&g_start==Attack::Ordinary&&g_input.pending==0);
 assert(link.mUnderFrameCtrl[0].rate==0.75f&&link.field_0x3484==26);
 assert(!try_action(&link,result));
 link.mProcID=daAlink_c::PROC_WAIT;g_input.pending=6;g_input.blocked(30);
 assert(try_action(&link,result)&&link.started==5&&charges==2&&staminaAmount==47);
 // Cannot cancel another action or start during UI, events, Wolf, airborne or while using an item.
 for(int context=0;context<8;++context){
  link=daAlink_c{};inputAllowed=true;active=true;g_input.pending=6;
  if(context==0)link.mProcID=daAlink_c::PROC_OTHER;
  if(context==1)link.wolf=true;if(context==2)link.event=true;if(context==3)inputAllowed=false;
  if(context==4)link.mLinkAcch.ground=false;if(context==5)link.mEquipItem=3;
  if(context==6)link.ride=true;if(context==7)active=false;
  assert(!try_action(&link,result)&&charges==2);
 }
 active=inputAllowed=true;link=daAlink_c{};staminaAmount=0;exhausted=true;g_input.pending=6;
 assert(try_action(&link,result)&&result==0&&charges==2&&!g_input.pending);
 result=1;assert(cut_action_pre(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0&&link.clears==1);
 link.event=true;assert(cut_action_pre(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);link.event=false;
 staminaAmount=100;exhausted=false;link.initSucceeds=false;g_input.pending=6;
 assert(try_action(&link,result)&&!result&&charges==2&&staminaAmount==100);
 link=daAlink_c{};result=1;guard_pre(nullptr,&args,nullptr,nullptr);assert(link.field_0x2fd5==0);
 link.mProcID=daAlink_c::PROC_GUARD_SLIP;guard_post(nullptr,&args,&result,nullptr);
 assert(staminaAmount==82&&g_input.counter==30);result=0;guard_post(nullptr,&args,&result,nullptr);assert(staminaAmount==82);
 staminaAmount=10;small_guard_post(nullptr,&args,nullptr,nullptr);assert(link.breaks==1&&staminaAmount==0&&!g_input.counter);
 // Learned Great Spin works below full health; unlearned skills and vanilla remain untouched.
 learned=true;life=4;result=0;assert(great_spin_pre(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==1);
 learned=false;assert(great_spin_pre(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&!result);
 link.training=true;assert(great_spin_pre(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result);
 active=false;assert(great_spin_pre(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
}
'''
run(fixture+functions+checks)
# Actual input backend: detect native mapping conflicts and use only the assigned player's pad.
source=(ROOT/'src/input/combat_controller.cpp').read_text()
start=source.index('Sample sample()');end=source.index('\nModResult install()',start)
run(r'''
#include <cassert>
#include <cstdint>
struct PADButtonMapping{uint32_t nativeButton;int padButton;};struct PADAxisMapping{int axis,nativeButton,padAxis;};
namespace tuning {struct Values{int heavyControllerButton=9;}v;auto& get(){return v;}}
struct Sample{uint32_t device=0;bool heavy=false,focused=false;};
int id=17;bool focus=true,held=true;PADButtonMapping mapping{99,1};PADAxisMapping axis{0,-1,0};
void* g_window(){return reinterpret_cast<void*>(1);}uint64_t g_flags(void*){return focus?0x200:0;}
int g_instance(uint32_t slot){assert(slot==0);return id;}
void* g_gamepad(uint32_t which){assert(which==17);return reinterpret_cast<void*>(2);}
bool g_button(void* p,int b){assert(p==reinterpret_cast<void*>(2)&&b==9);return held;}
PADButtonMapping* g_buttons(uint32_t,uint32_t* n){*n=1;return &mapping;}
PADAxisMapping* g_axes(uint32_t,uint32_t* n){*n=1;return &axis;}
'''+source[start:end]+r'''
int main(){auto s=sample();assert(s.heavy&&s.device==17&&s.focused);
 mapping.nativeButton=9;assert(!sample().heavy);mapping.nativeButton=99;
 axis.nativeButton=9;assert(!sample().heavy);axis.nativeButton=-1;
 focus=false;assert(!sample().focused);id=-1;assert(sample().device==0&&!sample().heavy);}
''')
print('Combat techniques: dedicated actions, input isolation, costs, blocking, Great Spin and native mapping checks passed')
