"""Execute the spring interaction, dialog callbacks and one-time introduction."""
from fixture import ROOT, function, run
source=(ROOT/'src/progression/spring.cpp').read_text()
code=r'''
#include <cassert>
#include <cmath>
#include <cstring>
#include <format>
#include <string>
#include <vector>
#include "progression/model.hpp"
struct ModContext{};using UiDialogHandle=uint64_t;constexpr int MOD_OK=0,MOD_ERROR=1,PAD_1=0,PAD_BUTTON_Y=8,FALSE=0;using BOOL=int;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};ModContext* mod_ctx=nullptr;
bool active=true,input=true,safe=true,forest=false;uint32_t slot=0;int room=1,life=16,persisted=0;std::string stage="F_SP104";
bool is_active(){return active;}uint32_t dComIfGs_getDataNum(){return slot;}bool dComIfGs_isCollectCrystal(int){return forest;}
const char* dComIfGp_getStartStageName(){return stage.c_str();}
struct daAlink_c{enum{BTN_A=1,BTN_B=2,BTN_X=4,BTN_Y=8,PROC_WAIT,PROC_SERVICE_WAIT,PROC_TIRED_WAIT,PROC_CROUCH,PROC_WOLF_WAIT,PROC_WOLF_SERVICE_WAIT,PROC_WOLF_TIRED_WAIT,PROC_WOLF_SIT};int mItemTrigger=8,mProcID=PROC_WAIT;float mMoveValue=0,mNormalSpeed=0,speedF=0;struct{bool ChkGroundHit(){return true;}}mLinkAcch;}link;auto* daAlink_getAlinkActorClass(){return &link;}int fopAcM_GetRoomNo(daAlink_c*){return room;}
namespace mods{template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
struct mDoCPd_c{static inline bool held=true,trig=true;static bool getHoldLockL(int){return held;}static bool getTrigY(int){return trig;}struct Pad{int mPressedButtonFlags=8;};static Pad&getCpadInfo(int){static Pad pad;return pad;}};
namespace remaster::player{bool input_allowed(){return input&&active;}}
int dComIfGs_getLife(){return life;}
namespace remaster::healing::wolf{bool busy(){return false;}}
namespace remaster::healing::stones{PRODUCTION_IDLE bool rest_available(daAlink_c* link){return safe&&idle(link);}}
namespace remaster::progression{Model state;bool ready(){return active;}Model& model(){return state;}void persist(){++persisted;}}
namespace remaster::progression::growth{bool ensure(){return active;}void apply_health(){life=std::min<int>(life,(3+remaster::progression::state.healthPoints)*4);}}
namespace remaster::tuning{struct Values{int staminaMax=100;}cfg;auto& get(){return cfg;}}
namespace fmt{template<class...T>std::string format(const char* p,T...a){return std::vformat(p,std::make_format_args(a...));}}
struct UiDialogAction{const char* label;void(*on_pressed)(ModContext*,UiDialogHandle,void*);void* user_data;bool keep_open;bool(*is_disabled)(ModContext*,void*);};
#define UI_DIALOG_ACTION_INIT {nullptr,nullptr,nullptr,false,nullptr}
struct UiDialogDesc{const char* title;const char* body_rml;UiDialogAction* actions;size_t action_count;void(*on_dismiss)(ModContext*,UiDialogHandle,void*);};
#define UI_DIALOG_DESC_INIT {nullptr,nullptr,nullptr,0,nullptr}
struct UiToastDesc{const char* title_rml;const char* body_rml;};
#define UI_TOAST_DESC_INIT {nullptr,nullptr}
struct Ui{int opens=0,closes=0,toasts=0;bool fails=false;std::string body;std::vector<UiDialogAction>actions;
 int dialog_push(ModContext*,UiDialogDesc* d,UiDialogHandle* h){if(fails)return MOD_ERROR;++opens;*h=123;body=d->body_rml;actions.assign(d->actions,d->actions+d->action_count);return MOD_OK;}
 int dialog_set_body(ModContext*,UiDialogHandle,const char* s){body=s;return MOD_OK;}
 int dialog_close(ModContext*,UiDialogHandle){++closes;return MOD_OK;}
 int push_toast(ModContext*,UiToastDesc*){++toasts;return fails?MOD_ERROR:MOD_OK;}}ui;auto* svc_ui=&ui;
namespace remaster::progression::spring{UiDialogHandle g_dialog=0;uint32_t g_slot=255;
'''
code=code.replace('PRODUCTION_IDLE',function((ROOT/'src/healing/rest_stones.cpp').read_text(),'idle'))
code+=''.join(function(source,n) for n in ['here','valid_dialog','exchange_disabled','exchange_pressed','dismissed','reset','interact_pre','tick']).replace('description()','description()')
# description returns std::string rather than the scalar types in the fixture helper.
start=source.index('std::string description()');end=source.index('\nbool exchange_disabled',start)
code=code.replace('bool here()',source[start:end]+'\nbool here()',1)
code+='}\n'+r'''
int main(){using namespace remaster::progression;using namespace spring;state.migrate_growth(15);state.add_growth(5);state.allocate(false);
 auto* p=&link;void* args[]={&p};int result=1;
 assert(interact_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE&&ui.opens==0); // story lock
 forest=true;link.mItemTrigger=daAlink_c::BTN_Y|daAlink_c::BTN_A;
 assert(interact_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE&&!g_dialog&&link.mItemTrigger==9);link.mItemTrigger=8;
 ui.fails=true;assert(interact_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE&&!g_dialog);assert(link.mItemTrigger==8&&mDoCPd_c::getCpadInfo(0).mPressedButtonFlags==8);ui.fails=false;
 assert(interact_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE&&g_dialog&&g_slot==0&&result==1&&link.mItemTrigger==0&&mDoCPd_c::getCpadInfo(0).mPressedButtonFlags==0);
 assert(ui.actions.size()==3&&!exchange_disabled(nullptr,&g_slot)&&exchange_disabled(nullptr,nullptr));
 assert(here());const auto earned=state.growthFragments;exchange_pressed(nullptr,g_dialog,&g_slot);
 assert(state.healthPoints==0&&state.staminaPoints==1&&life==12&&state.growthFragments==earned&&ui.body.find("Stamina: 120")!=std::string::npos);
 exchange_pressed(nullptr,g_dialog,nullptr);assert(state.healthPoints==1&&state.staminaPoints==0&&life==12&&state.growthFragments==earned);
 slot=1;exchange_pressed(nullptr,g_dialog,&g_slot);assert(state.healthPoints==1);tick();assert(!g_dialog&&ui.closes==1);slot=0;
 for(int gate=0;gate<5;++gate){safe=gate!=0;stage=gate==1?"F_SP108":"F_SP104";room=gate==2?0:1;input=gate!=3;active=gate!=4;
 assert(interact_pre(nullptr,args,&result,nullptr)==HOOK_CONTINUE&&!g_dialog);}
 safe=input=active=true;stage="F_SP104";room=1;state.claimed.erase("intro:growth-spring");ui.fails=true;tick();assert(!state.claimed.contains("intro:growth-spring"));
 ui.fails=false;tick();int notices=ui.toasts;tick();assert(ui.toasts==notices&&state.claimed.contains("intro:growth-spring"));
 interact_pre(nullptr,args,&result,nullptr);safe=false;exchange_pressed(nullptr,g_dialog,&g_slot);assert(state.healthPoints==1);tick();assert(!g_dialog);
}
'''
run(code)
print('Spring: story/proximity/safety/slot gates, dialog failure, free conserved exchanges, no healing, stale-dialog closure and one-time introduction passed')
