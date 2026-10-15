"""Execute production layout and currency callbacks across viewport sizes."""
from fixture import ROOT, function, run
layout=(ROOT/'src/hud/layout.cpp').read_text()
currency=(ROOT/'src/economy/currency.cpp').read_text()
code=r'''
#include <cassert>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
using f32=float;using u8=uint8_t;using Mtx=float[3][4];struct Vec{float x,y,z;};
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
bool active=true;bool is_active(){return active;}
namespace mods{template<class T>T arg(void*p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
struct J2DScreen{};
struct CPaneMgr{
 float x=0,y=0,sx=1,sy=1,w=160,h=100,px=1,py=1;bool visible=true;
 CPaneMgr*getPanePtr(){return this;}
 float getTranslateX(){return x;}float getTranslateY(){return y;}
 float getScaleX(){return sx;}float getScaleY(){return sy;}
 void scale(float a,float b){sx=a;sy=b;}void translate(float a,float b){x=a;y=b;}
 void paneTrans(float,float){assert(false && "paneTrans rewrites bounds; use translate for saved translations");}
 Vec getGlobalVtx(Mtx* m,u8 corner,bool,int){(*m)[0][0]=sx*px;(*m)[1][1]=sy*py;return {px*(x+((corner&1)?w*sx:0)),py*(y+((corner&2)?h*sy:0)),0};}
};
struct dMeter2Draw_c{
 J2DScreen*mpScreen=nullptr;CPaneMgr *mpButtonParent=nullptr,*mpButtonCrossParent=nullptr,*mpRupeeKeyParent=nullptr;
 int rupees=-1;void drawRupee(int n){rupees=n;}
};
struct mDoGph_gInf_c{static inline float bottom=448;static float getSafeMaxYF(){return bottom;}};
struct NativeMap{float getMapDispEdgeTop(){return mDoGph_gInf_c::bottom-133;}} map;
struct NativeMeter{NativeMap*mpMap=&map;}nativeMeter;NativeMeter*dMeter2Info_getMeterClass(){return &nativeMeter;}
namespace remaster::tuning{struct Values{int64_t hudPromptScalePercent=75;}cfg;auto&get(){return cfg;}}
namespace remaster::hud::layout {
struct Snapshot { CPaneMgr* pane=nullptr;float x=0,y=0,sx=1,sy=1;};
std::array<Snapshot,3>g_saved{};dMeter2Draw_c*g_meter=nullptr;
'''
code+=''.join(function(layout,n) for n in ['restore','place','meter_pre','screen_pre','meter_post','map_bottom'])
code+=r'''
}
namespace remaster::progression{bool valid=true;struct Model{uint32_t rareRupees=42;}state;bool ready(){return valid;}Model&model(){return state;}}
namespace remaster::economy{bool enabled(){return active;}
'''+function(currency,'meter_pre')+r'''
}
int main(){using namespace remaster;using namespace hud::layout;
 J2DScreen screen,other;auto*sp=&screen;void* sa[]={&sp};
 CPaneMgr buttons,cross,rupees;buttons.x=400;buttons.y=25;cross.x=20;cross.y=160;cross.px=cross.py=1.5f;
 dMeter2Draw_c meter{&screen,&buttons,&cross,&rupees};auto*mp=&meter;void*ma[]={&mp};
 for(float bottom:{448.0f,720.0f,1024.0f}){
  mDoGph_gInf_c::bottom=bottom;
  for(int frame=0;frame<3;++frame){
   meter_pre(nullptr,ma,nullptr,nullptr);
   sp=&other;screen_pre(nullptr,sa,nullptr,nullptr);assert(buttons.sx==1);
   sp=&screen;screen_pre(nullptr,sa,nullptr,nullptr);
   assert(buttons.sx==0.75f&&std::abs(buttons.x+buttons.w*buttons.sx-560)<0.001f&&buttons.y==25);
   Mtx m;assert(std::abs(cross.getGlobalVtx(&m,2,false,0).y-(bottom-145))<0.001f);
   screen_pre(nullptr,sa,nullptr,nullptr);assert(buttons.sx==0.75f); // no repeated shrink
   meter_post(nullptr,ma,nullptr,nullptr);
   assert(buttons.sx==1&&buttons.x==400&&buttons.y==25&&cross.x==20&&cross.y==160);
   float map=421;assert(map_bottom(nullptr,nullptr,&map,nullptr)==HOOK_SKIP_ORIGINAL&&map==bottom-22);
  }
 }
 economy::meter_pre(nullptr,ma,nullptr,nullptr);assert(meter.rupees==42);
 progression::state.rareRupees=100000;economy::meter_pre(nullptr,ma,nullptr,nullptr);assert(meter.rupees==9999);
 progression::valid=false;meter.rupees=17;economy::meter_pre(nullptr,ma,nullptr,nullptr);assert(meter.rupees==17);
 active=false;meter_pre(nullptr,ma,nullptr,nullptr);screen_pre(nullptr,sa,nullptr,nullptr);assert(buttons.sx==1);
 float native=421;assert(map_bottom(nullptr,nullptr,&native,nullptr)==HOOK_CONTINUE&&native==421);
 economy::meter_pre(nullptr,ma,nullptr,nullptr);assert(meter.rupees==17);
}
'''
run(code)
print('HUD: corner anchors, tall viewports, repeated draw restore, mode isolation and native rare-rupee digits passed')

