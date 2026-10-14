"""Production cache/purchase callbacks: finite credit, previews, migration and delivery."""
import re
from fixture import ROOT, function, run
source=(ROOT/'src/economy/rare.cpp').read_text()
items=(ROOT/'dusklight/include/d/d_item_data.h').read_text()
ids={n:int(v,16) for v,n in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(dItemNo_\w+)',items)}
selected=''.join(function(source,n) for n in ['enabled','ensure','owned','can_purchase','purchase','resolve','give','reset'])
constants='\n'.join(f'constexpr int {n}={ids[n]};' for n in set(re.findall(r'dItemNo_\w+',selected))|{'dItemNo_RED_BOTTLE_e','dItemNo_KAKERA_HEART_e','dItemNo_ZORAS_JEWEL_e'})
code=r'''
#include <cassert>
#include <cstring>
#include <set>
#include <string>
#include "economy/rare_model.hpp"
struct ModContext{};ModContext* mod_ctx=nullptr;constexpr int MOD_OK=0,MOD_ERROR=1,COLLECT_HYLIAN_SHIELD=2,ITEM_GIVE_RESOLVE=2,ITEM_GIVE_SILENT=1;
bool active=true,light=false,lakebed=false,shield=false,armor=false;bool is_active(){return active;}
std::string stage="R_SP109";int room=3,persisted=0,gives=0;bool fail=false;
const char* dComIfGp_getStartStageName(){return stage.c_str();}struct daAlink_c{}link;
auto* daAlink_getAlinkActorClass(){return &link;}int fopAcM_GetRoomNo(daAlink_c*){return room;}
bool dComIfGs_isCollectShield(int){return shield;}bool dComIfGs_isCollectClothes(int){return armor;}
void dComIfGs_setCollectShield(int){shield=true;}void dComIfGs_setCollectClothes(int){armor=true;}
bool dComIfGs_isLightDropGetFlag(int){return light;}bool dComIfGs_isCollectCrystal(int){return lakebed;}
namespace remaster::tuning{struct Values{bool removeCurrency=true,flaskEnabled=true;}cfg;auto& get(){return cfg;}}
namespace remaster::progression{Model state;bool valid=true;bool ready(){return valid;}Model& model(){return state;}void persist(){++persisted;}}
namespace remaster::progression::growth{bool ensure(){if(!remaster::progression::ready()||!active)return false;remaster::progression::state.migrate_growth(15);return true;}}
struct ItemCheckInfo{const char* name;uint8_t vanilla_item;};struct ItemCheckResolution{uint8_t item,display;bool animation;};struct ItemGiveInfo{const char* check_name;uint8_t item;};
struct Items{int give_item(ModContext*,const char*,uint8_t,int flags){assert(flags==(ITEM_GIVE_RESOLVE|ITEM_GIVE_SILENT));++gives;return fail?MOD_ERROR:MOD_OK;}}items;auto* svc_item=&items;
'''+constants+'\nnamespace remaster::economy::rare{std::set<std::string>g_pending;\n'+selected+'}\n'
reward_source=(ROOT/'src/progression/rewards.cpp').read_text()
world_checks=re.search(r'constexpr std::array<std::string_view, \d+> kFlaskChecks = \{.*?\};',reward_source,re.S).group()
code+=r'''
namespace remaster::resources::ammo{bool removes_pickup(int){return false;}}
namespace remaster::economy::merchants{bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}}
namespace remaster::progression::treasures {bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}bool custom_item(uint8_t){return false;}}
namespace remaster::progression::collections{bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}}
namespace remaster::progression{constexpr uint8_t kFlaskShard=0x13,kStaminaFragment=0x14;
'''+world_checks+function(reward_source,'rupee')+function(reward_source,'resolve')+'}\n'
code+=r'''
int main(){using namespace remaster;using namespace economy::rare;
 ItemCheckInfo check{kCaches[0].data(),5};ItemCheckResolution out{};assert(resolve(&check,&out)&&out.item==kRupeeCache);
 assert(progression::state.claimed.empty()&&progression::state.rareRupees==0&&!progression::state.growthReady); // previews never migrate/award
 check.vanilla_item=0x43;assert(!resolve(&check,&out));check.vanilla_item=5;
 for(auto name:kCaches){ItemGiveInfo gift{name.data(),kRupeeCache};give(nullptr,&gift,nullptr);give(nullptr,&gift,nullptr);}
 assert(progression::state.rareRupees==300&&progression::state.claimed.size()==21);
 ItemGiveInfo drop{nullptr,kRupeeCache},repeat{"minigame:repeat",kRupeeCache};give(nullptr,&drop,nullptr);give(nullptr,&repeat,nullptr);assert(progression::state.rareRupees==300);
 assert(!can_purchase(kOffers[0]));light=true;assert(can_purchase(kOffers[0]));
 fail=true;assert(!purchase(kOffers[0])&&g_pending.empty()&&progression::state.rareRupees==300);fail=false;
 assert(purchase(kOffers[0])&&!purchase(kOffers[0])&&progression::state.rareRupees==300);
 ItemGiveInfo delivered{kOffers[0].check,kOffers[0].item};give(nullptr,&delivered,nullptr);give(nullptr,&delivered,nullptr);
 assert(shield&&progression::state.rareRupees==240&&!can_purchase(kOffers[0]));
 stage="R_SP160";room=0;assert(!can_purchase(kOffers[1]));lakebed=true;assert(purchase(kOffers[1]));
 delivered={kOffers[1].check,kOffers[1].item};give(nullptr,&delivered,nullptr);assert(armor&&progression::state.rareRupees==60);
 progression::Model loaded;assert(progression::decode(progression::encode(progression::state),loaded)&&loaded.rareRupees==60);
 // Same-slot load can discard our pending set. Dispatch rechecks actual saved funds before granting.
 armor=false;progression::state.claimed.erase(kOffers[1].check);progression::state.rareRupees=0;reset(nullptr,0,nullptr);
 check={kOffers[1].check,kOffers[1].item};assert(resolve(&check,&out)&&out.item==255);delivered={kOffers[1].check,255};give(nullptr,&delivered,nullptr);assert(!armor&&progression::state.rareRupees==0);
 check={kOffers[0].check,kOffers[0].item};tuning::cfg.removeCurrency=false;assert(resolve(&check,&out)&&out.item==255);tuning::cfg.removeCurrency=true;
 active=false;assert(resolve(&check,&out)&&out.item==255);assert(progression::resolve(nullptr,&check,&out,nullptr)&&out.item==255);check={kCaches[0].data(),5};assert(!resolve(&check,&out));active=true;
 // Credit previously opened caches once and preserve legacy relic provenance.
 progression::Model old;old.claim(kCaches[0].data(),0x14);old.claim("chest:F_SP115:6",0x38);old.claim("chest:F_SP117:2",0x39);
 assert(migrate_economy(old)&&old.rareRupees==45&&old.claimed.contains("collection:bugs:3")&&old.claimed.contains("collection:poes:20"));
 assert(migrate_economy(old)&&old.rareRupees==45);
 progression::Model full;for(int i=0;i<512;++i){auto n="full:"+std::to_string(i);assert(full.claim(n.c_str(),255));}
 auto original=progression::encode(full);assert(!migrate_economy(full)&&progression::encode(full)==original); // no repeated credit if ledger is full
 assert(!award_cache(full,kCaches[0].data())&&full.rareRupees==0);
}
'''
run(code)
print('Rare economy: 20 finite caches, pure previews, no repeatable income, 300/240 budget, delivery-only spending, dispatch/load checks, native ownership and atomic legacy migration passed')
