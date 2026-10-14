"""Execute growth callbacks with native heart-item functions and pending counters."""
from fixture import ROOT, function, run
source=(ROOT/'src/progression/growth.cpp').read_text()
native=(ROOT/'dusklight/src/d/d_item.cpp').read_text()
code=r'''
#include <cassert>
#include <algorithm>
#include <cstdint>
#include "progression/model.hpp"
using f32=float;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
constexpr int dItemNo_KAKERA_HEART_e=0x13,dItemNo_UTAWA_HEART_e=0x22;
bool active=true,running=true;bool is_active(){return active;}
int maximum=15,life=8,pending=0,healing=0,persisted=0;bool stageLife=false;
int dComIfGs_getMaxLife(){return maximum;}void dComIfGs_setMaxLife(int n){maximum=n;}
int dComIfGs_getLife(){return life;}void dComIfGs_setLife(int n){life=n;}
int dComIfGs_getMaxLifeGauge(){return maximum/5*4;}
int dComIfGp_getItemMaxLifeCount(){return pending;}void dComIfGp_clearItemMaxLifeCount(){pending=0;}
void dComIfGp_setItemMaxLifeCount(int n){pending+=n;}void dComIfGp_setItemLifeCount(float n,int){healing+=int(n);}
struct stage_stag_info_class{};stage_stag_info_class stage;
stage_stag_info_class* dComIfGp_getStageStagInfo(){return &stage;}int dStage_stagInfo_GetSaveTbl(stage_stag_info_class*){return 5;}
void dComIfGs_onStageLife(){stageLife=true;}
struct ItemGiveInfo{const char* check_name;uint8_t item;};
namespace remaster::player{bool gameplay_running(){return running;}}
namespace remaster::progression{Model state;bool valid=true;bool ready(){return valid;}Model& model(){return state;}void persist(){++persisted;}}
'''
code+=function(native,'item_func_KAKERA_HEART')+function(native,'item_func_UTUWA_HEART')
code+='namespace remaster::progression::growth {\n'+''.join(function(source,n) for n in ['ensure','apply_health','allocate','tick','maximum_pre','reward_given'])+'}\n'
code+=r'''
int main(){using namespace remaster::progression;using namespace remaster::progression::growth;
 assert(ensure()&&state.growthReady&&maximum==15&&life==8);
 item_func_UTUWA_HEART();assert(stageLife&&pending==5);
 ItemGiveInfo boss{"boss-heart:forest",0x22};reward_given(nullptr,&boss,nullptr);
 assert(stageLife&&pending==0&&state.growthFragments==5&&state.unassigned()==1&&life==8&&maximum==15);
 assert(allocate(true)&&state.stamina_bonus()==20&&life==8);
 item_func_UTUWA_HEART();reward_given(nullptr,&boss,nullptr);assert(pending==0&&state.growthFragments==5);
 for(int i=0;i<5;++i){item_func_KAKERA_HEART();std::string name="piece:"+std::to_string(i);ItemGiveInfo piece{name.c_str(),0x13};reward_given(nullptr,&piece,nullptr);tick();}
 assert(state.growthFragments==10&&state.unassigned()==1&&maximum==15&&life==8); // no partial-counter double counting
 assert(allocate(false)&&maximum==20&&life==8);life=16;assert(state.exchange(true));apply_health();assert(maximum==15&&life==12);
 assert(state.exchange(false));apply_health();assert(maximum==20&&life==12); // exchanging back never heals
 item_func_KAKERA_HEART();ItemGiveInfo unknown{nullptr,0x13};reward_given(nullptr,&unknown,nullptr);
 assert(pending==1);maximum_pre(nullptr,nullptr,nullptr,nullptr);assert(pending==0&&state.growthFragments==11);
 maximum_pre(nullptr,nullptr,nullptr,nullptr);assert(state.growthFragments==11);
 active=false;item_func_KAKERA_HEART();maximum_pre(nullptr,nullptr,nullptr,nullptr);assert(pending==1);
 active=true;valid=false;reward_given(nullptr,&boss,nullptr);assert(pending==1&&state.growthFragments==11);
}
'''
run(code)
print('Growth callbacks: native boss flags, fragment/point conversion, duplicate grants, no respec healing, pending-counter fallback and mode/corrupt-save gates passed')
