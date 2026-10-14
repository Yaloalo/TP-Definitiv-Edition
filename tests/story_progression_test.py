"""Production story-sensitive checks; these fixtures are not a full game playthrough."""
import re
from fixture import ROOT, function, run

items = (ROOT / 'dusklight/include/d/d_item_data.h').read_text()
ids = dict((name, int(number, 16)) for number, name in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(dItemNo_\w+)', items))
rewards = (ROOT / 'src/progression/rewards.cpp').read_text()
checks = re.search(r'constexpr std::array<std::string_view, 20> kFlaskChecks = \{.*?\};', rewards, re.S).group()
common = r'''
#include <cassert>
#include <cstring>
#include <array>
#include <algorithm>
#include <string_view>
#include <set>
#include <cstdint>
#include "progression/model.hpp"
#include "resources/recharge_model.hpp"
using u8=uint8_t;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
bool active=true;bool is_active(){return active;}
namespace mods{template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
namespace remaster::tuning{struct Values{bool difficultyCurve=true,removeCurrency=true,removeConsumableDrops=true,flaskEnabled=true,removeHeartDrops=true,quickTransform=true;}cfg;auto& get(){return cfg;}}
namespace remaster::resources::ammo{bool removes_pickup(int n){return active&&remaster::tuning::get().removeConsumableDrops&&remaster::resources::ammo_pickup(n);}}
struct ItemCheckInfo{const char* name;uint8_t vanilla_item;};struct ItemCheckResolution{uint8_t item,display;bool animation;};
namespace remaster::economy::merchants{bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}bool custom_item(uint8_t){return false;}}
namespace remaster::economy::rare{bool resolve(const ItemCheckInfo*,ItemCheckResolution*){return false;}bool custom_item(uint8_t){return false;}}
namespace remaster::healing::snowpeak{bool allows_grant(){return false;}}
'''
constants = '\n'.join(f'constexpr int {name}={value};' for name, value in ids.items())
code = common + constants + '\nnamespace remaster::progression {\nconstexpr uint8_t kFlaskShard=0x13,kStaminaFragment=0x14;\n' + checks + '\n'
code += ''.join(function(rewards, name) for name in ['rupee', 'resolve', 'item_pre'])
code += r'''
}
int main(){using namespace remaster;using namespace progression;
 // All 256 native item IDs: only the currency/ammo categories resolve to new rewards.
 for(int item=0;item<256;++item){ItemCheckInfo info{"chest:D_MN05:99",uint8_t(item)};ItemCheckResolution out{255,255,false};
 const bool replaced=resolve(nullptr,&info,&out,nullptr);
 assert(replaced==(rupee(item)||resources::ammo::removes_pickup(item)));
 if(!replaced)assert(out.item==255);}
 // Native dungeon rewards use NONE. Resolving them must retain the native crystal/mirror branch.
 for(const char* name:{"boss:forest_temple","boss:goron_mines","boss:lakebed_temple","boss:arbiters_grounds","boss:snowpeak_ruins","boss:temple_of_time","boss:city_in_the_sky","boss:palace_of_twilight"}){
 ItemCheckInfo info{name,uint8_t(dItemNo_NONE_e)};ItemCheckResolution out{};assert(!resolve(nullptr,&info,&out,nullptr));}
 for(const char* name:{"shop:sera:0","chest:D_MN05:255","chest:D_MN05:-1"}){
 ItemCheckInfo info{name,1};ItemCheckResolution out{};assert(!resolve(nullptr,&info,&out,nullptr));}
 // Dungeon access/quest items must still call the original getItemFunc.
 for(int number:{dItemNo_SMALL_KEY_e,dItemNo_BOSS_KEY_e,dItemNo_KAKERA_HEART_e,dItemNo_UTAWA_HEART_e,
 dItemNo_PACHINKO_e,dItemNo_BOMB_BAG_LV1_e,dItemNo_BOW_e,dItemNo_HOOKSHOT_e,dItemNo_IRONBALL_e}){
 auto item=uint8_t(number);void* args[]={&item};assert(item_pre(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);}
 const auto before=Model{};Model m=before;ItemCheckInfo info{"chest:R_SP01:1",1};ItemCheckResolution out{};
 assert(resolve(nullptr,&info,&out,nullptr)&&out.item==kFlaskShard&&m.claimed.empty());
 tuning::cfg.removeCurrency=false;assert(!resolve(nullptr,&info,&out,nullptr));tuning::cfg.removeCurrency=true;
 active=false;assert(!resolve(nullptr,&info,&out,nullptr));
}
'''
run(code)

difficulty = (ROOT / 'src/core/difficulty.cpp').read_text()
code = common + r'''
std::set<int> crystals,mirrors;bool dComIfGs_isCollectCrystal(int n){return crystals.contains(n);}
bool dComIfGs_isCollectMirror(int n){return mirrors.contains(n);}
namespace remaster::difficulty {
'''+function(difficulty, 'dungeons_cleared')+r'''
}
int main(){using namespace remaster;
 // Every combination of native crystal/mirror completion bits, including out-of-order saves.
 for(int bits=0;bits<256;++bits){crystals.clear();mirrors.clear();int expected=0;
 for(int i=0;i<4;++i){if(bits&(1<<i)){crystals.insert(i);++expected;}if(bits&(1<<(i+4))){mirrors.insert(i);++expected;}}
 const auto c=crystals,m=mirrors;assert(difficulty::dungeons_cleared()==expected&&crystals==c&&mirrors==m);
 progression::Model p;p.bosses=difficulty::dungeons_cleared();assert(p.potency(3)==3+expected/2);
 progression::Model loaded;assert(progression::decode(progression::encode(p),loaded)&&loaded.bosses==p.bosses);}
}
'''
run(code)

quality = (ROOT / 'src/movement/quality.cpp').read_text()
code = common + r'''
struct dSv_event_flag_c{enum{M_077=77};};std::set<int> eventBits;
bool dComIfGs_isEventBit(int n){return eventBits.contains(n);}bool dComIfGp_checkPlayerStatus0(int,int){return false;}
struct MeterDraw{float alpha=1;float getButtonZAlpha(){return alpha;}}draw;
struct Meter{MeterDraw* getMeterDrawPtr(){return &draw;}}meter;Meter* dMeter2Info_getMeterClass(){return &meter;}
struct Midna{bool enabled=true;bool checkMetamorphoseEnableBase(){return enabled;}bool checkMetamorphoseEnable(){return enabled;}}midna;
struct daPy_py_c{static Midna* getMidnaActor(){return &midna;}};
struct daAlink_c{enum{FLG0_SWIM_UP=1,MODE_PLAYER_FLY=2,MODE_UNK_1000=0x1000};
 bool event=false,stage=false,wolf=false,ground=true,fly=false,magnet=false,riding=true,swimUp=true;int mEquipItem=0,transformations=0;
 struct Acch{bool* ground;bool ChkGroundHit(){return *ground;}}mLinkAcch{&ground};
 bool checkEventRun(){return event;}bool checkStageName(const char*){return stage;}bool checkNoResetFlg0(int){return swimUp;}
 bool checkModeFlg(int n){return n==MODE_PLAYER_FLY?fly:true;}bool checkMagneBootsOn(){return magnet;}
 bool checkMidnaRide(){return riding;}bool checkWolf(){return wolf;}void procCoMetamorphoseInit(){++transformations;}};
'''+constants+'\nnamespace remaster::movement::quality{\n'+function(quality, 'transform_before')+r'''
}
int main(){using namespace remaster;daAlink_c link;auto* p=&link;void* args[]={&p};
 // Input must never set the story unlock bit, or bypass it before the Master Sword milestone.
 assert(movement::quality::transform_before(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&link.transformations==0&&eventBits.empty());
 eventBits.insert(dSv_event_flag_c::M_077);const auto flags=eventBits;
 movement::quality::transform_before(nullptr,args,nullptr,nullptr);assert(link.transformations==1&&eventBits==flags);
 link.wolf=true;movement::quality::transform_before(nullptr,args,nullptr,nullptr);assert(link.transformations==2&&eventBits==flags);
 for(int gate=0;gate<10;++gate){link.transformations=0;link.event=gate==0;link.stage=gate==1;draw.alpha=gate==2?0:1;
 link.mEquipItem=gate==3?dItemNo_IRONBALL_e:0;link.swimUp=gate!=4;midna.enabled=gate!=5;
 link.ground=gate!=6;link.fly=gate==7;link.magnet=gate==8;link.riding=gate!=9;
 movement::quality::transform_before(nullptr,args,nullptr,nullptr);assert(link.transformations==0&&eventBits==flags);}
 active=false;assert(movement::quality::transform_before(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE&&eventBits==flags);
}
'''
run(code)
print('Story-sensitive production checks: 256 item IDs, native dungeon rewards, all 256 completion-bit combinations, save persistence and transformation unlock gates passed')
