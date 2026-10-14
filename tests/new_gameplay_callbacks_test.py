"""Execute production wolf, Snowpeak, recharge and slider callbacks without game assets."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
def function(source,name):
    m=re.search(r'^(?:HookAction|void|bool|int|uint32_t) '+name+r'\(',source,re.M)
    if not m: raise ValueError(name)
    start=m.start();at=source.index('{',start)+1;depth=1
    while depth:
        if source[at]=='{':depth+=1
        elif source[at]=='}':depth-=1
        at+=1
    return source[start:at]+'\n'
def run(source):
    with tempfile.TemporaryDirectory() as tmp:
        cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(source)
        subprocess.run(['c++','-std=c++20','-O0','-UNDEBUG','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
common=r'''
#include <algorithm>
#include <array>
#include <set>
#include <vector>
#include <string>
#include <string_view>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "resources/recharge_model.hpp"
#include "progression/model.hpp"
using u8=uint8_t;using u16=uint16_t;using f32=float;using fpc_ProcID=uint32_t;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};bool active=true,allowed=true,event=false;
bool is_active(){return active;}
namespace mods {template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}
template<class T>T& arg_ref(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
namespace remaster::player {bool input_allowed(){return allowed;}bool gameplay_running(){return allowed;}}
bool dComIfGp_event_runCheck(){return event;}
'''
wolf=(root/'src/healing/wolf.cpp').read_text()
wolf_fixture=r'''
int hp=4,maxHP=12,healCalls=0,persists=0;
int dComIfGs_getLife(){return hp;}int dComIfGs_getMaxLifeGauge(){return maxHP;}
void dComIfGp_setItemLifeCount(float amount,u8){++healCalls;hp=std::min(maxHP,hp+int(amount));}
namespace remaster::tuning {struct Values{bool flaskEnabled=true;int flaskStartingCharges=3,flaskMaximumCharges=8,flaskHealingHearts=3,wolfHealingWindupMs=2000;} cfg;auto& get(){return cfg;}}
namespace remaster::progression {Model save;bool ready(){return true;}Model& model(){return save;}void persist(){++persists;}}
struct daAlink_c {
 enum{PROC_WOLF_WAIT,PROC_WOLF_MOVE,PROC_WOLF_SIT,PROC_WOLF_TIRED_WAIT,PROC_WOLF_WAIT_TURN,PROC_ATTACK,WANM_WAIT_SIT};
 enum{BTN_A=1,BTN_B=2,BTN_X=4,BTN_Y=8};
 struct Acch{bool grounded=true;bool ChkGroundHit(){return grounded;}} mLinkAcch;
 bool wolf=true,initSucceeds=true;u16 mProcID=PROC_WOLF_WAIT;u8 mItemTrigger=15,mItemButton=15;int mDamageTimer=0;
 float mStickValue=1,mMoveValue=1,mNormalSpeed=1,speedF=1;int frames[1]{};int* mUnderFrameCtrl=frames;
 bool checkWolf(){return wolf;}bool checkAnmEnd(int*){return true;}bool checkUnderMove0BckNoArcWolf(int){return false;}
 void setSingleAnimeWolfBase(int){}int procWolfSitInit(int){if(initSucceeds)mProcID=PROC_WOLF_SIT;return initSucceeds;}
 void procWolfWaitInit(){mProcID=PROC_WOLF_WAIT;}
};
daAlink_c link;daAlink_c* daAlink_getAlinkActorClass(){return &link;}
namespace remaster::healing::wolf {
daAlink_c* g_owner=nullptr;int g_ticks=0,g_windup=60;bool g_healed=false;
void interrupt(daAlink_c*);void reset();
'''
wolf_names=['enabled','allowed_proc','lock_input','begin_recovery','combo_pre','input_post','sit_pre','busy','interrupt','reset','tick']
wolf_main=r'''
}
int main(){using namespace remaster;using namespace healing::wolf;
 auto* p=&link;void* args[]={&p};int result=0;
 assert(combo_pre(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&busy()&&progression::save.charges==2);
 assert(!event&&link.mDamageTimer==0&&link.mNormalSpeed==0&&link.mItemTrigger==0&&persists==1);
 for(int i=0;i<59;++i)sit_pre(nullptr,args,&result,nullptr);assert(hp==4&&healCalls==0);
 allowed=false;for(int i=0;i<100;++i)sit_pre(nullptr,args,&result,nullptr);assert(g_ticks==59&&healCalls==0);
 allowed=true;sit_pre(nullptr,args,&result,nullptr);assert(hp==12&&healCalls==1&&busy());
 for(int i=0;i<18;++i)sit_pre(nullptr,args,&result,nullptr);assert(!busy()&&link.mProcID==daAlink_c::PROC_WOLF_WAIT&&healCalls==1);
 hp=4;assert(begin_recovery(p)&&progression::save.charges==1);for(int i=0;i<30;++i)sit_pre(nullptr,args,&result,nullptr);
 interrupt(p);assert(!busy()&&hp==4&&progression::save.charges==1); // interrupted windup loses its charge
 assert(begin_recovery(p));event=true;sit_pre(nullptr,args,&result,nullptr);assert(!busy()&&hp==4&&progression::save.charges==0);
 event=false;assert(!begin_recovery(p));progression::save.charges=3;
 link.mProcID=daAlink_c::PROC_ATTACK;assert(!begin_recovery(p)&&progression::save.charges==3);
 link.mProcID=daAlink_c::PROC_WOLF_WAIT;link.mLinkAcch.grounded=false;assert(!begin_recovery(p));link.mLinkAcch.grounded=true;
 hp=maxHP;assert(!begin_recovery(p));hp=4;link.wolf=false;assert(combo_pre(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 link.wolf=true;active=false;assert(combo_pre(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 std::cout<<"Wolf recovery: shared charges, delay, interruption, menus, ground/action gates and mode isolation passed\n";
}
'''
run(common+wolf_fixture+''.join(function(wolf,n) for n in wolf_names)+wolf_main)

snow=(root/'src/healing/snowpeak_hearts.cpp').read_text()
snow_fixture=r'''
struct fopAc_ac_c{uint32_t id=0;};struct daItem_c:fopAc_ac_c{int item=0;};
namespace daItem_prm{int getItemNo(daItem_c* a){return a->item;}}
uint32_t fopAcM_GetID(const fopAc_ac_c* a){return a->id;}
std::string stage="D_MN11B";int param=13;bool bossDead=true;
const char* dComIfGp_getStartStageName(){return stage.c_str();}bool dComIfGs_isStageBossEnemy(){return bossDead;}
int* dComIfGp_evmng_getMyIntegerP(int,const char*){return &param;}
namespace remaster::healing::snowpeak {
std::vector<bool> g_scope;unsigned g_sources=0;std::set<fpc_ProcID> g_hearts;std::vector<bool> g_grants;unsigned g_grantDepth=0;
bool allows_spawn();
'''
snow_names=['snowpeak_stage','enter_cut','ending_pre','hug_pre','cut_post','grant_pre','grant_post','allows_spawn','allows_actor','allows_grant','reset']
snow_main=r'''
}
int main(){using namespace remaster::healing::snowpeak;
 int staff=0;void* npc=nullptr;void* args[]={&npc,&staff};daItem_c shower;shower.id=7;daItem_c ordinary;ordinary.id=8;
 bossDead=false;ending_pre(nullptr,args,nullptr,nullptr);assert(!allows_spawn());cut_post(nullptr,nullptr,nullptr,nullptr);
 bossDead=true;param=1;ending_pre(nullptr,args,nullptr,nullptr);assert(!allows_spawn());cut_post(nullptr,nullptr,nullptr,nullptr);
 param=13;ending_pre(nullptr,args,nullptr,nullptr);assert(allows_spawn()&&allows_actor(&shower));cut_post(nullptr,nullptr,nullptr,nullptr);
 assert(!allows_spawn()&&allows_actor(&shower)&&!allows_actor(&ordinary));
 u8 item=0;uint32_t tag=0;fopAc_ac_c* giver=&shower;void* grant[]={&item,&tag,&giver};
 grant_pre(nullptr,grant,nullptr,nullptr);assert(allows_grant());grant_post(nullptr,grant,nullptr,nullptr);assert(!allows_grant()&&!allows_actor(&shower));
 giver=&ordinary;grant_pre(nullptr,grant,nullptr,nullptr);assert(!allows_grant());grant_post(nullptr,grant,nullptr,nullptr);
 param=1;hug_pre(nullptr,args,nullptr,nullptr);assert(allows_spawn());param=7;ending_pre(nullptr,args,nullptr,nullptr);assert(allows_spawn());
 cut_post(nullptr,nullptr,nullptr,nullptr);assert(allows_spawn());cut_post(nullptr,nullptr,nullptr,nullptr);assert(!allows_spawn());
 stage="F_SP114";hug_pre(nullptr,args,nullptr,nullptr);assert(!allows_spawn());cut_post(nullptr,nullptr,nullptr,nullptr);
 stage="D_MN11B";active=false;hug_pre(nullptr,args,nullptr,nullptr);assert(!allows_spawn());cut_post(nullptr,nullptr,nullptr,nullptr);
 std::cout<<"Snowpeak: exact post-boss cut steps, nested scope and provenance-based heart healing passed\n";
}
'''
run(common+snow_fixture+''.join(function(snow,n) for n in snow_names)+snow_main)

ammo=(root/'src/resources/ammo.cpp').read_text()
item_data=(root/'dusklight/include/d/d_item_data.h').read_text()
ammo_names=['clear_clocks','gameplay','count','restore_bag','bomb_before','bomb_after','empty_bait_pre','selection_post','enabled','removes_pickup','damage_received','death','slot_changed','reset','refill','tick']
ammo_code=''.join(function(ammo,n) for n in ammo_names)
ids={n:int(v,16) for v,n in re.findall(r'/\* (0x[0-9A-Fa-f]+) \*/ (dItemNo_\w+)',item_data)}
needed=set(re.findall(r'dItemNo_\w+',ammo_code))
constants='\n'.join(f'constexpr int {n}={ids[n]};' for n in needed)
ammo_fixture=r'''
enum ModResult{MOD_OK};ModContext* mod_ctx=nullptr;bool visible=false;
struct UiMock{ModResult is_any_document_visible(ModContext*,bool* out){*out=visible;return MOD_OK;}} ui;
UiMock* svc_ui=&ui;
constexpr int SLOT_1=1,SLOT_4=4,SLOT_11=11,SLOT_14=14,SLOT_15=15,SLOT_17=17,SLOT_23=23;
namespace remaster::tuning {struct Values{
 bool ammoRecharge=true,removeConsumableDrops=true;
 int64_t arrowRechargeDelayMs=5000,arrowRechargeIntervalMs=1000,seedRechargeDelayMs=3000,seedRechargeIntervalMs=1000;
 int64_t bombRechargeDelayMs=8000,bombRechargeIntervalMs=4000,waterBombRechargeDelayMs=10000,waterBombRechargeIntervalMs=5000;
 int64_t bomblingRechargeDelayMs=10000,bomblingRechargeIntervalMs=6000,oilRechargeDelayMs=3000,oilRechargeIntervalMs=1000,baitRechargeDelayMs=3000,baitRechargeIntervalMs=2000;
 } cfg;auto& get(){return cfg;}}
struct daAlink_c{enum {PROC_DEAD=99};int mProcID=0;};
int hp=12,saveSlot=0,selected=15;u8 inventory[24];int arrow=5,arrowMax=30,arrowPending=0,seed=0,seedMax=50,seedPending=0,oil=2000,oilMax=2200,oilPending=0;
int bomb[3]{},bombPending[3]{},bottle[4]{};
u8 dComIfGs_getItem(int i,bool){return i>=0&&i<24?inventory[i]:255;}
void dComIfGs_setItem(int i,u8 v){inventory[i]=v;}void dComIfGp_setItem(int,u8){}
int dComIfGs_getLife(){return hp;}int dComIfGs_getDataNum(){return saveSlot;}
int dComIfGs_getArrowNum(){return arrow;}int dComIfGs_getArrowMax(){return arrowMax;}
int dComIfGp_getItemArrowNumCount(){return arrowPending;}void dComIfGp_setItemArrowNumCount(int i){arrowPending+=i;}
int dComIfGs_getPachinkoNum(){return seed;}int dComIfGs_getPachinkoMax(){return seedMax;}
int dComIfGp_getItemPachinkoNumCount(){return seedPending;}void dComIfGp_setItemPachinkoNumCount(int i){seedPending+=i;}
int dComIfGs_getBombMax(int type){return type==0x70?30:type==0x71?15:10;}
int dComIfGs_getBombNum(int i){return bomb[i];}int dComIfGp_getItemBombNumCount(int i){return bombPending[i];}
void dComIfGp_setItemBombNumCount(int i,int v){bombPending[i]+=v;}
int dComIfGs_getMaxOil(){return oilMax;}int dComIfGs_getOil(){return oil;}
int dComIfGp_getItemOilCount(){return oilPending;}void dComIfGp_setItemOilCount(int i){oilPending+=i;}
int dComIfGs_getBottleMax(){return 10;}int dComIfGs_getBottleNum(int i){return bottle[i];}
void dComIfGs_setBottleNum(int i,u8 n){bottle[i]=n;}void dComIfGs_addBottleNum(int i,int n){bottle[i]+=n;}
int dComIfGs_getSelectItemIndex(u8){return selected;}
namespace remaster::resources::ammo {
std::array<Recharge,6> g_clock;std::array<Recharge,4> g_bait;std::array<u8,3> g_beforeBags;
daAlink_c* g_owner=nullptr;int g_lastLife=-1;bool g_retry=false;uint32_t g_retrySlot=255;
bool enabled();void reset();void refill();
'''
ammo_main=r'''
}
int main(){using namespace remaster;using namespace resources::ammo;
 std::fill(std::begin(inventory),std::end(inventory),255);daAlink_c link;auto* p=&link;u8 selectId=0;int decision=12;void* args[]={&p,&selectId};
 for(int i=0;i<300;++i)tick(p);assert(arrowPending==0); // no bow means no hidden ammunition is granted
 inventory[SLOT_4]=0x43;reset();for(int i=0;i<179;++i)tick(p);assert(arrowPending==0);tick(p);assert(arrowPending==1);
 arrowPending=0;damage_received();for(int i=0;i<179;++i)tick(p);assert(arrowPending==0);tick(p);assert(arrowPending==1);
 arrowPending=0;reset();for(int i=0;i<100;++i)tick(p);visible=true;for(int i=0;i<500;++i)tick(p);assert(arrowPending==0);visible=false;
 for(int i=0;i<80;++i)tick(p);assert(arrowPending==1);
 // Native HUD can empty the last bomb's type; the production post-hook restores the original water type.
 inventory[SLOT_15]=0x71;bomb[0]=1;bombPending[0]=-1;bomb_before(nullptr,nullptr,nullptr,nullptr);
 inventory[SLOT_15]=0x50;bomb[0]=0;bombPending[0]=0;bomb_after(nullptr,nullptr,nullptr,nullptr);assert(inventory[SLOT_15]==0x71);
 selection_post(nullptr,args,&decision,nullptr);assert(decision==0); // preserved empty bag cannot produce free bombs
 reset();for(int i=0;i<449;++i)tick(p);assert(bombPending[0]==0);tick(p);assert(bombPending[0]==1);
 decision=12;selection_post(nullptr,args,&decision,nullptr);assert(decision==12);
 // Bait retains its type after the final use, and is unavailable until it has regenerated.
 inventory[SLOT_11]=0x76;bottle[0]=1;selected=SLOT_11;void* emptyArgs[]={&selectId};
 assert(empty_bait_pre(nullptr,emptyArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&bottle[0]==0&&inventory[SLOT_11]==0x76);
 decision=7;selection_post(nullptr,args,&decision,nullptr);assert(decision==0);
 reset();for(int i=0;i<149;++i)tick(p);assert(bottle[0]==0);tick(p);assert(bottle[0]==1);
 decision=7;selection_post(nullptr,args,&decision,nullptr);assert(decision==7);
 // Rest refills count pending expenditure and never exceeds the real native capacities.
 inventory[SLOT_23]=0x4b;inventory[SLOT_1]=0x48;arrow=arrowMax;arrowPending=-1;seed=40;seedPending=-5;
 oil=2195;oilPending=0;bomb[0]=14;bombPending[0]=-1;refill();assert(arrowPending==0&&seedPending==10&&oilPending==5&&bombPending[0]==1&&bottle[0]==10);
 // Real death sets a same-slot retry refill; ordinary load/switch does not.
 arrow=0;arrowPending=0;death();slot_changed(1,false);for(int i=0;i<10;++i)tick(p);assert(arrowPending==0);
 saveSlot=0;death();slot_changed(0,false);tick(p);assert(arrowPending==30);
 arrowPending=0;active=false;assert(!removes_pickup(0x0e));for(int i=0;i<300;++i)tick(p);assert(arrowPending==0);
 active=true;tuning::cfg.ammoRecharge=false;tick(p);assert(arrowPending==0);assert(removes_pickup(0x0e)&&!removes_pickup(0x43));
 std::cout<<"Production ammo callbacks: ownership, queued counts, menus, empty bag/bait preservation, zero gates and retry isolation passed\n";
}
'''
run(common+constants+'\n'+ammo_fixture+ammo_code+ammo_main)

# Curated permanent rewards must remain complete when adding relic placements.
rewards=(root/'src/progression/rewards.cpp').read_text()
flask=rewards[rewards.index('kFlaskChecks ='):rewards.index('};',rewards.index('kFlaskChecks ='))]
flask_checks=set(re.findall(r'"(chest:[^"]+)"',flask))
relic_checks=set(re.findall(r'"(chest:[^"]+)"',(root/'src/economy/merchant_model.hpp').read_text()))
assert len(flask_checks)==20 and len(relic_checks)==4 and not flask_checks.intersection(relic_checks)
print('Twenty flask shard placements and four unique relics do not overlap')

# Exercise the actual RmlUi/config binding, including failed writes and external resets.
slider=(root/'src/core/tuning.cpp').read_text()
slider_fixture=r'''
#include "core/slider_model.hpp"
enum {MOD_OK,MOD_ERROR};ModContext* mod_ctx=nullptr;
struct Input{std::string id,value;};
const std::string& input_id(const void* p){return static_cast<const Input*>(p)->id;}
std::string input_value(const void* p){return static_cast<const Input*>(p)->value;}
void input_set(void* p,const std::string& value){static_cast<Input*>(p)->value=value;}
int64_t timing=5000;int writes=0;bool fail=false;std::string label;
struct Config{int set_int(ModContext*,int,int64_t value){++writes;if(fail)return MOD_ERROR;timing=value;return MOD_OK;}} config;
struct Ui{void elem_set_text(ModContext*,int,const char* value){label=value;}} ui;
auto* svc_config=&config;auto* svc_ui=&ui;
namespace fmt{template<class...T>std::string format(const char*,T...){return "timing label";}}
namespace remaster::tuning {
struct Spec{bool slider;const char* name;int64_t min,max,step;int handle;int64_t lastSliderValue;int64_t* intTarget;int sliderLabel;const char* label;const char* suffix;};
Spec g_specs[]={{true,"arrowRechargeDelayMs",0,60000,100,1,5000,&timing,1,"Delay"," ms"}};
auto g_sliderId=input_id;auto g_sliderGet=input_value;auto g_sliderSet=input_set;
'''
slider_main=r'''
}
int main(){using namespace remaster::tuning;
 Input input{"other-mod-slider","8000"};void* p=&input;void* args[]={&p};
 slider_after(nullptr,args,nullptr,nullptr);assert(writes==0&&timing==5000);
 input.id="tp-definitive-timing-arrowRechargeDelayMs";input.value="8150";
 slider_after(nullptr,args,nullptr,nullptr);assert(writes==1&&timing==8200&&g_specs[0].lastSliderValue==8200&&!label.empty());
 input.value="8200";slider_after(nullptr,args,nullptr,nullptr);assert(writes==1);
 timing=3000;slider_after(nullptr,args,nullptr,nullptr);assert(input.value=="3000"&&g_specs[0].lastSliderValue==3000&&writes==1);
 input.value="4000";fail=true;slider_after(nullptr,args,nullptr,nullptr);assert(timing==3000&&g_specs[0].lastSliderValue==3000);
 fail=false;slider_after(nullptr,args,nullptr,nullptr);assert(timing==4000&&g_specs[0].lastSliderValue==4000);
 const int before=writes;input.value="nan";slider_after(nullptr,args,nullptr,nullptr);assert(writes==before&&timing==4000);
 std::cout<<"Production sliders: owned IDs, rounding, saved settings, external reset and failed-write retry passed\n";
}
'''
run(common+slider_fixture+function(slider,'slider_after')+slider_main)

# Queueing/previewing a reward must not spend its relic or persist a quest before delivery.
merchant=(root/'src/economy/merchants.cpp').read_text()
merchant_names=['owns_reward','trade_available','queue','enabled','custom_item','resolve','message','on_give','reset','tick']
merchant_code=''.join(function(merchant,n) for n in merchant_names)
needed=set(re.findall(r'dItemNo_\w+',merchant_code))
constants='\n'.join(f'constexpr int {n}={ids[n]};' for n in needed)
merchant_fixture=r'''
#include "economy/merchant_model.hpp"
enum {MOD_OK,MOD_ERROR};constexpr int COLLECT_HYLIAN_SHIELD=2,SLOT_5=5,SLOT_15=15,SLOT_23=23;
ModContext* mod_ctx=nullptr;int gives=0,persists=0;bool giveFails=false;std::string queued;
struct Items{int give_item(ModContext*,const char* name,uint8_t,int){++gives;queued=name;return giveFails?MOD_ERROR:MOD_OK;}} itemService;
auto* svc_item=&itemService;
struct Ui{void elem_set_text(ModContext*,int,const char*){}} ui;auto* svc_ui=&ui;
namespace fmt{template<class...T>std::string format(const char*,T...){return "status";}}
namespace remaster::tuning{struct Values{bool removeCurrency=true,merchantRewards=true;} cfg;auto& get(){return cfg;}}
namespace remaster::progression{Model state;bool ready(){return true;}Model& model(){return state;}void persist(){++persists;}}
std::string stage="R_SP109";int room=3;u8 inventory[24];std::set<int> shield,clothes,firstBits,eventBits,crystals;int bombMax[3]={30,15,10};
const char* dComIfGp_getStartStageName(){return stage.c_str();}
bool dComIfGs_isCollectShield(int n){return shield.contains(n);}void dComIfGs_setCollectShield(int n){shield.insert(n);}
bool dComIfGs_isCollectClothes(int n){return clothes.contains(n);}void dComIfGs_setCollectClothes(int n){clothes.insert(n);}
bool dComIfGs_isItemFirstBit(int n){return firstBits.contains(n);}
bool dComIfGs_isEventBit(int n){return eventBits.contains(n);}void dComIfGs_onEventBit(int n){eventBits.insert(n);}
bool dComIfGs_isCollectCrystal(int n){return crystals.contains(n);}
u8 dComIfGs_getItem(int n,bool){return inventory[n];}
int dComIfGs_getBombMax(int n){return bombMax[n-0x70];}void dComIfGs_setBombMax(int n,int v){bombMax[n-0x70]=v;}
struct daAlink_c{enum{PROC_GET_ITEM=5};int mProcID=PROC_GET_ITEM;float mNormalSpeed=0;struct{u8 field_0x300c=0;}mProcVar2;} link;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}int fopAcM_GetRoomNo(daAlink_c*){return room;}
struct ItemCheckInfo{const char* name;uint8_t vanilla_item;};struct ItemCheckResolution{uint8_t item,display;bool animation;};
struct ItemGiveInfo{const char* check_name;uint8_t item;};
namespace remaster::economy::merchants{
std::set<std::string> g_pending;std::array<uint32_t,4> g_messages{10000,10001,10002,10003};int g_status=0;
bool enabled();
'''
merchant_main=r'''
}
int main(){using namespace remaster;using namespace economy::merchants;
 std::fill(std::begin(inventory),std::end(inventory),255);ItemCheckInfo check{kRelics[0].check,7};ItemCheckResolution out{};
 assert(resolve(&check,&out)&&out.item==kRelics[0].id&&progression::state.claimed.empty()&&persists==0);
 assert(!trade_available(kRelics[0]));ItemGiveInfo relic{kRelics[0].check,kRelics[0].id};on_give(&relic);
 assert(trade_available(kRelics[0]));room=1;assert(!trade_available(kRelics[0]));room=3;
 giveFails=true;assert(!queue(kRelics[0].trade,kRelics[0].reward)&&g_pending.empty()&&!redeemed(progression::state,kRelics[0]));
 giveFails=false;assert(queue(kRelics[0].trade,kRelics[0].reward)&&!redeemed(progression::state,kRelics[0])&&!trade_available(kRelics[0]));
 assert(!queue(kRelics[0].trade,kRelics[0].reward));
 ItemGiveInfo upgrade{kRelics[0].trade,kRelics[0].reward};on_give(&upgrade);
 assert(shield.contains(2)&&redeemed(progression::state,kRelics[0])&&g_pending.empty()&&!trade_available(kRelics[0]));
 progression::Model loaded;assert(progression::decode(progression::encode(progression::state),loaded)&&redeemed(loaded,kRelics[0]));
 ItemGiveInfo bag{kRelics[2].trade,kRelics[2].reward};on_give(&bag);assert(bombMax[0]==60&&bombMax[1]==30&&bombMax[2]==20);
 ItemGiveInfo armor{kRelics[3].trade,kRelics[3].reward};on_give(&armor);assert(clothes.contains(2));
 // ID 0x37's vanilla message overlaps Heart Piece text. Only our actual get-item remaps it.
 link.mProcVar2.field_0x300c=0x37;assert(message(&link,0x9c)==10001);link.mProcVar2.field_0x300c=8;assert(message(&link,0x9c)==0x9c);
 link.mProcID=0;link.mProcVar2.field_0x300c=0x37;assert(message(&link,0x9c)==0x9c);
 progression::state={};reset();eventBits.clear();clothes.clear();allowed=true;tick();assert(g_pending.empty());
 ItemGiveInfo bow{"bow_chest",dItemNo_BOW_e};on_give(&bow);assert(g_pending.contains("quest:bow_hawkeye"));
 assert(!progression::state.claimed.contains("quest:bow_hawkeye"));
 reset();firstBits.insert(dItemNo_BOW_e);tick();assert(g_pending.contains("quest:bow_hawkeye"));
 inventory[SLOT_5]=dItemNo_HAWK_EYE_e;ItemGiveInfo hawkeye{"quest:bow_hawkeye",dItemNo_HAWK_EYE_e};on_give(&hawkeye);
 assert(progression::state.claimed.contains("quest:bow_hawkeye")&&!g_pending.contains("quest:bow_hawkeye"));
 firstBits.clear();reset();tick();assert(g_pending.empty());
 ItemGiveInfo sera{"sera_reward",dItemNo_HALF_MILK_BOTTLE_e};on_give(&sera);tick();assert(g_pending.contains("quest:sera_slingshot")&&!progression::state.claimed.contains("quest:sera_slingshot"));
 reset();tick();assert(g_pending.contains("quest:sera_slingshot")); // interrupted queue/load remains eligible
 ItemGiveInfo slingshot{"quest:sera_slingshot",dItemNo_PACHINKO_e};inventory[SLOT_23]=dItemNo_PACHINKO_e;on_give(&slingshot);assert(eventBits.contains(0x4902));
 crystals.insert(1);tick();assert(g_pending.contains("quest:fyrus_bomb_bag"));
 reset();inventory[SLOT_15]=0x70;clothes.insert(1);crystals.insert(2);tick();assert(g_pending.contains("quest:zora_water_bombs")&&g_pending.contains("quest:lakebed_bomblings"));
 reset();active=false;tick();assert(g_pending.empty()&&!resolve(&check,&out));
 std::cout<<"Production rewards: preview safety, delayed grants, non-farmable trades, native ownership, quest unlocks and message isolation passed\n";
}
'''
run(common+constants+'\n'+merchant_fixture+merchant_code+merchant_main)
