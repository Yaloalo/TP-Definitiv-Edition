"""Production collection callbacks with indexed native quest flags and queued grants."""
import re
from fixture import ROOT, function, run
source=(ROOT/'src/progression/collections.cpp').read_text()
items=(ROOT/'dusklight/include/d/d_item_data.h').read_text()
ids={n:int(v,16) for v,n in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(dItemNo_\w+)',items)}
selected=''.join(function(source,n) for n in ['enabled','delivered','caught','notice','reset','give','ensure','pairs','souls','unpresented','present_bugs','rewards_available','claim_rewards','resolve'])
constants='\n'.join(f'constexpr int {n}={ids[n]};' for n in set(re.findall(r'dItemNo_\w+',selected))|{'dItemNo_DROP_BOTTLE_e','dItemNo_SILVER_RUPEE_e'})
code=r'''
#include <cassert>
#include <charconv>
#include <set>
#include <format>
#include <array>
#include "progression/collection_model.hpp"
#include "progression/treasure_model.hpp"
bool is_active();struct ModContext{};ModContext* mod_ctx=nullptr;constexpr int MOD_OK=0,MOD_ERROR=1,ITEM_GIVE_RESOLVE=2,ITEM_GIVE_SILENT=1;
struct ItemCheckInfo{const char* name;};struct ItemCheckResolution{uint8_t item,display;bool animation;};struct ItemGiveInfo{const char* check_name;uint8_t item;};
struct UiToastDesc{const char* title_rml=nullptr;const char* body_rml=nullptr;};
#define UI_TOAST_DESC_INIT {}
std::set<int> nativeFlags;std::set<int> ownedItems;int poeCount=0,persisted=0,toasts=0;bool active=true,saveReady=true,fail=false;bool is_active(){return active;}
// These APIs take saveBitLabels indices, not raw bits. Reject the wrong API/encoding.
bool daNpcF_chkEvtBit(int id){assert(id>=0x191&&id<0x191+24);return nativeFlags.contains(id);}
void daNpcF_onEvtBit(int id){assert(id>=0x191&&id<0x191+24);nativeFlags.insert(id);}
bool dComIfGs_isItemFirstBit(int id){return ownedItems.contains(id);}int dComIfGs_getPohSpiritNum(){return poeCount;}
struct Ui{int push_toast(ModContext*,UiToastDesc*){++toasts;return MOD_OK;}}ui;auto* svc_ui=&ui;
struct Items{std::vector<std::pair<std::string,int>> queue;int give_item(ModContext*,const char* name,int id,int flags){assert(flags==3);if(fail)return MOD_ERROR;queue.emplace_back(name,id);return MOD_OK;}}items;auto* svc_item=&items;
namespace fmt{template<class...T>std::string format(std::string_view f,T...args){return std::vformat(f,std::make_format_args(args...));}}
namespace remaster::tuning{struct Values{bool removeCurrency=true,merchantRewards=true;int flaskStartingCharges=3,flaskMaximumCharges=8;}cfg;auto& get(){return cfg;}}
namespace remaster::progression{Model state;bool ready(){return saveReady;}Model& model(){return state;}void persist(){++persisted;}}
namespace remaster::progression::treasures{bool ensure(){return active&&saveReady&&migrate(remaster::progression::state);}}
namespace remaster::progression::growth{bool ensure(){if(!active||!saveReady)return false;remaster::progression::state.migrate_growth(15);return true;}void apply_health(){}}
namespace remaster::economy::rare{bool enabled(){return active;}bool ensure(){if(!active||!saveReady)return false;remaster::progression::state.migrate_growth(15);return true;}}
'''+constants+r'''
namespace remaster::progression::collections{
std::set<std::string> g_pending;
std::string bug_name(int i){return "bug:"+std::to_string(dItemNo_M_BEETLE_e+i);}
bool ensure();bool claim_rewards(bool);
'''+selected+r'''
}
int main(){using namespace remaster;using namespace progression::collections;
 for(unsigned mask=0;mask<(1u<<12);++mask){nativeFlags.clear();int expected=0;for(int p=0;p<12;++p)if(mask&(1u<<p)){nativeFlags.insert(0x191+2*p);nativeFlags.insert(0x192+2*p);++expected;}assert(pairs()==expected);}
 nativeFlags.clear();ownedItems={dItemNo_M_BEETLE_e,dItemNo_M_BEETLE_e+1};assert(unpresented()==2&&pairs()==0);
 ItemCheckInfo info{"collection:bugs:1"};ItemCheckResolution out{};auto before=progression::encode(progression::state);
 assert(resolve(&info,&out)&&out.item==255&&progression::encode(progression::state)==before); // preview is pure
 assert(present_bugs()&&pairs()==1&&unpresented()==0&&progression::state.growthFragments==0&&progression::state.relics[2]==2&&items.queue.size()==1);
 assert(!present_bugs()&&items.queue.size()==1);ItemGiveInfo gift{kBugs[0].check,kBugs[0].item};give(nullptr,&gift,nullptr);give(nullptr,&gift,nullptr);
 assert(progression::state.growthFragments==5&&!rewards_available(true));
 fail=true;poeCount=60;assert(!claim_rewards(false)&&g_pending.empty());fail=false;
 assert(claim_rewards(false)&&!claim_rewards(false)&&g_pending.size()==5);
 // The normal progression observer handles the one-fragment reward, merchants commit relic provenance.
 for(auto& [name,id]:items.queue){if(id==0x14)progression::state.claim(name.c_str(),id);if(id==0x38||id==0x39)progression::state.claim(name.c_str(),id);gift={name.c_str(),uint8_t(id)};give(nullptr,&gift,nullptr);}
 assert(progression::state.growthFragments==11&&progression::state.flaskShards==4&&progression::state.equipmentUpgrades==4);
 assert(progression::state.claimed.contains("collection:poes:20")&&!rewards_available(false));
 for(int i=2;i<24;++i){ownedItems.insert(dItemNo_M_BEETLE_e+i);}assert(present_bugs()&&pairs()==12);
 for(auto& [name,id]:items.queue){if(id==0x38)progression::state.claim(name.c_str(),id);gift={name.c_str(),uint8_t(id)};give(nullptr,&gift,nullptr);}
 assert(progression::state.flaskShards==8&&progression::state.capacity(3,8)==5&&progression::state.equipmentUpgrades==6);
 assert(progression::state.growthFragments==11&&progression::state.relics[2]==24); // Bug components are separate; pair/Poe blessings total 11 growth fragments
 for(int i=0;i<12;++i){auto n="world-shard:"+std::to_string(i);progression::state.claim(n.c_str(),0x13);}assert(progression::state.capacity(3,8)==8);
 auto complete=progression::encode(progression::state);assert(!present_bugs()&&!claim_rewards(true)&&!claim_rewards(false)&&progression::encode(progression::state)==complete);
 progression::Model loaded;assert(progression::decode(complete,loaded)&&loaded.equipmentUpgrades==6);
 // Native Jovani checks share the milestone ledger; incomplete, repeated or unavailable saves retain inert native presentations.
 reset(nullptr,0,nullptr);info={"jovani_reward_1"};assert(resolve(&info,&out)&&out.item==economy::rare::kStoryAcknowledgment);
 progression::state={};nativeFlags.clear();poeCount=19;assert(resolve(&info,&out)&&out.item==economy::rare::kStoryAcknowledgment);poeCount=20;assert(resolve(&info,&out)&&out.item==0x39);
 saveReady=false;assert(resolve(&info,&out)&&out.item==economy::rare::kStoryAcknowledgment);saveReady=true;
 info={"jovani_reward_2"};assert(resolve(&info,&out)&&out.item==economy::rare::kStoryAcknowledgment);poeCount=60;assert(resolve(&info,&out)&&out.item==0x3b);
 info={"bug:192junk"};assert(!resolve(&info,&out));info={"bug:191"};assert(!resolve(&info,&out));info={"bug:192"};assert(resolve(&info,&out)&&out.item==progression::treasures::kGildedThread);
 // Legacy hand-ins migrate once; full ledger failure does not partially mutate native hand-ins.
 progression::state={};nativeFlags={0x191,0x192};assert(ensure()&&progression::state.growthFragments==0&&progression::state.relics[2]==2);assert(ensure()&&progression::state.relics[2]==2);
 progression::state={};progression::state.migrate_growth(15);for(int i=0;i<512;++i){auto n="full:"+std::to_string(i);progression::state.claim(n.c_str(),255);}before=progression::encode(progression::state);
 assert(!ensure()&&!present_bugs()&&!claim_rewards(false)&&progression::encode(progression::state)==before);assert(resolve(&info,&out));
 active=false;assert(!resolve(&info,&out));assert(!rewards_available(true));
 info={kBugs[0].check};assert(resolve(&info,&out)&&out.item==255);active=true;tuning::cfg.merchantRewards=false;assert(resolve(&info,&out)&&out.item==255);
}
'''
# Exercise the pinned native presentation function and Pouya's actor-awaiting branch.
native=(ROOT/'dusklight/src/f_op/f_op_actor_mng.cpp').read_text()
a=native.index('fpc_ProcID fopAcM_createItemForPresentDemo(');b=native.index('{',a)+1;depth=1
while depth:
 depth+=(native[b]=='{')-(native[b]=='}');b+=1
present=native[a:b]
pouya=(ROOT/'dusklight/src/d/actor/d_a_npc_pouya.cpp').read_text()
a=pouya.index('                    if (mItemPartnerId == fpcM_ERROR_PROCESS_ID_e) {',pouya.index('switch (evt_id)'))
b=pouya.index('                    break;',a)
await_actor=pouya[a:b]
boundary=r'''
using u8=uint8_t;using u32=uint32_t;using fpc_ProcID=int;constexpr int fpcM_ERROR_PROCESS_ID_e=-1;
struct cXyz{};struct csXyz{};int displayed=255,presented=255,spawned=0,eventChanges=0;
#define TARGET_PC 1
#define IF_DUSK_ARG(...) , __VA_ARGS__
#define JUT_ASSERT(line,expr) assert(expr)
#define OS_REPORT(...) ((void)0)
void dComIfGp_event_setGtItm(int id){presented=id;}
int fopAcM_createDemoItem(const cXyz*,int,int,const csXyz*,int,const cXyz*,u8,u32){assert(displayed!=255);++spawned;return 77;}
bool fopAcM_IsExecuting(int id){return id==77;}
namespace dusk::mods{struct Result{uint8_t itemNo;uint32_t tag;};Result item_check_commit(const char*name,int vanilla,void*){
 ItemCheckInfo info{name};ItemCheckResolution result{uint8_t(vanilla),uint8_t(vanilla),false};remaster::progression::collections::resolve(&info,&result);displayed=result.display;return {result.item,0};}}
'''+present+'\n'+r'''
struct NativePouya{struct{cXyz pos;}current;int mItemPartnerId=-1,field_0xfce=0,mMode=0,mEvtNo=0;
 void evtChange(){++eventChanges;}void reward(int local_64){
'''+await_actor+'\n}};\n'
code=code.replace('int main(){',boundary+'\nint main(){',1)
# Insert assertions at the end of main, without modifying earlier production function bodies.
a=code.rfind('\n}')
code=code[:a]+r'''
 active=true;tuning::cfg.merchantRewards=true;saveReady=true;progression::decode(complete,progression::state);
 auto kept=progression::encode(progression::state);NativePouya jovani20,jovani60;
 jovani20.reward(dItemNo_DROP_BOTTLE_e);assert(jovani20.mMode==1&&jovani20.mEvtNo==1&&spawned==1&&presented==economy::rare::kStoryAcknowledgment&&displayed==dItemNo_ZORAS_JEWEL_e);
 gift={"jovani_reward_1",uint8_t(presented)};give(nullptr,&gift,nullptr);assert(progression::encode(progression::state)==kept);
 jovani60.reward(dItemNo_SILVER_RUPEE_e);assert(jovani60.mMode==1&&eventChanges==2&&spawned==2);
 gift={"jovani_reward_2",uint8_t(presented)};give(nullptr,&gift,nullptr);assert(progression::encode(progression::state)==kept);
 info={"bug:192"};assert(resolve(&info,&out)&&out.item==economy::rare::kStoryAcknowledgment&&out.display==dItemNo_ZORAS_JEWEL_e);
 assert(fopAcM_createItemForPresentDemo(nullptr,out.item,0,-1,-1,nullptr,nullptr,0)==77);
 assert(fopAcM_createItemForPresentDemo(nullptr,255,0,-1,-1,nullptr,nullptr,0)==-1);
'''+code[a:]
run(code)
print('Collection callbacks: all 4096 pair sets, native indexed flags, gifts, duplicates, pure previews, load/ledger safety and 20-shard budget passed')
