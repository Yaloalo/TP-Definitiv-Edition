"""Production collection callbacks with indexed native quest flags and queued grants."""
import re
from fixture import ROOT, function, run
source=(ROOT/'src/progression/collections.cpp').read_text()
items=(ROOT/'dusklight/include/d/d_item_data.h').read_text()
ids={n:int(v,16) for v,n in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(dItemNo_\w+)',items)}
selected=''.join(function(source,n) for n in ['delivered','caught','notice','reset','give','ensure','pairs','souls','unpresented','present_bugs','rewards_available','claim_rewards','resolve'])
constants='\n'.join(f'constexpr int {n}={ids[n]};' for n in set(re.findall(r'dItemNo_\w+',selected)))
code=r'''
#include <cassert>
#include <charconv>
#include <set>
#include <format>
#include <array>
#include "progression/collection_model.hpp"
struct ModContext{};ModContext* mod_ctx=nullptr;constexpr int MOD_OK=0,MOD_ERROR=1,ITEM_GIVE_RESOLVE=2,ITEM_GIVE_SILENT=1;
struct ItemCheckInfo{const char* name;};struct ItemCheckResolution{uint8_t item,display;bool animation;};struct ItemGiveInfo{const char* check_name;uint8_t item;};
struct UiToastDesc{const char* title_rml=nullptr;const char* body_rml=nullptr;};
#define UI_TOAST_DESC_INIT {}
std::set<int> nativeFlags;std::set<int> ownedItems;int poeCount=0,persisted=0,toasts=0;bool active=true,saveReady=true,fail=false;
// These APIs take saveBitLabels indices, not raw bits. Reject the wrong API/encoding.
bool daNpcF_chkEvtBit(int id){assert(id>=0x191&&id<0x191+24);return nativeFlags.contains(id);}
void daNpcF_onEvtBit(int id){assert(id>=0x191&&id<0x191+24);nativeFlags.insert(id);}
bool dComIfGs_isItemFirstBit(int id){return ownedItems.contains(id);}int dComIfGs_getPohSpiritNum(){return poeCount;}
struct Ui{int push_toast(ModContext*,UiToastDesc*){++toasts;return MOD_OK;}}ui;auto* svc_ui=&ui;
struct Items{std::vector<std::pair<std::string,int>> queue;int give_item(ModContext*,const char* name,int id,int flags){assert(flags==3);if(fail)return MOD_ERROR;queue.emplace_back(name,id);return MOD_OK;}}items;auto* svc_item=&items;
namespace fmt{template<class...T>std::string format(std::string_view f,T...args){return std::vformat(f,std::make_format_args(args...));}}
namespace remaster::tuning{struct Values{int flaskStartingCharges=3,flaskMaximumCharges=8;}cfg;auto& get(){return cfg;}}
namespace remaster::progression{Model state;bool ready(){return saveReady;}Model& model(){return state;}void persist(){++persisted;}}
namespace remaster::progression::growth{void apply_health(){}}
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
 assert(present_bugs()&&pairs()==1&&unpresented()==0&&progression::state.growthFragments==2&&items.queue.size()==1);
 assert(!present_bugs()&&items.queue.size()==1);ItemGiveInfo gift{kBugs[0].check,kBugs[0].item};give(nullptr,&gift,nullptr);give(nullptr,&gift,nullptr);
 assert(progression::state.growthFragments==7&&!rewards_available(true));
 fail=true;poeCount=60;assert(!claim_rewards(false)&&g_pending.empty());fail=false;
 assert(claim_rewards(false)&&!claim_rewards(false)&&g_pending.size()==5);
 // The normal progression observer handles the one-fragment reward, merchants commit relic provenance.
 for(auto& [name,id]:items.queue){if(id==0x14)progression::state.claim(name.c_str(),id);if(id==0x38||id==0x39)progression::state.claim(name.c_str(),id);gift={name.c_str(),uint8_t(id)};give(nullptr,&gift,nullptr);}
 assert(progression::state.growthFragments==13&&progression::state.flaskShards==4&&progression::state.equipmentUpgrades==4);
 assert(progression::state.claimed.contains("collection:poes:20")&&!rewards_available(false));
 for(int i=2;i<24;++i){ownedItems.insert(dItemNo_M_BEETLE_e+i);}assert(present_bugs()&&pairs()==12);
 for(auto& [name,id]:items.queue){if(id==0x38)progression::state.claim(name.c_str(),id);gift={name.c_str(),uint8_t(id)};give(nullptr,&gift,nullptr);}
 assert(progression::state.flaskShards==8&&progression::state.capacity(3,8)==5&&progression::state.equipmentUpgrades==6);
 assert(progression::state.growthFragments==35); // 24 individual bugs + 5 pair bonus + 6 Poe growth
 for(int i=0;i<12;++i){auto n="world-shard:"+std::to_string(i);progression::state.claim(n.c_str(),0x13);}assert(progression::state.capacity(3,8)==8);
 auto complete=progression::encode(progression::state);assert(!present_bugs()&&!claim_rewards(true)&&!claim_rewards(false)&&progression::encode(progression::state)==complete);
 progression::Model loaded;assert(progression::decode(complete,loaded)&&loaded.equipmentUpgrades==6);
 // Native Jovani checks share the milestone ledger; incomplete, repeated or unavailable saves cancel delivery.
 reset(nullptr,0,nullptr);info={"jovani_reward_1"};assert(resolve(&info,&out)&&out.item==255);
 progression::state={};nativeFlags.clear();poeCount=19;assert(resolve(&info,&out)&&out.item==255);poeCount=20;assert(resolve(&info,&out)&&out.item==0x39);
 saveReady=false;assert(resolve(&info,&out)&&out.item==255);saveReady=true;
 info={"jovani_reward_2"};assert(resolve(&info,&out)&&out.item==255);poeCount=60;assert(resolve(&info,&out)&&out.item==0x3b);
 info={"bug:192junk"};assert(!resolve(&info,&out));info={"bug:191"};assert(!resolve(&info,&out));info={"bug:192"};assert(resolve(&info,&out)&&out.item==0x3b);
 // Legacy hand-ins migrate once; full ledger failure does not partially mutate native hand-ins.
 progression::state={};nativeFlags={0x191,0x192};assert(ensure()&&progression::state.growthFragments==2);assert(ensure()&&progression::state.growthFragments==2);
 progression::state={};progression::state.migrate_growth(15);for(int i=0;i<512;++i){auto n="full:"+std::to_string(i);progression::state.claim(n.c_str(),255);}before=progression::encode(progression::state);
 assert(!ensure()&&!present_bugs()&&!claim_rewards(false)&&progression::encode(progression::state)==before);assert(resolve(&info,&out));
 active=false;assert(!resolve(&info,&out));assert(!rewards_available(true));
}
'''
run(code)
print('Collection callbacks: all 4096 pair sets, native indexed flags, gifts, duplicates, pure previews, load/ledger safety and 20-shard budget passed')