# Compile the real context class with a stateful GPU-port double: projection
# must be established on entry and the native port restored on scope exit.
header=(ROOT/'src/hud/draw_context.hpp').read_text()
context=header[header.index('namespace remaster::hud {'):]
run(r'''
#include <algorithm>
#include <cassert>
#include <cmath>
constexpr float FB_WIDTH=608,FB_HEIGHT=448;
int current=0;float orthoX=0,orthoY=0,orthoW=0,orthoH=0;
struct J2DGrafContext{virtual void setPort(){current=1;}} previous;
J2DGrafContext*dComIfGp_getCurrentGrafPort(){return &previous;}
struct J2DOrthoGraph:J2DGrafContext{
 J2DOrthoGraph(float,float,float,float,float,float){}
 void setOrtho(float x,float y,float w,float h,float,float){orthoX=x;orthoY=y;orthoW=w;orthoH=h;}
 void setPort()override{current=2;}
};
struct mDoGph_gInf_c{static inline float w=608,h=448;
 static float getWidthF(){return w;}static float getHeightF(){return h;}
 static float getMinXF(){return (608-w)/2;}static float getMinYF(){return (448-h)/2;}};
'''+context+r'''
int main(){for(float aspect:{0.75f,608.0f/448,16.0f/9,21.0f/9}){
 mDoGph_gInf_c::w=448*aspect;
 {remaster::hud::DrawContext c(true);assert(current==2);assert(orthoX<=0.01f&&orthoY<=0.01f);
 assert(orthoW>=607.99f&&orthoH>=447.99f);assert(std::abs(orthoW/orthoH-aspect)<0.001f);}
 assert(current==1);
 {remaster::hud::DrawContext c;assert(current==2);assert(orthoW==mDoGph_gInf_c::w&&orthoH==448);}
 assert(current==1);
}}
''')
print('HUD draw context: explicit projection, unstretched menu across aspect ratios, native port restoration passed')

prompt=(ROOT/'src/hud/prompt.cpp').read_text()
run(r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
using u8=uint8_t;struct ModContext{};enum HookAction{HOOK_CONTINUE};
namespace JUtility{struct TColor{int r,g,b,a;};}
struct JUTFont {
 struct TWidth{int field_0x0=2,field_0x1=6;};
 bool fixed=false,gx=false;std::vector<std::string> draws;std::vector<float> ys;
 bool isLeadByte(unsigned c){return c==0x81;}bool isFixed(){return fixed;}int getFixedWidth(){return 10;}int getCellWidth(){return 11;}
 void getWidthEntry(unsigned,TWidth*w){*w={2,6};}
 void setGX(){gx=true;}void setCharColor(JUtility::TColor){}
 float drawString_scale(float x,float y,float,float,const char*s,bool flag){assert(gx&&flag&&x>100&&y>0);draws.emplace_back(s);ys.push_back(y);return 0;}
}font;
JUTFont*mDoExt_getSubFont(){return &font;}JUTFont*mDoExt_getMesgFont(){return &font;}
struct mDoGph_gInf_c{static float getSafeWidthF(){return 608;}static float getSafeMinXF(){return 0;}static float getSafeMaxXF(){return 608;}static float getSafeMaxYF(){return 448;}};
int boxes=0;void J2DFillBox(float x,float,float w,float,JUtility::TColor){++boxes;assert(x>150&&x+w<460);}
namespace remaster::hud{struct DrawContext{};namespace idle_fade{float opacity(){return 1;}}
std::vector<std::string>g_prompts;
'''+''.join(function(prompt,n) for n in ['text_width','begin_prompts','draw_queued','draw_prompt'])+r'''
}
int main(){using namespace remaster::hud;
 assert(text_width(&font,"abc",11)==18&&font.draws.empty());
 font.fixed=true;assert(text_width(&font,"abc",11)==30&&font.draws.empty());font.fixed=false;
 assert(text_width(&font,"\x81\x40",11)==6); // one native double-byte glyph
 begin_prompts(nullptr,nullptr,nullptr,nullptr);
 draw_prompt("Rest: hold L + R");draw_prompt("Ordonna: exchange health and stamina");draw_prompt("Rest: hold L + R");
 assert(font.draws.empty()); // collecting and measuring never draw
 draw_queued(nullptr,nullptr,nullptr,nullptr);
 assert(boxes==1&&font.draws.size()==2&&font.ys[1]-font.ys[0]==16);
 assert(font.draws[0].find("Rest")!=std::string::npos&&font.draws[1].find("Ordonna")!=std::string::npos);
 draw_queued(nullptr,nullptr,nullptr,nullptr);assert(boxes==1); // consumed once
 draw_prompt("Stale previous frame");begin_prompts(nullptr,nullptr,nullptr,nullptr);draw_queued(nullptr,nullptr,nullptr,nullptr);assert(boxes==1);
 draw_prompt("A long interaction hint that needs to wrap onto multiple lines while the map and rupee counter keep their own corners.");
 auto old=font.draws.size();draw_queued(nullptr,nullptr,nullptr,nullptr);assert(font.draws.size()>old+1);
 for(size_t i=old;i<font.draws.size();++i)assert(text_width(&font,font.draws[i],11)<=268);
}
''')
print('Interaction hints: non-rendering glyph measurement, merged simultaneous hints, wrapping and frame reset passed')
