"""Test real economy/pacing callbacks while every built-in gameplay setting is absent."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
def function(source,name):
    m=re.search(r'^(?:HookAction|void|bool) '+name+r'\(',source,re.M); start=m.start()
    at=source.index('{',start)+1; depth=1
    while depth:
        if source[at]=='{': depth+=1
        elif source[at]=='}': depth-=1
        at+=1
    return source[start:at]+'\n'
currency=(root/'src/economy/currency.cpp').read_text()
pacing=(root/'src/pacing/quality.cpp').read_text()
fixture=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <cstdio>
using u8=uint8_t;using u16=uint16_t; using BOOL=int;
constexpr int FALSE=0,PAD_1=0; struct ModContext{};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}
template<class T>T& arg_ref(void* p,int i){return *static_cast<T*>(static_cast<void**>(p)[i]);}}
bool active=true; bool is_active(){return active;}
namespace remaster::tuning {struct Values {bool removeCurrency=true,instantSaves=true,fastTransitions=true,instantText=true;} cfg; auto& get(){return cfg;}}
struct mesg_flow_node_branch {u16 param=0;};
struct Object {int input=0; void setInputValue(int n){input=n;}};
struct jmessage_tReference {Object obj; int timer=7; Object* getObjectPtr(){return &obj;} void setSendTimer(int n){timer=n;}};
struct jmessage_tSequenceProcessor {int field_0xb2=0; jmessage_tReference ref; const jmessage_tReference* getReference(){return &ref;}};
struct mDoCPd_c {static inline bool hold=false; static bool getHoldB(int){return hold;}};
int syncResult=0,written=0;
struct MemCard {int SaveSync(){return syncResult;}} g_mDoMemCd_control;
namespace dusk::mods::svc {void save_slot_written(int){++written;}}
constexpr int Z2SE_SY_FILE_SAVE_LEVEL=0;
void mDoAud_seStartLevel(int,void*,int,int){}
struct dMenu_save_c {enum {PROC_MEMCARD_DATA_SAVE_WAIT2=2};int mWaitTimer=20,mCmdState=0,mMenuProc=0,mSelectedFile=0; void memCardDataSaveWait();};
#define TARGET_PC 1
struct dOvlpFd3_c {int mTimer=-1;}; struct Z2SceneMgr {int load1stWait=10;};
int fadeAudio=-1; void mDoAud_setFadeOutStart(int n){fadeAudio=n;}
namespace economy {
using remaster::tuning::get; unsigned g_feeScope=0;
bool enabled(){return active&&get().removeCurrency;}
'''
native=(root/'dusklight/src/d/d_menu_save.cpp').read_text()
fixture=fixture.replace('namespace economy {',function(native,'dMenu_save_c::memCardDataSaveWait')+'\nnamespace economy {')
source=''
source+=''.join(function(currency,n) for n in ['read','write','delta','price','donation','fee_pre','fee_post','armor'])
source+='}\nnamespace pacing { using namespace remaster; unsigned g_transitionConstructDepth=0;\n'
source+=''.join(function(pacing,n) for n in ['accelerate_text','save_wait_before','transition_construct_before','transition_construct_after','fade_before','transition_before','audio_fade_after','audio_wave_after'])
main=r'''
}
int main(){
 using namespace remaster;
 // Currency is zero, fees are free, and vanilla quest event branches still execute.
 u16 balance=200; assert(economy::read(nullptr,nullptr,&balance,nullptr)==HOOK_SKIP_ORIGINAL&&balance==0);
 void* moneyArgs[]={&balance}; balance=9999; assert(economy::write(nullptr,moneyArgs,nullptr,nullptr)==HOOK_CONTINUE&&balance==0);
 assert(economy::delta(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 mesg_flow_node_branch node{30}; auto* pnode=&node; void* flow=nullptr; int debug=0; void* priceArgs[]={&flow,&pnode,&flow,&debug};
 u16 answer=1; assert(economy::price(nullptr,priceArgs,&answer,nullptr)==HOOK_SKIP_ORIGINAL&&answer==0);
 node.param=0; economy::price(nullptr,priceArgs,&answer,nullptr); assert(answer==1); // a reward is never blocked by wallet space
 economy::fee_pre(nullptr,nullptr,nullptr,nullptr); economy::read(nullptr,nullptr,&balance,nullptr); assert(balance>=100);
 economy::fee_post(nullptr,nullptr,nullptr,nullptr); economy::read(nullptr,nullptr,&balance,nullptr); assert(balance==0);
 jmessage_tReference reference; auto* pref=&reference; void* donationArgs[]={&pref};
 assert(economy::donation(nullptr,donationArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&reference.obj.input==9999);
 BOOL armor=1; assert(economy::armor(nullptr,nullptr,&armor,nullptr)==HOOK_SKIP_ORIGINAL&&armor==0);
 // Only presentation wait timers are cleared: the original save state and I/O checks must still run.
 dMenu_save_c menu; auto* pmenu=&menu; void* saveArgs[]={&pmenu};
 assert(pacing::save_wait_before(nullptr,saveArgs,nullptr,nullptr)==HOOK_CONTINUE&&menu.mWaitTimer==0);
 syncResult=0; menu.memCardDataSaveWait(); assert(menu.mMenuProc==0&&written==0);
 syncResult=2; menu.memCardDataSaveWait(); assert(menu.mMenuProc==dMenu_save_c::PROC_MEMCARD_DATA_SAVE_WAIT2&&written==0);
 menu.mMenuProc=0; syncResult=1; menu.memCardDataSaveWait(); assert(menu.mMenuProc==dMenu_save_c::PROC_MEMCARD_DATA_SAVE_WAIT2&&written==1);
 tuning::cfg.instantSaves=false; menu.mWaitTimer=20; pacing::save_wait_before(nullptr,saveArgs,nullptr,nullptr); assert(menu.mWaitTimer==20);
 jmessage_tSequenceProcessor seq; mDoCPd_c::hold=true; pacing::accelerate_text(&seq); assert(seq.field_0xb2==1&&seq.ref.timer==0);
 seq.field_0xb2=0; seq.ref.timer=7; tuning::cfg.instantText=false; pacing::accelerate_text(&seq); assert(seq.field_0xb2==0&&seq.ref.timer==7);
 // Scene fades accelerate only in the intended constructor, not arbitrary cutscenes.
 int duration=120; void* fade=nullptr; void* fadeArgs[]={&fade,&duration}; pacing::fade_before(nullptr,fadeArgs,nullptr,nullptr); assert(duration==120);
 pacing::transition_construct_before(nullptr,nullptr,nullptr,nullptr); pacing::fade_before(nullptr,fadeArgs,nullptr,nullptr); assert(duration==30);
 pacing::transition_construct_after(nullptr,nullptr,nullptr,nullptr); duration=90; pacing::fade_before(nullptr,fadeArgs,nullptr,nullptr); assert(duration==90);
 dOvlpFd3_c overlay; auto* poverlay=&overlay; void* overlayArgs[]={&poverlay}; pacing::transition_before(nullptr,overlayArgs,nullptr,nullptr); assert(overlay.mTimer==0&&fadeAudio==0);
 active=false; balance=77; assert(economy::read(nullptr,nullptr,&balance,nullptr)==HOOK_CONTINUE&&balance==77);
 menu.mWaitTimer=20; pacing::save_wait_before(nullptr,saveArgs,nullptr,nullptr); assert(menu.mWaitTimer==20);
 std::cout<<"Production economy/pacing: free quest fees, no currency, scoped fades, save sync and mode isolation passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'; cpp.write_text(fixture+source+main)
    subprocess.run(['c++','-std=c++20','-O0','-UNDEBUG',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
