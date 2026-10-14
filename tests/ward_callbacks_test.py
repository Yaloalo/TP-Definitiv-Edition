"""Actual ward/damage callbacks and pinned native damage branch, without game assets."""
import re
from fixture import ROOT,function,run
source=(ROOT/'src/combat/ward.cpp').read_text()
def pointer_function(source,name):
 a=source.index('void* '+name+'(');b=source.index('{',a)+1;depth=1
 while depth:
  depth+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[a:b]+'\n'
selected=''.join(function(source,n) for n in ['ensure','clear_clock','new_save','loaded','enabled','reset','damage_received','absorb','refill','death','boss_victory','tick'])
native=(ROOT/'dusklight/src/d/actor/d_a_alink_damage.inc').read_text();a=native.index('int daAlink_c::setDamagePoint(');b=native.index('\n#if TARGET_PC\n    dusk::AchievementSystem',a)
native_branch=native[a:b]+'\nreturn i_dmgAmount;\n}\n'
code=r'''
#include <cassert>
#include <cmath>
#include <vector>
#include <cstring>
#include <string>
#include "combat/ward_model.hpp"
using f32=float;using u8=uint8_t;using BOOL=int;constexpr int FALSE=0;
struct ModContext{};ModContext*mod_ctx=nullptr;enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
bool active=true,input=true,owned=true,enemies=false;bool is_active(){return active;}unsigned slot=0;int hp=40,persists=0,rupeeDeltas=0;std::string stage="F_SP108";int room=0;
namespace mods{template<class T>T arg(void*p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}template<class T>T&arg_ref(void*p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}namespace log{template<class...T>void info(T...){}}}
namespace remaster::progression{Model state;bool valid=true;bool ready(){return valid;}Model&model(){return state;}void persist(){++persists;}}
namespace remaster::tuning{struct Values{bool magicWard=true,debugLog=false,removeCurrency=true,staminaEnabled=false;int enemyDamagePercent=100;int64_t wardRecoveryDelayMs=12000,wardRecoveryIntervalMs=6000;}cfg;auto&get(){return cfg;}}
namespace remaster::player{bool input_allowed(){return active&&input;}}
namespace remaster::difficulty{float multiplier(){return 1;}const char*tier_name(){return "test";}}
struct Vec{float x=0;float abs(Vec v){return std::abs(x-v.x);}};
struct fopAc_ac_c{int health=1,group=2;struct{Vec pos;}current;};constexpr int fopAc_ENEMY_e=2;
struct daAlink_c:fopAc_ac_c{enum{PROC_WAIT,PROC_DEAD};int mProcID=PROC_WAIT;bool worn=true,wolf=false;bool checkMagicArmorWearAbility(){return worn&&!wolf;}bool checkWolf(){return wolf;}void offWolfEyeUp(){}BOOL checkMagicArmorNoDamage();float damageMagnification(BOOL,int){return 1;}int setDamagePoint(int,BOOL,BOOL,int); }link;
bool dComIfGs_isCollectClothes(int){return owned;}int dComIfGs_getLife(){return hp;}unsigned dComIfGs_getDataNum(){return slot;}const char*dComIfGp_getStartStageName(){return stage.c_str();}int fopAcM_GetRoomNo(daAlink_c*){return room;}int fopAcM_GetGroup(fopAc_ac_c*a){return a->group;}
fopAc_ac_c enemy;void* fopAcIt_Judge(void*(*fn)(void*,void*),void*data){return enemies?fn(&enemy,data):nullptr;}
namespace remaster::economy{
'''+function((ROOT/'src/economy/currency.cpp').read_text(),'armor')+r'''
}
BOOL daAlink_c::checkMagicArmorNoDamage(){if(!checkMagicArmorWearAbility())return false;int result=1;remaster::economy::armor(nullptr,nullptr,&result,nullptr);return result;}
namespace remaster::combat::ward{
resources::Recharge g_clock;daAlink_c*g_owner=nullptr;std::string g_stage;int g_room=-1,g_lastLife=-1;bool g_retry=false;uint32_t g_retrySlot=255;
bool enabled();void reset();
'''+pointer_function(source,'threat')+selected+r'''
}
namespace remaster::healing{void damage_interrupted(daAlink_c*){}}
namespace remaster::resources::ammo{void damage_received(){}}
namespace remaster::combat::damage{
struct DamageScope{daAlink_c*link=nullptr;int expected=0;bool correctArmor=false;};std::vector<DamageScope>g_scopes;
'''+''.join(function((ROOT/'src/combat/damage.cpp').read_text(),n) for n in ['damage_end','armor_life_pre','on_set_damage_pre'])+r'''
}
void dComIfGp_setItemLifeCount(float amount,u8 type){void*args[]={&amount,&type};remaster::combat::damage::armor_life_pre(nullptr,args,nullptr,nullptr);hp+=int(amount);}
void dComIfGp_setItemRupeeCount(int){++rupeeDeltas;}
namespace dusk{enum class MagicArmorMode{NORMAL,DOUBLE_DEFENSE,INVINCIBLE};struct Mode{MagicArmorMode value=MagicArmorMode::NORMAL;MagicArmorMode getValue(){return value;}};struct Settings{struct{Mode armorRupeeDrain;}game;}settings;auto&getSettings(){return settings;}}
#define TARGET_PC 1
#define DEBUG 0
'''+native_branch+r'''
void hit(int amount){auto*p=&link;BOOL zora=0,timer=1;int kind=0;void*args[]={&p,&amount,&zora,&timer,&kind};remaster::combat::damage::on_set_damage_pre(nullptr,args,nullptr,nullptr);link.setDamagePoint(amount,zora,timer,kind);remaster::combat::damage::damage_end(nullptr,args,nullptr,nullptr);assert(remaster::combat::damage::g_scopes.empty());}
int main(){using namespace remaster;using namespace combat::ward;
 progression::state.migrate_growth(15);assert(ensure()&&progression::state.wardCharges==3);auto money=progression::state.rareRupees;
 for(auto preset:{dusk::MagicArmorMode::NORMAL,dusk::MagicArmorMode::DOUBLE_DEFENSE,dusk::MagicArmorMode::INVINCIBLE}){
  dusk::settings.game.armorRupeeDrain.value=preset;refill();int before=hp;hit(8);assert(hp==before-4&&progression::state.wardCharges==2&&rupeeDeltas==0);
 }
 progression::state.wardCharges=0;int before=hp;hit(8);assert(hp==before-8&&rupeeDeltas==0); // empty ward never inherits native invincibility/double defense
 refill();before=hp;hit(1);assert(hp==before-1&&progression::state.wardCharges==2); // no rounded-to-zero hits
 before=hp;hit(-4);assert(hp==before+4&&progression::state.wardCharges==2&&progression::state.rareRupees==money);
 link.wolf=true;before=hp;hit(8);assert(hp==before-8&&progression::state.wardCharges==2);link.wolf=false;
 hp=40;progression::state.wardCharges=0;reset();for(int i=0;i<539;++i)tick(&link);assert(progression::state.wardCharges==0);tick(&link);assert(progression::state.wardCharges==1);
 enemies=true;for(int i=0;i<1000;++i)tick(&link);assert(progression::state.wardCharges==1&&g_clock.quietTicks==0);enemies=false;
 input=false;for(int i=0;i<1000;++i)tick(&link);assert(progression::state.wardCharges==1);input=true;
 damage_received();for(int i=0;i<539;++i)tick(&link);assert(progression::state.wardCharges==1);tick(&link);assert(progression::state.wardCharges==2);
 auto saved=progression::encode(progression::state);loaded(nullptr,0,nullptr);tick(&link);assert(progression::state.wardCharges==2);link.worn=false;tick(&link);link.worn=true;tick(&link);assert(progression::state.wardCharges==2);
 ++room;tick(&link);assert(g_clock.quietTicks==1);--room;stage="D_MN10";tick(&link);assert(g_clock.quietTicks==1);
 progression::state.wardCharges=0;boss_victory("F_SP108");assert(progression::state.wardCharges==0);boss_victory("D_MN10A");assert(progression::state.wardCharges==3);progression::state.wardCharges=1;boss_victory("D_MN10A");assert(progression::state.wardCharges==1);
 progression::state.equipmentUpgrades=1|progression::treasures::kSunward|progression::treasures::kMirrorWard;refill();before=hp;hit(10);assert(hp==before-4&&progression::state.wardCharges==4);
 hp=40;progression::state.wardCharges=0;death();link.mProcID=daAlink_c::PROC_DEAD;tick(&link);assert(progression::state.wardCharges==0);loaded(nullptr,0,nullptr);link.mProcID=daAlink_c::PROC_WAIT;tick(&link);assert(progression::state.wardCharges==5);
 progression::state.wardCharges=0;death();slot=1;loaded(nullptr,1,nullptr);tick(&link);assert(progression::state.wardCharges==0);new_save(nullptr,1,nullptr);assert(!g_retry);
 owned=false;tick(&link);assert(progression::state.wardCharges==0);owned=true;progression::valid=false;assert(absorb(&link)==1);progression::valid=true;
 active=false;auto state=progression::encode(progression::state);assert(absorb(&link)==1);tick(&link);assert(progression::encode(progression::state)==state);
}
'''
run(code)
print('Ward/native damage: all armor presets, no rupee/stamina use, bounded hit protection, safe timed recovery, upgrades, save/tunic/death/boss isolation passed')
