"""Production exploration receipt/resolver boundaries and generated catalogue integrity."""
import re,subprocess
from fixture import ROOT,function,run
subprocess.run(['python3',str(ROOT/'tools/generate_reward_catalog.py'),'--check'],check=True)
source=(ROOT/'src/progression/treasures.cpp').read_text()
code=r'''
#include <cassert>
#include <cstring>
#include <set>
#include "progression/treasure_model.hpp"
#include "resources/recharge_model.hpp"
struct ModContext{};ModContext*mod_ctx=nullptr;constexpr int MOD_OK=0,dItemNo_LINKS_SAVINGS_e=237,dItemNo_NONE_e=255,dItemNo_KAKERA_HEART_e=33,dItemNo_ZORAS_JEWEL_e=61;
bool active=true,saveReady=true;int persists=0,toasts=0;bool is_active(){return active;}std::set<int>flags;bool dComIfGs_isEventBit(int id){return flags.contains(id);}
struct ItemCheckInfo{const char*name;uint8_t vanilla_item;};struct ItemCheckResolution{uint8_t item,display;bool animation;};struct ItemGiveInfo{const char*check_name;uint8_t item;};
struct UiToastDesc{const char*title_rml;const char*body_rml;};
#define UI_TOAST_DESC_INIT {}
struct Ui{int push_toast(ModContext*,UiToastDesc*){++toasts;return MOD_OK;}}ui;auto*svc_ui=&ui;
namespace remaster::tuning{struct Values{bool removeCurrency=true,removeConsumableDrops=true;}cfg;auto&get(){return cfg;}}
namespace remaster::progression{constexpr uint8_t kStaminaFragment=0x14;Model state;bool ready(){return saveReady;}Model&model(){return state;}void persist(){++persists;}}
namespace remaster::progression::growth{bool ensure(){if(!active||!saveReady)return false;remaster::progression::state.migrate_growth(15);return true;}}
namespace remaster::player{bool input_allowed(){return active;}}
namespace remaster::progression::treasures{
bool ensure();
'''+''.join(function(source,n) for n in ['removable','persistent','give','ensure','resolve','tick'])+r'''
}
int main(){using namespace remaster;using namespace progression::treasures;ItemCheckResolution out{};
 auto original=progression::encode(progression::state);
 for(const auto& r:kFinds){ItemCheckInfo check{r.check,r.original};bool replaced=resolve(&check,&out);
  assert(replaced==(r.kind!=Kind::Flask&&r.kind!=Kind::Rupees));if(replaced)assert(out.item==(r.kind==Kind::Growth?0x14:kRelicFind));
  check.vanilla_item=0x20;assert(!resolve(&check,&out)); // preserve keys even at assigned coordinates
 }
 assert(progression::encode(progression::state)==original); // all previews are pure
 for(const auto& r:kFinds)if(r.kind==Kind::Craft||r.kind==Kind::Ancient||r.kind==Kind::Sunward||r.kind==Kind::Mirror){ItemGiveInfo info{r.check,kRelicFind};give(nullptr,&info,nullptr);give(nullptr,&info,nullptr);}
 assert(progression::state.relics[0]==146&&progression::state.relics[1]==84&&progression::state.growthFragments==0&&toasts==232);
 assert(progression::state.claimed.contains(kSunwardRelic)&&progression::state.claimed.contains(kMirrorRelic));tick();assert((progression::state.honors&3)==3);
 for(int i=192;i<216;++i){auto name="bug:"+std::to_string(i);ItemGiveInfo info{name.c_str(),kGildedThread};give(nullptr,&info,nullptr);give(nullptr,&info,nullptr);}assert(progression::state.relics[2]==24&&progression::state.growthFragments==0);
 auto complete=progression::encode(progression::state);ItemGiveInfo bad{nullptr,kRelicFind},repeat{"repeat:race",kRelicFind};give(nullptr,&bad,nullptr);give(nullptr,&repeat,nullptr);assert(progression::encode(progression::state)==complete);
 ItemGiveInfo prize{"star_reward_2",0x53};give(nullptr,&prize,nullptr);assert(progression::state.honors&4);flags={0x3b10,0x5908,0x3e40};ensure();assert(progression::state.honors==63);
 // Unknown persistent checks have finite crafting value, never blanket growth; unsaved drops/shop stock do not.
 ItemCheckInfo unknown{"chest:D_MN05:99",4};assert(resolve(&unknown,&out)&&out.item==kRelicFind);
 for(const char*name:{"chest:D_MN05:255","freestanding:F_SP108:-1","shop:R_SP01:1:4","drop:pot","race:repeat"}){unknown.name=name;assert(!resolve(&unknown,&out));}
 active=false;unknown.name=kFinds[0].check;assert(!resolve(&unknown,&out));active=true;saveReady=false;complete=progression::encode(progression::state);give(nullptr,&prize,nullptr);assert(progression::encode(progression::state)==complete);
}
'''
run(code)
print('278 assigned checks: protected native items, pure previews, finite material receipts, bug components, honors, missing provenance and mode/save gates passed')
