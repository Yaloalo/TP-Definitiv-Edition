"""Exercise production shield callbacks and stamina gating without game assets."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/combat/manual_shield.cpp").read_text()
source = re.sub(r'^#include.*\n', '', source, flags=re.M)
shared_aliases = set(re.findall(r'remaster::hooks::(\w+)', source))
shared_fixture = 'namespace remaster::hooks { ' + ''.join(f'struct {name}{{}};' for name in shared_aliases) + ' }\n'
stamina = (root / "src/combat/stamina.cpp").read_text()
begin = stamina.index('HookAction on_guard_attack_pre(')
end = stamina.index('// --- Charging ---', begin)
gate = stamina[begin:end]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
using u8=uint8_t; using BOOL=int;
constexpr int FALSE=0, BUTTON_STATUS_SHIELD_ATTACK=1, PAD_1=0, dItemNo_IRONBALL_e=2;
struct ModContext{};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
enum ModResult{MOD_OK};
#define DEFINE_HOOK(target,name) struct name{}
bool active=true, running=true, shieldOwned=true, learned=true, rHeld=false, staminaReady=true;
namespace remaster {
bool is_active(){return active;}
namespace player {bool gameplay_running(){return running;}}
namespace tuning {struct Values{bool manualShield=true;} values; const Values& get(){return values;}}
}
struct daPy_py_c {
 static constexpr int FLG3_TRANING_SHIELD_ATTACK=3,FLG2_UNK_8000000=4;
 static bool checkShieldGet(){return shieldOwned;}
};
struct dSv_event_flag_c {static constexpr int F_0338=1;};
bool dComIfGs_isEventBit(int){return learned;}
struct mDoCPd_c {static int getHoldLockR(int){return rHeld;}};
struct daAlink_c {
 static constexpr int PROC_GUARD_SLIP=1,BTN_B=2;
 int mProcID=0,mEquipItem=0,bStatus=0,guardFlag=0,charges=0;
 bool wolf=false,bTrigger=false,recovering=false,context=true;
 struct Ground {bool ChkGroundHit(){return true;}} mLinkAcch;
 bool checkWolf(){return wolf;}
 bool checkNoResetFlg3(int){return false;}
 bool checkGuardActionChange(){return context;}
 bool checkUpperReadyThrowAnime(){return false;}
 bool checkModeFlg(int){return false;}
 static bool checkNotBattleStage(){return false;}
 bool checkMagneBootsOn(){return false;}
 bool checkSmallUpperGuardAnime(){return recovering;}
 bool checkGuardAccept(){return context;}
 bool checkGrabAnime(){return false;}
 bool checkDkCaught2Anime(){return false;}
 bool checkKandelaarSwingAnime(){return false;}
 bool checkCutDashAnime(){return false;}
 bool checkCutDashChargeAnime(){return false;}
 bool checkEquipAnime(){return false;}
 bool checkUpperGuardAnime(){return false;}
 bool checkRideOn(){return false;}
 void onNoResetFlg2(int){guardFlag=1;}
 void offNoResetFlg2(int){guardFlag=0;}
 void setBStatus(int s){bStatus=s;}
 bool itemTriggerCheck(int){return bTrigger;}
 int procGuardAttackInit();
};
namespace mods {
 template<class T>T arg(void* args,int){return *static_cast<T*>(args);}
 namespace hook {
 template<class T,class F>ModResult add_pre(F){return MOD_OK;}
 template<class T,class F>ModResult add_post(F){return MOD_OK;}
 }
}
'''
checks = r'''
namespace remaster::combat::stamina {
bool can_act(){return staminaReady;}
GATE
}
int daAlink_c::procGuardAttackInit(){
 void* args=this; int result=0;
 if(remaster::combat::stamina::on_guard_attack_pre(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL) return result;
 ++charges; return 1;
}
int main(){
 using namespace remaster::combat::manual_shield;
 daAlink_c link; daAlink_c* args=&link; BOOL result=1;
 // R alone shields; no targeting/L input is needed.
 rHeld=true; after_shield_guard(nullptr,&args,nullptr,nullptr); assert(link.guardFlag==1);
 assert(before_sword_trigger(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL && !result);
 link.bTrigger=true; result=0;
 assert(before_item_action(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL && result && link.charges==1);
 assert(!allow_guard_attack(&link)); // automatic bashes stay disabled
 staminaReady=false; result=1;
 before_item_action(nullptr,&args,&result,nullptr); assert(!result && link.charges==1);
 staminaReady=true; rHeld=false;
 after_shield_guard(nullptr,&args,nullptr,nullptr); assert(!link.guardFlag);
 assert(before_sword_trigger(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
 learned=false; rHeld=true;
 assert(before_item_action(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
 learned=true; shieldOwned=false;
 after_shield_guard(nullptr,&args,nullptr,nullptr); assert(!link.guardFlag);
 shieldOwned=true; link.wolf=true;
 assert(before_item_action(nullptr,&args,&result,nullptr)==HOOK_CONTINUE && allow_guard_attack(&link));
 link.wolf=false; active=false; link.guardFlag=7;
 after_shield_guard(nullptr,&args,nullptr,nullptr); assert(link.guardFlag==7);
 assert(before_sword_trigger(nullptr,&args,&result,nullptr)==HOOK_CONTINUE && allow_guard_attack(&link));
 active=true; running=false;
 assert(before_item_action(nullptr,&args,&result,nullptr)==HOOK_CONTINUE);
 std::cout<<"Manual shield: R-only guard, learned R+B bash, stamina, wolf and inactive mode checks passed\n";
}
'''.replace('GATE', gate)
with tempfile.TemporaryDirectory() as directory:
    cpp = Path(directory) / "shield.cpp"
    binary = Path(directory) / "shield"
    cpp.write_text(fixture + shared_fixture + source + checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
