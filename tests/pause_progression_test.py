"""Execute native equipment-menu input and rendering callbacks with host doubles."""
from fixture import ROOT, function, run
source=(ROOT/'src/progression/pause.cpp').read_text()
code=r'''
#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <vector>
#include "progression/model.hpp"
#include "economy/merchant_model.hpp"
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};constexpr int MOD_OK=0,PAD_1=0;
constexpr int COLLECT_HYLIAN_SHIELD=2,SLOT_5=5,dItemNo_HAWK_EYE_e=0x3e,dItemNo_NORMAL_BOMB_e=0x70,dItemNo_WATER_BOMB_e=0x71,dItemNo_POKE_BOMB_e=0x72;
ModContext* mod_ctx=nullptr;bool active=true,visible=false;bool is_active(){return active;}
namespace mods{template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
struct Ui{int is_any_document_visible(ModContext*,bool* result){*result=visible;return MOD_OK;}}ui;auto* svc_ui=&ui;
struct dMenu_Collect2D_c{int sub=0;int getSubWindowOpenCheck(){return sub;}};
struct mDoCPd_c{static inline int trig=0;static int getTrigZ(int){return trig&1;}static int getTrigB(int){return trig&2;}
static int getTrigA(int){return trig&4;}static int getTrigX(int){return trig&8;}};
namespace remaster::progression{Model state;bool valid=true;bool ready(){return valid;}Model& model(){return state;}}
namespace remaster::progression::collections{int pairs(){return 12;}int souls(){return 60;}}
namespace remaster::progression::growth{bool ensure(){return active&&remaster::progression::ready();}bool allocate(bool s){return ensure()&&remaster::progression::model().allocate(s);}}
namespace remaster::tuning{struct Values{int staminaMax=100,flaskStartingCharges=3,flaskMaximumCharges=8,flaskHealingHearts=3;}cfg;auto& get(){return cfg;}}
namespace fmt{template<class...T>std::string format(const char* p,T...a){return std::vformat(p,std::make_format_args(a...));}}
namespace JUtility{struct TColor{int r,g,b,a;};}
struct Font{std::vector<std::string>text;float bottom=0;void setGX(){}void setCharColor(JUtility::TColor){}
void drawString_scale(float,float y,float,float h,const char* s,bool){text.emplace_back(s);bottom=std::max(bottom,y+h);}}font;
struct Port{int restored=0;void setup2D(){++restored;}}port;
Port* dComIfGp_getCurrentGrafPort(){return &port;}Font* mDoExt_getSubFont(){return &font;}Font* mDoExt_getMesgFont(){return &font;}
void J2DFillBox(float,float,float,float,JUtility::TColor){}
bool dComIfGs_isCollectShield(int){return true;}bool dComIfGs_isCollectClothes(int){return true;}
int dComIfGs_getItem(int,bool){return 0x3e;}int dComIfGs_getBombMax(int){return 60;}
namespace remaster::progression::pause{bool g_open=false;dMenu_Collect2D_c* g_owner=nullptr;
'''
code+=''.join(function(source,n) for n in ['reset','available','wait_pre','close_pre','draw_post'])+'}\n'
code+=r'''
int main(){using namespace remaster::progression;using namespace pause;state.migrate_growth(15);state.add_growth(10);
 dMenu_Collect2D_c menu;auto* p=&menu;void* a[]={&p};
 assert(wait_pre(nullptr,a,nullptr,nullptr)==HOOK_CONTINUE&&!g_open);
 mDoCPd_c::trig=1;assert(wait_pre(nullptr,a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&g_open);
 mDoCPd_c::trig=4;wait_pre(nullptr,a,nullptr,nullptr);assert(state.healthPoints==1&&state.unassigned()==1);
 mDoCPd_c::trig=8;wait_pre(nullptr,a,nullptr,nullptr);assert(state.staminaPoints==1&&state.unassigned()==0);
 wait_pre(nullptr,a,nullptr,nullptr);assert(state.staminaPoints==1); // cannot overspend
 for(const auto& r:remaster::economy::merchants::kRelics)state.claim(r.check,r.id);
 state.equipmentUpgrades=7;draw_post(nullptr,a,nullptr,nullptr);assert(font.bottom<400&&font.text.size()==21&&port.restored==2);
 bool health=false,stamina=false,flask=false,gear=false;
 for(auto& line:font.text){health|=line.find("Health: 4")!=std::string::npos;stamina|=line.find("Stamina: 120")!=std::string::npos;
 flask|=line.find("Sacred Flask")!=std::string::npos;gear|=line.find("Bomb capacities")!=std::string::npos;}
 assert(health&&stamina&&flask&&gear);
 for(int gate=0;gate<4;++gate){font.text.clear();active=gate!=0;visible=gate==1;valid=gate!=2;menu.sub=gate==3?1:0;
 assert(wait_pre(nullptr,a,nullptr,nullptr)==HOOK_CONTINUE);draw_post(nullptr,a,nullptr,nullptr);assert(font.text.empty()&&!g_open);}
 active=valid=true;visible=false;menu.sub=0;mDoCPd_c::trig=1;wait_pre(nullptr,a,nullptr,nullptr);
 mDoCPd_c::trig=2;assert(wait_pre(nullptr,a,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&!g_open); // B closes sheet, not the native menu
 mDoCPd_c::trig=1;wait_pre(nullptr,a,nullptr,nullptr);close_pre(nullptr,a,nullptr,nullptr);assert(!g_open&&!g_owner);
 mDoCPd_c::trig=0;draw_post(nullptr,a,nullptr,nullptr);assert(font.text.size()==1&&font.text[0].find("Z: Growth")!=std::string::npos);
 reset(nullptr,1,nullptr);assert(!g_owner&&!g_open);
}
'''
run(code)
print('Pause callbacks: native Z/A/X/B navigation, upgrade accounting, health/stamina/flask/gear display, bounded panel, graphics restore and submenu/UI/save isolation passed')
