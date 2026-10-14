"""Production NPC routing/dialog/input callbacks: context and original story passthrough."""
import re
from fixture import ROOT, run
source=(ROOT/'src/economy/conversations.cpp').read_text()
def extract(name):
 m=re.search(r'^(?:void\*|fopAc_ac_c\*|const char\*|std::string|HookAction|void|bool|Kind) '+re.escape(name)+r'\(',source,re.M)
 assert m,name
 a=m.start();b=source.index('{',a)+1;depth=1
 while depth:
  depth+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[a:b]+'\n'
selected=''.join(extract(n) for n in ['kind','safe','threat','find','nearby','valid','title','body','disabled','closed','pressed','open','interact_pre','reset','tick'])
state= source[source.index('enum class Kind'):source.index('Kind kind(')]
state+=source[source.index('struct Search'):source.index('void* find(')]
profiles=set(re.findall(r'fpcNm_\w+',selected))
constants='\n'.join(f'constexpr int {name}={i+10};' for i,name in enumerate(sorted(profiles)))
code=r'''
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <format>
#include <chrono>
#include "economy/rare_model.hpp"
#include "economy/merchant_model.hpp"
#include "economy/guild_model.hpp"
struct ModContext{};ModContext* mod_ctx=nullptr;using fpc_ProcID=int;using UiDialogHandle=int;constexpr int fpcM_ERROR_PROCESS_ID_e=-1,MOD_OK=0,MOD_ERROR=1,PAD_1=0,PAD_BUTTON_A=1,PAD_BUTTON_Y=2,fopAc_ENEMY_e=2,dItemNo_KANTERA_e=0x48;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct Vec{float x=0;float abs(Vec other){return std::abs(x-other.x);}};
struct fopAc_ac_c{int id=1,profile=0,room=3,health=1,group=0,angle=0;struct{Vec pos;}current;};
struct daAlink_c:fopAc_ac_c{enum{PROC_WAIT,PROC_SERVICE_WAIT,PROC_TIRED_WAIT,PROC_ATTACK,BTN_A=1,BTN_Y=2};bool wolf=false;bool checkWolf(){return wolf;}struct{bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;float mNormalSpeed=0;int mProcID=PROC_WAIT,mItemTrigger=7;}link;
std::vector<fopAc_ac_c*>actors;std::string stage="R_SP109";unsigned slot=0;bool mart=true;bool daNpcF_chkEvtBit(int id){assert(id==124);return mart;}
bool running=true,allowed=true,active=true,readySave=true,lantern=false;int presses=3;bool lockL=false,uiFail=false;int trades=0,purchases=0,presented=0,claimed=0,closes=0;
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}int fopAcM_GetRoomNo(fopAc_ac_c* a){return a->room;}int fopAcM_GetGroup(fopAc_ac_c* a){return a->group;}int fopAcM_GetID(fopAc_ac_c* a){return a->id;}
int fopAcM_seenActorAngleY(fopAc_ac_c*,fopAc_ac_c* a){return a->angle;}
fopAc_ac_c* fopAcM_SearchByID(int id){for(auto*a:actors)if(a->id==id)return a;return nullptr;}
void* fopAcIt_Judge(void*(*f)(void*,void*),void* d){for(auto*a:actors)if(auto*r=f(a,d))return r;return nullptr;}
auto* daAlink_getAlinkActorClass(){return &link;}const char* dComIfGp_getStartStageName(){return stage.c_str();}unsigned dComIfGs_getDataNum(){return slot;}bool dComIfGs_isItemFirstBit(int){return lantern;}
struct Pad{int mPressedButtonFlags=3;}pad;
struct mDoCPd_c{static bool getTrigA(int){return pad.mPressedButtonFlags&1;}static bool getTrigY(int){return pad.mPressedButtonFlags&2;}static bool getHoldLockL(int){return lockL;}static Pad&getCpadInfo(int){return pad;}};
namespace mods{template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
namespace fmt{template<class...T>std::string format(std::string_view f,T...args){return std::vformat(f,std::make_format_args(args...));}}
namespace remaster::player{bool input_allowed(){return allowed;}bool gameplay_running(){return running;}}
namespace remaster::progression{Model state;bool ready(){return readySave;}auto& model(){return state;}}
namespace remaster::progression::collections{bool enabled(){return active;}bool ensure(){return readySave;}int pairs(){return 3;}int souls(){return 20;}int unpresented(){return 2;}bool rewards_available(bool){return true;}void present_bugs(){++presented;}void claim_rewards(bool){++claimed;}}
namespace remaster::economy::rare{bool enabled(){return active;}bool ensure(){return readySave;}bool can_purchase(const Offer&){return progression::state.rareRupees>=60;}void purchase(const Offer&){++purchases;}}
namespace remaster::economy::guild{bool can_craft(const Recipe&){return false;}void craft(const Recipe&){}}
namespace remaster::economy::merchants{bool enabled(){return active;}bool can_trade(const Relic&){return true;}void trade(const Relic&){++trades;}}
struct UiDialogAction{const char*label=nullptr;void(*on_pressed)(ModContext*,UiDialogHandle,void*)=nullptr;void*user_data=nullptr;bool keep_open=false;bool(*is_disabled)(ModContext*,void*)=nullptr;};
#define UI_DIALOG_ACTION_INIT {}
struct UiDialogDesc{const char*title=nullptr;const char*body_rml=nullptr;UiDialogAction*actions=nullptr;unsigned action_count=0;void(*on_dismiss)(ModContext*,UiDialogHandle,void*)=nullptr;};
#define UI_DIALOG_DESC_INIT {}
struct Ui{std::vector<UiDialogAction>actions;std::string text;int dialog_push(ModContext*,UiDialogDesc*d,int*h){if(uiFail)return MOD_ERROR;actions.assign(d->actions,d->actions+d->action_count);text=d->body_rml;*h=1;return MOD_OK;}void dialog_set_body(ModContext*,int,const char*t){text=t;}void dialog_close(ModContext*,int){++closes;}}ui;auto*svc_ui=&ui;
'''+constants+'\nnamespace remaster::economy::conversations{\n'+state+'\n'+selected+r'''
}
int main(){using namespace remaster;using namespace economy::conversations;
 fopAc_ac_c malo{2,fpcNm_NPC_MARO_e,3};malo.current.pos.x=100;actors={&malo};link.room=3;auto*p=&link;void*args[]={&p};
 assert(nearby()==&malo);uiFail=true;interact_pre(nullptr,args,nullptr,nullptr);assert(pad.mPressedButtonFlags==3&&link.mItemTrigger==7&&!g_dialog);
 uiFail=false;interact_pre(nullptr,args,nullptr,nullptr);assert(g_dialog&&pad.mPressedButtonFlags==0&&link.mItemTrigger==4&&ui.actions.size()==5);
 assert(ui.actions[0].is_disabled(nullptr,ui.actions[0].user_data));progression::state.rareRupees=60;assert(!ui.actions[0].is_disabled(nullptr,ui.actions[0].user_data));
 // Moving away, changing slot/room/profile or an enemy invalidates even saved callbacks.
 auto buy=ui.actions[0];malo.current.pos.x=281;buy.on_pressed(nullptr,1,buy.user_data);assert(purchases==0);malo.current.pos.x=100;
 ++slot;assert(!valid());--slot;malo.profile=fpcNm_NPC_BANS_e;assert(!valid());malo.profile=fpcNm_NPC_MARO_e;
 stage="R_SP160";assert(!valid());stage="R_SP109";link.room=1;assert(!valid());link.room=3;
 buy.on_pressed(nullptr,1,buy.user_data);assert(purchases==1);
 auto native=ui.actions[3];native.on_pressed(nullptr,1,native.user_data);assert(!g_dialog&&g_native==2);
 pad.mPressedButtonFlags=1;link.mItemTrigger=7;interact_pre(nullptr,args,nullptr,nullptr);assert(!g_dialog&&pad.mPressedButtonFlags==1&&link.mItemTrigger==7&&g_native==-1); // native A unaltered
 // Opening requires a supported nearby NPC, facing, safe human idle and no threat.
 fopAc_ac_c enemy{3,0,3,1,fopAc_ENEMY_e};enemy.current.pos.x=2500;actors.push_back(&enemy);assert(!nearby());enemy.health=0;assert(nearby());
 for(int gate=0;gate<7;++gate){link.wolf=gate==0;link.mNormalSpeed=gate==1?1:0;link.mLinkAcch.ground=gate!=2;link.mProcID=gate==3?daAlink_c::PROC_ATTACK:daAlink_c::PROC_WAIT;running=gate!=4;readySave=gate!=5;active=gate!=6;assert(!nearby());}
 link.wolf=false;link.mNormalSpeed=0;link.mLinkAcch.ground=true;link.mProcID=daAlink_c::PROC_WAIT;running=readySave=active=true;
 malo.angle=0x3000;assert(!nearby());malo.angle=0;malo.room=1;assert(!nearby());malo.room=3;
 fopAc_ac_c closer{4,fpcNm_NPC_MARO_e,3};closer.current.pos.x=50;actors.push_back(&closer);assert(nearby()==&closer);actors.pop_back();
 // Initial lantern and cat/milk story remain native until their original milestone.
 stage="F_SP108";malo.room=4;link.room=4;malo.profile=fpcNm_NPC_KKRI_e;assert(kind(&malo)==Kind::None);lantern=true;assert(kind(&malo)==Kind::Coro);
 stage="R_SP01";malo.room=1;link.room=1;malo.profile=fpcNm_NPC_SEIRA_e;assert(kind(&malo)==Kind::None);progression::state.claim("quest:sera_helped",255);assert(kind(&malo)==Kind::Sera);
 stage="R_SP160";malo.room=2;link.room=2;malo.profile=fpcNm_NPC_INS_e;assert(open(&malo));assert(ui.actions.size()==4);ui.actions[0].on_pressed(nullptr,1,ui.actions[0].user_data);assert(presented==1);
 ui.actions[1].on_pressed(nullptr,1,ui.actions[1].user_data);assert(claimed==1);reset(nullptr,0,nullptr);assert(!g_dialog&&g_choices.empty()&&closes==1);
 malo.profile=fpcNm_NPC_POUYA_e;assert(kind(&malo)==Kind::Jovani);assert(open(&malo));slot=1;tick();assert(!g_dialog&&closes==2);
 g_native=2;g_nativeUntil=Clock::now()-std::chrono::seconds(1);tick();assert(g_native==-1);
}
'''
run(code)
print('NPC conversations: contextual purchases/gifts, facing/threat/slot gates, failure-safe A consumption, native story passthrough and modal lifecycle passed')
