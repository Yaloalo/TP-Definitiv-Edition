"""Execute the actual flask/drop/currency callbacks with game-free engine fixtures."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]

def function(source, name):
    match=re.search(r'^(?:HookAction|void|bool) '+name+r'\(', source, re.M)
    if not match: raise ValueError(name)
    end=source.index('{',match.start()); depth=1; at=end+1
    while depth:
        if source[at]=='{': depth+=1
        elif source[at]=='}': depth-=1
        at+=1
    return source[match.start():at]+'\n'

src=(root/'src/healing/flask.cpp').read_text()
selected=['healing_content','drink_init','life_pre','common_pre','common_post','forbidden','drop_pre','fast_drop_pre','general_drop_pre','field_item_pre','mounted_drink_pre','ring_count','ring_max','death_post','damage_interrupted','normalize_bottles','shop_arc_before','shop_sold_out_before']
callbacks=''.join(function(src,x) for x in selected)
ids={name: int(v,16) for v,name in re.findall(r'/\* (0x[0-9A-Fa-f]+) \*/ (dItemNo_\w+)',(root/'dusklight/include/d/d_item_data.h').read_text())}
rewards=(root/'src/progression/rewards.cpp').read_text()
reward_callbacks=''.join(function(rewards,n) for n in ['rupee','resolve','item_pre'])
checks=rewards[rewards.index('constexpr std::array<std::string_view, 12> kFlaskChecks'):rewards.index('};',rewards.index('constexpr std::array<std::string_view, 12> kFlaskChecks'))+2]
used=set(re.findall(r'dItemNo_\w+', callbacks+reward_callbacks)) | {"dItemNo_WATER_BOTTLE_e"}
enums='\n'.join(f'constexpr int {name}={ids[name]};' for name in sorted(used))
fixture=r'''
#include <algorithm>
#include <array>
#include <string_view>
#include <cassert>
#include <cstdint>
#include <iostream>
#include "progression/model.hpp"
using u8=uint8_t; using u16=uint16_t; using u32=uint32_t; using f32=float; using fpc_ProcID=uint32_t;
constexpr int cPhs_ERROR_e=5; constexpr fpc_ProcID fpcM_ERROR_PROCESS_ID_e=0xffffffff;
struct ModContext{}; constexpr int TRUE=1;using BOOL=int;constexpr int fpcNm_ShopItem_e=11;struct fopAc_ac_c{int profile=11;};
struct daShopItem_c:fopAc_ac_c {int m_itemNo=0;};
struct dShopSystem_c {enum {ITEM_MAX_e=7};struct ItemCtrl {int id=0; int getItemIndex(int){return id;}} ctrl; ItemCtrl* getShopItemCtrlP(){return &ctrl;}};
fopAc_ac_c* stockActor=nullptr;fopAc_ac_c* fopAcM_SearchByID(int){return stockActor;}int fopAcM_GetName(fopAc_ac_c* a){return a->profile;} struct daItem_c {int item,bit;};
namespace daItem_prm {int getItemNo(daItem_c* a){return a->item;} int getItemBitNo(daItem_c* a){return a->bit;}}
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T> T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}
template<class T> T& arg_ref(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
bool active=true,playing=true,event=false; int hp=4,maxHP=24,selectedSlot=14,persists=0;
bool is_active(){return active;}
namespace remaster::tuning {struct Values { bool flaskEnabled=true,removeHeartDrops=true,removeCurrency=true; int flaskStartingCharges=3,flaskMaximumCharges=8,flaskHealingHearts=3; } cfg; auto& get(){return cfg;} }
namespace remaster::player {bool gameplay_running(){return playing;}}
namespace remaster::progression {Model saved; bool available=true; Model& model(){return saved;} bool ready(){return available;} void persist(){++persists;}}
struct daAlink_c {
 enum {PROC_WAIT, PROC_BOTTLE_DRINK, PROC_DEAD, ANM_BOTTLE_DRINK_START};
 struct {struct {struct {struct {int mStartDrinkAnm=0;} m;} mBottle;} mItem;} hio;
 decltype(hio)* mpHIO=&hio;
 int mSelectItemId=0,mProcID=PROC_WAIT,field_0x2f98=0,mDamageTimer=0,held=0,restored=0;
 float mNormalSpeed=0; struct {struct {int y=0;} angle;} current; struct {int y=42;} shape_angle;
 struct {int field_0x3008=0;} mProcVar0; struct {int field_0x300c=0;} mProcVar2; struct {int field_0x300e=0;} mProcVar3;
 bool wolf=false; int mode=0;
 bool checkWolf(){return wolf;} bool checkModeFlg(int v){return mode&v;}
 void commonProcInit(int p){mProcID=p;} void setSingleAnimeParam(int,int*){}
 void keepItemData(){++held;} void setBottleModel(int){} void returnKeepItemData(){++restored;}
 void procWaitInit(){mProcID=PROC_WAIT;}
};
daAlink_c engineLink; daAlink_c* daAlink_getAlinkActorClass(){return &engineLink;}
int dComIfGs_getSelectItemIndex(int){return selectedSlot;} int dComIfGs_getLife(){return hp;} int dComIfGs_getMaxLifeGauge(){return maxHP;}
bool dComIfGp_event_runCheck(){return event;}
u8 bottles[4]={0xff,0xff,0xff,0xff}; int bottleWrites=0;
u8 dComIfGs_getItem(int i,bool){return bottles[i-11];}
namespace remaster::resources::ammo {int hits=0;void damage_received(){++hits;}void death(){}bool enabled(){return false;}bool removes_pickup(int i){return false;}}
namespace remaster::healing::snowpeak {bool special=false;bool allows_spawn(){return special;}bool allows_actor(daItem_c*){return special;}bool allows_grant(){return special;}}
namespace remaster::healing::wolf {int interrupts=0;void interrupt(daAlink_c*){++interrupts;}}
namespace remaster::healing::stones {void damage_received(){}}
struct ItemCheckInfo;struct ItemCheckResolution;
namespace remaster::economy::merchants {bool enabled(){return false;}bool custom_item(uint8_t){return false;}bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}}
namespace remaster::economy::rare {bool custom_item(uint8_t){return false;}bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}}
namespace remaster::progression::collections {bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}}
namespace remaster::healing {
void damage_interrupted(daAlink_c*);
daAlink_c* g_drinking=nullptr; bool g_healed=false,g_retry=false; unsigned g_bottleDepth=0; uint32_t g_saveSlot=0,g_retrySlot=255;
bool enabled(){return is_active() && tuning::get().flaskEnabled && progression::ready();}
auto& progress(){return progression::model();}
uint32_t capacity(){return progress().capacity(tuning::get().flaskStartingCharges,tuning::get().flaskMaximumCharges);}
void set_bottle(int i,u8 item){bottles[i-11]=item; ++bottleWrites;}
'''
reward_source='}\nstruct ItemCheckInfo {const char* name; uint8_t vanilla_item;};\nstruct ItemCheckResolution {uint8_t item,display_item; bool was_resolved;};\nnamespace remaster::progression {using namespace remaster::healing;constexpr uint8_t kFlaskShard=0x13,kStaminaFragment=0x14;\n'+checks+'\n'+reward_callbacks
main=r'''
}
int main(){
 using namespace remaster; using namespace remaster::healing;
 auto* link=&engineLink; int item=dItemNo_RED_BOTTLE_e, result=-1; u16 drinkItem=item;
 void* args[]={&link,&drinkItem}; progression::saved.bottleSlot=14;
 assert(drink_init(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==1);
 assert(progress().charges==2 && g_drinking==link && link->mProcID==daAlink_c::PROC_BOTTLE_DRINK);
 assert(!event && link->mDamageTimer==0 && persists==1);
 // Healing arrives at the native animation's drinking point, never on button press.
 float amount=32; u8 type=0; void* healArgs[]={&amount,&type};
 common_pre(nullptr,nullptr,nullptr,nullptr);
 assert(life_pre(nullptr,healArgs,nullptr,nullptr)==HOOK_CONTINUE && amount==12 && g_healed);
 assert(life_pre(nullptr,healArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 common_post(nullptr,nullptr,nullptr,nullptr); assert(g_bottleDepth==0);
 damage_interrupted(link); assert(!g_drinking && link->mProcID==daAlink_c::PROC_WAIT && progress().charges==2);
 assert(drink_init(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL && progress().charges==1);
 damage_interrupted(link); assert(progress().charges==1); // interrupted windup loses a charge
 hp=maxHP; result=-1; drink_init(nullptr,args,&result,nullptr); assert(result==0 && progress().charges==1);
 hp=4; progress().charges=0; result=-1; drink_init(nullptr,args,&result,nullptr); assert(result==0 && !g_drinking);
 progress().charges=3; selectedSlot=11; result=-1;
 assert(drink_init(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==0 && progress().charges==3);
 // Ordinary potion/soup drinking never restores health; quest/direct scripted heals still do.
 common_pre(nullptr,nullptr,nullptr,nullptr); amount=32;
 assert(life_pre(nullptr,healArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 amount=-4; assert(life_pre(nullptr,healArgs,nullptr,nullptr)==HOOK_CONTINUE);
 common_post(nullptr,nullptr,nullptr,nullptr); amount=12; assert(life_pre(nullptr,healArgs,nullptr,nullptr)==HOOK_CONTINUE);
 // Direct negative health deltas also interrupt recovery and reset supply clocks.
 selectedSlot=14;drink_init(nullptr,args,&result,nullptr);assert(g_drinking==link);
 const int interrupts=healing::wolf::interrupts,hits=resources::ammo::hits;
 amount=-4;life_pre(nullptr,healArgs,nullptr,nullptr);
 assert(!g_drinking&&progress().charges==2&&healing::wolf::interrupts==interrupts+1&&resources::ammo::hits==hits+1);
 // Real quest containers survive conversion and an existing empty slot becomes the flask.
 progress().bottleSlot=255; bottles[0]=dItemNo_WATER_BOTTLE_e; bottles[1]=0x66; bottles[2]=0x7e; bottles[3]=dItemNo_EMPTY_BOTTLE_e;
 normalize_bottles(); assert(progress().bottleSlot==14 && bottles[0]==dItemNo_WATER_BOTTLE_e && bottles[1]==0x66 && bottles[2]==0x7e && bottles[3]==dItemNo_RED_BOTTLE_e);
 progress().bottleSlot=255; bottles[3]=dItemNo_WATER_BOTTLE_e; normalize_bottles(); assert(progress().bottleSlot==255 && bottles[3]==dItemNo_WATER_BOTTLE_e);
 bottles[3]=dItemNo_EMPTY_BOTTLE_e; normalize_bottles();
 // Disposable coin/heart spawns vanish; permanent checks and heart containers survive.
 int spawnItem=1,bit=-1; const void* pos=nullptr; void* dropArgs[]={&pos,&spawnItem,&bit}; fpc_ProcID drop=7;
 assert(drop_pre(nullptr,dropArgs,&drop,nullptr)==HOOK_SKIP_ORIGINAL && drop==fpcM_ERROR_PROCESS_ID_e);
 bit=10; assert(drop_pre(nullptr,dropArgs,&drop,nullptr)==HOOK_CONTINUE);
 daItem_c field{0,10}; auto* fieldPtr=&field; void* fieldArgs[]={&fieldPtr};
 assert(field_item_pre(nullptr,fieldArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==cPhs_ERROR_e);
 field.item=0x21; assert(field_item_pre(nullptr,fieldArgs,&result,nullptr)==HOOK_CONTINUE);
 field.item=0x22; assert(field_item_pre(nullptr,fieldArgs,&result,nullptr)==HOOK_CONTINUE);
 field.item=1; assert(field_item_pre(nullptr,fieldArgs,&result,nullptr)==HOOK_CONTINUE);
 field.bit=255; assert(field_item_pre(nullptr,fieldArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(mounted_drink_pre(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==0);
 daShopItem_c stock; stock.m_itemNo=dItemNo_RED_BOTTLE_e; auto* pstock=&stock; void* stockArgs[]={&pstock};
 assert(shop_arc_before(nullptr,stockArgs,nullptr,nullptr)==HOOK_CONTINUE && stock.m_itemNo==dItemNo_NONE_e);
 dShopSystem_c shop; auto* pshop=&shop; int index=0; void* shopArgs[]={&pshop,&index};stockActor=&stock;
 assert(shop_sold_out_before(nullptr,shopArgs,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==TRUE);
 stock.m_itemNo=0x4b; assert(shop_arc_before(nullptr,stockArgs,nullptr,nullptr)==HOOK_CONTINUE && stock.m_itemNo==0x4b);
 assert(shop_sold_out_before(nullptr,shopArgs,&result,nullptr)==HOOK_CONTINUE);
 index=7; assert(shop_sold_out_before(nullptr,shopArgs,&result,nullptr)==HOOK_CONTINUE);
 ItemCheckInfo info{"chest:R_SP01:1",0xed}; ItemCheckResolution reward{};
 const auto progressBefore=progress();
 for(int i=0;i<4;i++) assert(progression::resolve(nullptr,&info,&reward,nullptr) && reward.item==0x13 && reward.display_item==0x61);
 assert(progress().charges==progressBefore.charges && progress().claimed==progressBefore.claimed); // previews have no side effects
 info.name="chest:D_MN05:7"; info.vanilla_item=3;
 assert(progression::resolve(nullptr,&info,&reward,nullptr) && reward.item==0x14);
 info.name="freestanding:F_SP108:255"; assert(!progression::resolve(nullptr,&info,&reward,nullptr));
 info.name="shop:R_SP01:1:3"; assert(!progression::resolve(nullptr,&info,&reward,nullptr));
 info.name="chest:D_MN05:0"; info.vanilla_item=0x20; assert(!progression::resolve(nullptr,&info,&reward,nullptr));
 u8 grant=0x21; void* grantArgs[]={&grant};assert(progression::item_pre(nullptr,grantArgs,nullptr,nullptr)==HOOK_CONTINUE);
 grant=0;assert(progression::item_pre(nullptr,grantArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 tuning::cfg.removeHeartDrops=false;assert(progression::item_pre(nullptr,grantArgs,nullptr,nullptr)==HOOK_CONTINUE);tuning::cfg.removeHeartDrops=true;
 grant=1;assert(progression::item_pre(nullptr,grantArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 grant=0x13;assert(progression::item_pre(nullptr,grantArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 active=false; assert(drink_init(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
 assert(drop_pre(nullptr,dropArgs,&drop,nullptr)==HOOK_CONTINUE);
 active=true; tuning::cfg.flaskEnabled=false; assert(drink_init(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
 tuning::cfg.flaskEnabled=true; playing=false; selectedSlot=14; assert(drink_init(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==0);
 playing=true; result=1; death_post(nullptr,nullptr,&result,nullptr); assert(g_retry && g_retrySlot==g_saveSlot);
 std::cout<<"Production flask callbacks: vulnerable healing, interruption, resource limits, quest bottles, drops and mode isolation passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'; cpp.write_text(fixture+enums+'\n'+callbacks+reward_source+main)
    subprocess.run(['c++','-std=c++20','-O0','-UNDEBUG','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
