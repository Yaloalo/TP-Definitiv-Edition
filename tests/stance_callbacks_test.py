"""Production stance model and first adapter: contacts, native reactions, lifecycle and gates."""
from fixture import ROOT, function, run
import re
src=(ROOT/'src/combat/stance.cpp').read_text()
def extract(name):
 if name!='state_for':return function(src,name)
 start=src.index('State& state_for(');end=src.index('{',start)+1;depth=1
 while depth:
  depth+=(src[end]=='{')-(src[end]=='}');end+=1
 return src[start:end]
# Assert the adapter's native action interpretation against the pinned implementation.
native=(ROOT/'dusklight/src/d/actor/d_a_e_oc.cpp').read_text()
actions=re.search(r'enum OC_ACTIONS\s*\{([^}]+)',native).group(1)
actions=re.findall(r'E_OC_ACTION_\w+',actions)
assert actions[4:7]==['E_OC_ACTION_ATTACK','E_OC_ACTION_DAMAGE','E_OC_ACTION_BIG_DAMAGE']
assert actions[9]=='E_OC_ACTION_DEATH'
fixture=r'''
#include <cassert>
#include <map>
#include <vector>
#include <cstring>
using s16=short;using u8=unsigned char;
#define AVOID_UB 1
#define KREG_S(x) 0
#define NREG_S(x) 0
#define S16_ADD(a,b) a+=b
#define S16_SUB(a,b) a-=b
struct cXyz{};
#include "combat/stance_model.hpp"
struct {uint32_t mCounter0=1;}g_Counter;
using fpc_ProcID=int;struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
constexpr int fpcNm_E_OC_e=1,AT_TYPE_NORMAL_SWORD=1,AT_TYPE_MASTER_SWORD=2,AT_TYPE_SHIELD_ATTACK=4;
constexpr int AT_TYPE_UNK=8,AT_TYPE_ARROW=16,AT_TYPE_BOMB=32,AT_TYPE_IRON_BALL=64,AT_TYPE_BOOMERANG=128,AT_TYPE_40=256,AT_TYPE_SLINGSHOT=512;
constexpr int E_OC_ACTION_WAIT=0,E_OC_ACTION_WALK=1,E_OC_ACTION_TALK=2,E_OC_ACTION_FIND=3,E_OC_ACTION_DAMAGE=5,E_OC_ACTION_BIG_DAMAGE=6,E_OC_ACTION_DEATH=9;
float cM_rnd(){return 0;}bool dComIfGp_checkPlayerStatus0(int,int){return false;}
struct fopAc_ac_c{int id=1,name=1;};int fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->name;}
std::map<int,fopAc_ac_c*> actors;fopAc_ac_c* fopAcM_SearchByID(int id){return actors.contains(id)?actors[id]:nullptr;}
struct daPy_py_c {static bool checkWoodSwordEquip(){return false;}enum {CUT_TYPE_DOWN,CUT_TYPE_GUARD_ATTACK,CUT_TYPE_HEAD_JUMP,CUT_TYPE_TWIRL,
CUT_TYPE_MORTAL_DRAW_A,CUT_TYPE_MORTAL_DRAW_B,CUT_TYPE_LARGE_JUMP_INIT,CUT_TYPE_LARGE_JUMP,
CUT_TYPE_LARGE_JUMP_FINISH,CUT_TYPE_JUMP,CUT_TYPE_LARGE_TURN_LEFT,CUT_TYPE_LARGE_TURN_RIGHT,
CUT_TYPE_TURN_LEFT,CUT_TYPE_TURN_RIGHT,CUT_TYPE_NORMAL};};
struct daAlink_c:fopAc_ac_c{bool wolf=false;int cut=daPy_py_c::CUT_TYPE_NORMAL;bool checkCutJumpCancelTurn(){return false;}bool checkWolf(){return wolf;}int getCutType(){return cut;}} link;
daAlink_c* daAlink_getAlinkActorClass(){return &link;}
daAlink_c* daPy_getPlayerActorClass(){return &link;}
struct dCcD_GObjInf{int GetAtSpl(){return 0;}int type=1;bool ChkAtType(int t){return type&t;}} collider;
struct daE_OC_c:fopAc_ac_c {
 void damage_check();void executeDamage();int getCutType(){return 2;}
 void offCutDownHitFlg(){downHit=false;}
 int field_0x6d6=0,field_0x6dc=0;const char* mName="E_OC";
 struct Status{void Move(){}}mStts;struct Attention{int flags=1;}attention_info;
 struct Sound{void startCollisionSE(int,int){}void startCreatureVoice(int,int){}}mSound;
 struct Morph{bool isStop(){return false;}}morf;Morph* mpMorf=&morf;
 bool mBattleOn=false;int mPrevShapeAngle=0,field_0x6e3=0,field_0xe60=0,field_0xe64=0,tevStr=0;float speedF=0;
 struct{int y=0;}shape_angle;struct{cXyz pos;}current;
 void setBck(int,int,float,float){}void setSpitEffect(){}
int health=40,field_0x560=220,mActionMode=4,mOcState=0,field_0x6cc=0,offCalls=0;bool downHit=false,head=false;
 bool checkCutDownHitFlg(){return downHit;}void onHeadLockFlg(){head=true;}void offHeadLockFlg(){head=false;}
 void setActionMode(int,int);int mOldActionMode=0,field_0x6c0=0,field_0x6c2=0;void offTgSph(){++offCalls;}
 struct Sphere{bool hit=false;dCcD_GObjInf* obj=nullptr;fopAc_ac_c* owner=nullptr;
 bool collisionOn=false,targetOn=false;void OnCoSetBit(){collisionOn=true;}void OnTgSetBit(){targetOn=true;}
 cXyz pos;cXyz* GetTgHitPosP(){return &pos;}bool ChkTgHit(){return hit;}dCcD_GObjInf* GetTgHitObj(){return obj;}fopAc_ac_c* GetTgHitAc(){return owner;}}mSphs_cc[2];
 struct Info{dCcD_GObjInf* mpCollider=nullptr;int mAttackPower=1;struct Direction{int y=0;}mHitDirection;}mAtInfo;
};int fopAcM_searchPlayerAngleY(daE_OC_c*){return 123;}
 constexpr int Z2SE_EN_OC_V_DAMAGE_LR=1,Z2SE_EN_OC_V_DAMAGE=2,Z2SE_EN_OC_V_GALE=3,E_OC_ACTION_MOVE_OUT=7;
 int cLib_distanceAngleS(int,int){return 0;}void cLib_chaseF(float*,float,float){}
 void fopAcM_effSmokeSet2(int*,int*,cXyz*,void*,float,int*){}
 void dComIfGp_setHitMark(int,daE_OC_c*,cXyz*,void*,void*,int){}
 // Collision damage arithmetic is outside this fixture; the native function below
 // still owns sphere selection, shield reactions, Helm Splitter death and actions.
 void cc_at_check(daE_OC_c*,daE_OC_c::Info*){}

namespace mods{template<class T>T arg(void* a,int){return *static_cast<T*>(a);}}
bool mode=true,allowed=true;
namespace remaster {
 bool is_active(){return mode;}
 namespace player{bool input_allowed(){return allowed;}}
 namespace tuning{struct Values{bool hiddenStance=true,combatTechniques=true;int bokoblinStance=55;}v;auto& get(){return v;}}
 namespace combat::techniques{enum class Attack{Ordinary,Heavy,GuardCounter};Attack attack=Attack::Ordinary;uint64_t serial=1;
 Attack current_attack(daAlink_c*){return attack;}uint64_t attack_serial(){return serial;}}
 namespace combat::stance {
 constexpr int kAttack=4,kDamage=5,kDown=6,kLastOrdinary=8;
 struct Entry{daE_OC_c* actor=nullptr;State state;uint64_t lastParry=0,attackingBash=0;uint32_t bashFrame=0;};std::map<int,Entry> g_states;
 struct Contact{daE_OC_c* actor=nullptr;uint64_t serial=0;float impact=0;bool bash=false,attacking=false;};std::vector<Contact> g_contacts;
'''
native_damage=native[native.index('void daE_OC_c::damage_check()'):native.index('void daE_OC_c::setBck(')]
native_damage+=native[native.index('void daE_OC_c::executeDamage()'):native.index('void daE_OC_c::executeBigDamage()')]
native_damage+=native[native.index('void daE_OC_c::setActionMode('):native.index('int daE_OC_c::getCutType()')]
funcs=''.join(extract(n) for n in ['active','eligible','state_for','impact_for','resistance_for','break_stance','before_damage','after_damage','deleted','owns_bokoblin','parry','tick','reset'])
checks=r'''
}}
int main(){using namespace remaster::combat;using namespace stance;
 State upgrade;assert(!upgrade.contact(1,10,55));assert(!upgrade.contact(1,40,55));
 assert(upgrade.pressure==40);assert(!upgrade.contact(1,40,55));assert(upgrade.pressure==40);

 State state;
 assert(!state.contact(1,35,55));assert(state.pressure==35);
 assert(!state.contact(1,35,55)&&state.pressure==35); // duplicate collider/contact
 assert(state.contact(2,25,55)&&state.pressure==0&&state.recoveryTicks==60);
 assert(!state.contact(3,100,55));for(int i=0;i<60;++i)state.tick();
 assert(!state.contact(4,10,55));for(int i=0;i<90;++i)state.tick();assert(state.pressure==10);
 state.tick();assert(state.pressure==9.5f);for(int i=0;i<50;++i)state.tick();assert(state.pressure==0);
 daE_OC_c actor;actors[1]=&actor;daE_OC_c* args=&actor;
 actor.mSphs_cc[0]={true,&collider,&link};actor.mSphs_cc[1]=actor.mSphs_cc[0];
 techniques::attack=techniques::Attack::Heavy;
 before_damage(nullptr,&args,nullptr,nullptr);actor.field_0x6cc=10;actor.mActionMode=6;
 after_damage(nullptr,&args,nullptr,nullptr);assert(actor.mActionMode==5&&state_for(&actor).pressure==35);
 // Same swing reaching both spheres/native callbacks contributes once.
 actor.field_0x6cc=0;before_damage(nullptr,&args,nullptr,nullptr);actor.field_0x6cc=10;
 after_damage(nullptr,&args,nullptr,nullptr);assert(state_for(&actor).pressure==35);
 ++techniques::serial;actor.field_0x6cc=0;before_damage(nullptr,&args,nullptr,nullptr);
 actor.field_0x6cc=10;after_damage(nullptr,&args,nullptr,nullptr);
 assert(actor.mActionMode==6&&actor.offCalls==1&&actor.health==40); // break never fabricates death
 // Plain bash does not grant the parry-only Helm Splitter opening.
 reset();actor.mActionMode=4;actor.field_0x6cc=0;actor.head=true;collider.type=AT_TYPE_SHIELD_ATTACK;
 link.cut=daPy_py_c::CUT_TYPE_GUARD_ATTACK;techniques::attack=techniques::Attack::Ordinary;++techniques::serial;
 before_damage(nullptr,&args,nullptr,nullptr);actor.field_0x6cc=10;actor.mActionMode=5;actor.mOcState=3;
 after_damage(nullptr,&args,nullptr,nullptr);assert(actor.mOcState==2&&!actor.head&&state_for(&actor).pressure==10);

 // Bash-first and parry-first must reconcile only a proven same-frame incoming attack.
 assert(parry(&actor,&link)&&actor.mOcState==3&&actor.head&&state_for(&actor).pressure==40);
 assert(!parry(&actor,&link));
 for(int scenario=0;scenario<3;++scenario){
  reset();actor.mActionMode=scenario==2?5:4;actor.field_0x6cc=0;++techniques::serial;
  before_damage(nullptr,&args,nullptr,nullptr);actor.field_0x6cc=10;actor.mActionMode=5;actor.mOcState=3;
  after_damage(nullptr,&args,nullptr,nullptr);
  if(scenario==0)++g_Counter.mCounter0;if(scenario==1)++techniques::serial;
  assert(!parry(&actor,&link)); // expired frame, different attack, or enemy was already recoiling
 }
 // Disabling techniques preserves native learned follow-up access with stance still enabled.
 reset();remaster::tuning::v.combatTechniques=false;actor.mActionMode=4;actor.field_0x6cc=0;++techniques::serial;
 before_damage(nullptr,&args,nullptr,nullptr);actor.field_0x6cc=10;actor.mActionMode=5;actor.mOcState=3;actor.head=true;
 after_damage(nullptr,&args,nullptr,nullptr);assert(actor.mOcState==3&&actor.head);
 assert(!parry(&actor,&link));remaster::tuning::v.combatTechniques=true;
 // Same-target bash after a successful parry retains the opening without double pressure.
 reset();actor.mActionMode=4;actor.field_0x6cc=0;++techniques::serial;assert(parry(&actor,&link));
 before_damage(nullptr,&args,nullptr,nullptr);actor.field_0x6cc=10;actor.mActionMode=5;actor.mOcState=3;
 after_damage(nullptr,&args,nullptr,nullptr);assert(actor.mOcState==3&&actor.head&&state_for(&actor).pressure==40);
 collider.type=AT_TYPE_NORMAL_SWORD;link.cut=daPy_py_c::CUT_TYPE_NORMAL;
 // Ordinary 40-HP enemies must break on a committed heavy before damage kills them.
 reset();actor.field_0x560=40;actor.health=40;actor.mActionMode=4;actor.field_0x6cc=0;
 techniques::attack=techniques::Attack::Heavy;++techniques::serial;
 before_damage(nullptr,&args,nullptr,nullptr);actor.health-=30;actor.mActionMode=6;actor.field_0x6cc=10;
 after_damage(nullptr,&args,nullptr,nullptr);assert(actor.health==10&&actor.mActionMode==6);
 actor.field_0x560=220;
 // Native lethal hits and explicit executions are untouched.
 reset();actor.mActionMode=4;actor.field_0x6cc=0;before_damage(nullptr,&args,nullptr,nullptr);
 actor.health=0;actor.field_0x6cc=10;actor.mActionMode=9;after_damage(nullptr,&args,nullptr,nullptr);
 assert(actor.health==0&&actor.mActionMode==9&&g_states.empty());
 actor.health=40;actor.mActionMode=4;actor.field_0x6cc=0;actor.downHit=true;
 before_damage(nullptr,&args,nullptr,nullptr);after_damage(nullptr,&args,nullptr,nullptr);assert(g_states.empty());actor.downHit=false;
 // Unaccepted, invulnerable, Wolf, scripted and nonplayer contacts grant no stance.
 for(int scenario=0;scenario<6;++scenario){
  reset();link.wolf=false;actor.field_0x6cc=0;actor.mActionMode=4;actor.mSphs_cc[1].owner=&link;allowed=true;
  if(scenario==0)actor.field_0x6cc=3;if(scenario==1)link.wolf=true;
  if(scenario==2)actor.mActionMode=11;if(scenario==3)allowed=false;
  if(scenario==4)actor.mSphs_cc[1].owner=&actor;
  before_damage(nullptr,&args,nullptr,nullptr); // scenario 5 has no accepted native hit afterward
  after_damage(nullptr,&args,nullptr,nullptr);assert(g_states.empty());
 }
 allowed=true;link.wolf=false;actor.mActionMode=4;actor.field_0x6cc=0;reset();++techniques::serial;
 assert(parry(&actor,&link)&&actor.mActionMode==5&&actor.head&&state_for(&actor).pressure==40);
 assert(!parry(&actor,&link)); // no reacting twice to the already interrupted attack
 actor.mActionMode=4;++techniques::serial;assert(parry(&actor,&link)&&actor.mActionMode==6);
 actor.name=77;assert(!parry(&actor,&link));actor.name=1;

 // Execute the pinned native damage_check body between production hooks, both orders.
 for(int order=0;order<2;++order){
  reset();actor=daE_OC_c{};actor.mpMorf=&actor.morf;actor.health=220;actor.field_0x560=220;actor.mActionMode=4;
  collider.type=AT_TYPE_SHIELD_ATTACK;link.cut=daPy_py_c::CUT_TYPE_GUARD_ATTACK;
  actor.mSphs_cc[0]={true,&collider,&link};++techniques::serial;
  if(order==0)assert(parry(&actor,&link));
  before_damage(nullptr,&args,nullptr,nullptr);actor.damage_check();after_damage(nullptr,&args,nullptr,nullptr);
  actor.executeDamage(); // native action() executes the reaction before the next actor
  if(order==1)assert(parry(&actor,&link));
  assert(actor.mActionMode==5&&actor.head&&state_for(&actor).pressure==40);
 }
 // Weak parries break immediately; native Helm Splitter kills sturdy enemies,
 // so neither result is misrepresented as a nonlethal pressure-only follow-up.
 reset();actor=daE_OC_c{};actor.mpMorf=&actor.morf;actor.field_0x560=40;++techniques::serial;
 assert(parry(&actor,&link)&&actor.mActionMode==6);
 reset();actor=daE_OC_c{};actor.mpMorf=&actor.morf;actor.health=220;actor.field_0x560=220;
 collider.type=AT_TYPE_NORMAL_SWORD;link.cut=daPy_py_c::CUT_TYPE_HEAD_JUMP;
 actor.mSphs_cc[0]={true,&collider,&link};++techniques::serial;
 before_damage(nullptr,&args,nullptr,nullptr);actor.damage_check();after_damage(nullptr,&args,nullptr,nullptr);
 assert(actor.health==0&&actor.mActionMode==6&&g_states.empty());
 // A bash that itself breaks an attacking enemy can still reconcile its incoming parry.
 reset();actor=daE_OC_c{};actor.mpMorf=&actor.morf;actor.field_0x560=40;state_for(&actor).pressure=25;
 collider.type=AT_TYPE_SHIELD_ATTACK;link.cut=daPy_py_c::CUT_TYPE_GUARD_ATTACK;
 actor.mSphs_cc[0]={true,&collider,&link};++techniques::serial;
 before_damage(nullptr,&args,nullptr,nullptr);actor.damage_check();after_damage(nullptr,&args,nullptr,nullptr);
 assert(actor.mActionMode==6&&parry(&actor,&link)&&actor.mActionMode==6);
 // Actor generation/lifecycle, menu freeze and mode isolation.
 state_for(&actor).pressure=20;state_for(&actor).quietTicks=100;allowed=false;tick();assert(state_for(&actor).pressure==20);
 allowed=true;tick();assert(state_for(&actor).pressure==19.5f);
 daE_OC_c replacement;replacement.id=1;assert(state_for(&replacement).pressure==0);
 actors.erase(1);tick();assert(g_states.empty());state_for(&actor);deleted(nullptr,&args,nullptr,nullptr);assert(g_states.empty());
 state_for(&actor);mode=false;tick();assert(g_states.empty());mode=true;
 // All learned techniques have distinct pressure roles, without overriding their native gates.
 techniques::attack=techniques::Attack::Ordinary;
 link.cut=daPy_py_c::CUT_TYPE_DOWN;assert(impact_for(&link)==0);
 link.cut=daPy_py_c::CUT_TYPE_HEAD_JUMP;assert(impact_for(&link)==40);
 link.cut=daPy_py_c::CUT_TYPE_TWIRL;assert(impact_for(&link)==28);
 link.cut=daPy_py_c::CUT_TYPE_MORTAL_DRAW_A;assert(impact_for(&link)==35);
 link.cut=daPy_py_c::CUT_TYPE_LARGE_JUMP_FINISH;assert(impact_for(&link)==40);
 link.cut=daPy_py_c::CUT_TYPE_LARGE_TURN_LEFT;assert(impact_for(&link)==25);
}
'''
run(fixture+funcs+'\n}}\n'+native_damage+'\nnamespace remaster { namespace combat::stance {\n'+checks)
print('Stance: production adapter, native action IDs, deduplication, recovery, skills, parries and lifecycle checks passed')
