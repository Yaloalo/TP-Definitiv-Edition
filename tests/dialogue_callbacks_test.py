"""Production dialogue provider/binding plus pinned native fundraising input path."""
import re
from fixture import ROOT,function,run
source=(ROOT/'src/economy/dialogue.cpp').read_text()
def extract(name):
 m=re.search(r'^(?:ModResult|Topic|const char\*|void|bool|HookAction) '+name+r'\(',source,re.M);a=m.start();b=source.index('{',a)+1;depth=1
 while depth:
  depth+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[a:b]+'\n'
selected=''.join(extract(n) for n in ['enabled','speaker_topic','current_topic','body','override_text','register_prompt','bound_post','render_pre'])
profiles=set(re.findall(r'fpcNm_\w+',selected));profile_header=(ROOT/'dusklight/include/f_pc/f_pc_name.h').read_text();ids={name:int(n,16) for n,name in re.findall(r'/\*\s*(0x\w+)\s*\*/\s*X\((fpcNm_\w+)\)',profile_header)}
constants='\n'.join(f'constexpr int {name}={ids[name]};' for name in profiles)
code=r'''
#include <cassert>
#include <set>
#include <map>
#include <cstring>
#include "economy/dialogue_model.hpp"
#include "economy/dialogue_table.hpp"
struct ModContext{};ModContext*mod_ctx=nullptr;using MessageOverrideHandle=uint64_t;using u16=uint16_t;using u8=uint8_t;using BOOL=int;using ModResult=int;constexpr int MOD_OK=0,MOD_ERROR=1,MSGTAG_INPUT_VALUE=37;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};bool active=true,mart=true;bool is_active(){return active;}
namespace remaster::tuning{struct Values{bool removeCurrency=true,economyDialogue=true;}cfg;auto&get(){return cfg;}}
namespace mods{template<class T>T arg(void*p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}template<class T>T&arg_ref(void*p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}namespace log{template<class...T>void warn(T...){}}}
struct fopAc_ac_c{int profile=0;};struct daAlink_c:fopAc_ac_c{}link;fopAc_ac_c speaker;fopAc_ac_c*pt1=&speaker,*pt2=nullptr;auto*daAlink_getAlinkActorClass(){return &link;}auto*dComIfGp_event_getPt1(){return pt1;}auto*dComIfGp_event_getPt2(){return pt2;}int fopAcM_GetName(fopAc_ac_c*a){return a->profile;}std::string stage="R_SP127";const char*dComIfGp_getStartStageName(){return stage.c_str();}bool daNpcF_chkEvtBit(int i){assert(i==124);return mart;}
struct MessageOverrideContext{uint16_t group,message_id;uint8_t language;const uint8_t*original_text;size_t original_text_size;};struct MessageTextData{const uint8_t*text;size_t text_size;};
struct Messages{int calls=0,removed=0,failAt=0;int override_message_fn(ModContext*,int,int,int,bool(*)(ModContext*,const MessageOverrideContext*,MessageTextData*,void*),void*,uint64_t*h){++calls;if(calls==failAt)return MOD_ERROR;*h=calls;return MOD_OK;}void remove_override(ModContext*,uint64_t){++removed;}}messagesService;auto*svc_message=&messagesService;
'''+constants+r'''
namespace remaster::economy::dialogue{std::set<std::pair<uint16_t,uint16_t>>g_registered;std::vector<uint8_t>g_text;std::map<std::pair<const void*,uint16_t>,uint64_t>g_resources;
'''+selected+r'''
}
// The native donation path: preserve the original indexed flags and confirmation/cancel behavior.
struct dSv_event_flag_c{static inline std::array<u16,1000>saveBitLabels{};};struct dSv_event_tmp_flag_c{static inline std::array<u16,100>tempBitLabels{};};
std::set<int>flags,tmp;int fund=0,total=0,debits=0;bool shop=false,pressedA=true,pressedB=false;
bool dComIfGs_isEventBit(int id){return flags.contains(id);}void dComIfGs_onEventBit(int id){flags.insert(id);}bool dComIfGs_isTmpBit(int id){return tmp.contains(id);}void dComIfGs_offTmpBit(int id){tmp.erase(id);}
void dComIfGp_setItemRupeeCount(int){++debits;}void dMeter2Info_onShopTalkFlag(){shop=true;}void dMeter2Info_offShopTalkFlag(){shop=false;}
void dMsgObject_addFundRaising(int n){fund+=n;}void dMsgObject_setFundRaising(int n){fund=n;}int dMsgObject_getFundRaising(){return fund;}void dMsgObject_addTotalPayment(int n){total+=n;}
using fpcLyIt_JudgeFunc=void*(*)(void*,void*);void*dMsgObject_searchSSItem(void*,void*){return nullptr;}void fpcM_Search(fpcLyIt_JudgeFunc,void*){}
struct mDoCPd_c{static bool getTrigA(int){return pressedA;}static bool getTrigB(int){return pressedB;}};
struct dMsgObject_c;struct jmessage_tReference{dMsgObject_c*object=nullptr;dMsgObject_c*getObjectPtr(){return object;}void inputNumber();};
struct dMsgObject_c{int input=0,status=0;bool payment=false;int field_0x199=0;jmessage_tReference*mpRefer=nullptr;int getInputValue(){return input;}void setInputValue(int n){input=n;}bool isSend(){return true;}void onPaymentFlagLocal(){payment=true;}int getFundRaisingValue(){return flags.contains(dSv_event_flag_c::saveBitLabels[376])?2000:1000;}void setStatusLocal(int n){status=n;}void inputProc();};
namespace remaster::economy{bool enabled(){return active&&remaster::tuning::get().removeCurrency;}
'''+function((ROOT/'src/economy/currency.cpp').read_text(),'donation')+r'''
}
void jmessage_tReference::inputNumber(){auto*p=this;void*args[]={&p};remaster::economy::donation(nullptr,args,nullptr,nullptr);}
'''+function((ROOT/'dusklight/src/d/d_msg_object.cpp').read_text(),'dMsgObject_c::inputProc')+r'''
int main(){using namespace remaster;using namespace economy::dialogue;
 const std::vector<uint8_t>original={'P','a','y',' ','2','0',' ','r','u','p','e','e','s','.',' ','B','r','e','a','k',' ','p','o','t','s','.',0};MessageOverrideContext info{7,9501,0,original.data(),original.size()};MessageTextData out{};
 speaker.profile=fpcNm_NPC_HENNA_e;assert(override_text(nullptr,&info,&out,nullptr));auto value=visible({out.text,out.text_size});assert(value.find("no fee")!=value.npos&&value.find("Break pots")!=value.npos&&!money(lower(value)));
 speaker.profile=0;assert(!override_text(nullptr,&info,&out,nullptr));info.group=2;info.message_id=5999;assert(override_text(nullptr,&info,&out,nullptr)&&visible({out.text,out.text_size}).find("sixty")!=value.npos);
 info.group=4;info.message_id=5440;mart=false;assert(override_text(nullptr,&info,&out,nullptr)&&visible({out.text,out.text_size}).find("restore")!=value.npos);mart=true;
 active=false;assert(!override_text(nullptr,&info,&out,nullptr));active=true;tuning::cfg.economyDialogue=false;assert(!override_text(nullptr,&info,&out,nullptr));tuning::cfg.economyDialogue=true;
 auto*p=&speaker;uint32_t numeric=37;void*args[]={&p,&numeric};struct{bool result=false;uint8_t canary[3]={7,8,9};}returned;
 assert(render_pre(nullptr,args,&returned.result,nullptr)==HOOK_SKIP_ORIGINAL&&returned.result&&returned.canary[0]==7&&returned.canary[2]==9);numeric=8;assert(render_pre(nullptr,args,&returned.result,nullptr)==HOOK_CONTINUE);
 register_prompt(4,6402);assert(messagesService.calls==6);register_prompt(4,6402);assert(messagesService.calls==6);messagesService.failAt=9;register_prompt(4,6403);assert(!g_registered.contains({4,6403})&&messagesService.removed==2);messagesService.failAt=0;register_prompt(4,6403);assert(g_registered.contains({4,6403}));
 for(int i=0;i<1000;++i)dSv_event_flag_c::saveBitLabels[i]=0x4000+i;dSv_event_tmp_flag_c::tempBitLabels[80]=0x5000;
 dMsgObject_c object;jmessage_tReference reference{&object};object.mpRefer=&reference;tmp.insert(0x5000);object.inputProc();assert(flags.contains(dSv_event_flag_c::saveBitLabels[376])&&fund==0&&object.status==14&&!shop);
 object.inputProc();assert(flags.contains(dSv_event_flag_c::saveBitLabels[124])&&fund==0); // original second restoration milestone
 tmp.clear();object.inputProc();assert(total==9999&&object.payment); // prayers retain native credit
 pressedA=false;pressedB=true;tmp.insert(0x5000);auto earned=total;object.inputProc();assert(total==earned&&!tmp.contains(0x5000)&&!shop);
}
'''
run(code)
print('Dialogue callbacks: NPC/mode gates, explicit/dynamic wording, bool ABI bounds, transactional registration and pinned native support/prayer completion passed')
