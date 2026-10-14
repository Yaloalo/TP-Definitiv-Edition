"""Production artisan transactions: native gear gates, location, mode and atomic costs."""
from fixture import ROOT,function,run
source=(ROOT/'src/economy/guild.cpp').read_text()
code=r'''
#include <cassert>
#include <cstring>
#include "economy/guild_model.hpp"
#include "resources/recharge_model.hpp"
struct ModContext{};ModContext*mod_ctx=nullptr;
constexpr int SLOT_15=15,dItemNo_BOW_e=0x43,dItemNo_BOMB_BAG_LV1_e=0x50;
bool mart=true;bool daNpcF_chkEvtBit(int id){assert(id==124);return mart;}
bool active=true,readySave=true,running=true,forest=false,bow=false,armor=false;int room=1,persists=0,toasts=0;std::string stage="R_SP01";uint8_t bags[3]={255,255,255};
struct daAlink_c{}link;auto*daAlink_getAlinkActorClass(){return &link;}int fopAcM_GetRoomNo(daAlink_c*){return room;}
const char*dComIfGp_getStartStageName(){return stage.c_str();}bool dComIfGs_isCollectCrystal(int){return forest;}bool dComIfGs_isItemFirstBit(int){return bow;}bool dComIfGs_isCollectClothes(int){return armor;}uint8_t dComIfGs_getItem(int slot,bool){return bags[slot-15];}
namespace remaster::progression{Model state;bool ready(){return readySave;}Model&model(){return state;}void persist(){++persists;}}
namespace remaster::progression::treasures{bool ensure(){return active&&readySave;}}
namespace remaster::economy::merchants{bool enabled(){return active;}}
namespace remaster::player{bool gameplay_running(){return running;}}
struct UiToastDesc{const char*title_rml;const char*body_rml;};
#define UI_TOAST_DESC_INIT {}
struct Ui{void push_toast(ModContext*,UiToastDesc*){++toasts;}}ui;auto*svc_ui=&ui;
namespace remaster::economy::guild{
'''+''.join(function(source,n) for n in ['equipment','can_craft','craft'])+r'''
}
int main(){using namespace remaster;using namespace economy::guild;
 progression::state.migrate_growth(15);progression::state.relics={146,84,24};
 assert(!can_craft(kRecipes[0]));forest=true;assert(can_craft(kRecipes[0]));auto before=progression::encode(progression::state);running=false;assert(!craft(kRecipes[0])&&progression::encode(progression::state)==before);running=true;
 room=0;assert(!craft(kRecipes[0]));room=1;assert(craft(kRecipes[0])&&!craft(kRecipes[0])&&persists==1&&toasts==1);
 stage="R_SP109";room=3;assert(!can_craft(kRecipes[1]));bow=true;assert(craft(kRecipes[1]));
 room=1;assert(!can_craft(kRecipes[2]));bags[0]=0x70;assert(craft(kRecipes[2]));
 stage="R_SP160";room=0;assert(!can_craft(kRecipes[3]));armor=true;mart=false;assert(!can_craft(kRecipes[3]));mart=true;assert(!can_craft(kRecipes[3]));progression::state.equipmentUpgrades|=1;
 progression::state.claim(progression::treasures::kSunwardRelic,255);assert(craft(kRecipes[3]));assert(!can_craft(kRecipes[4]));progression::state.claim(progression::treasures::kMirrorRelic,255);assert(craft(kRecipes[4]));
 assert(!craft(kRecipes[7]));stage="R_SP109";room=3;assert(craft(kRecipes[5]));stage="R_SP160";room=0;assert(craft(kRecipes[6])&&craft(kRecipes[7]));
 assert(progression::state.relics[0]==12&&progression::state.relics[1]==0&&progression::state.relics[2]==0&&progression::state.honors==448&&persists==8&&toasts==8);
 active=false;before=progression::encode(progression::state);assert(!craft(kRecipes[0])&&progression::encode(progression::state)==before);
}
'''
run(code)
print('Artisan callbacks: gear/story/location gates, component spending, ordered ward/honor upgrades, duplicates and mode isolation passed')
