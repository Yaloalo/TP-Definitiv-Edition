// Owned pacing controls, adapted from Dusklight v2.0.3's native message,
// save, scene-transition and item functions. No host config is changed.
#include "pacing/quality.hpp"
#include <algorithm>
#include "core/tuning.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_msg_class.h"
#include "d/d_msg_object.h"
#include "d/d_menu_save.h"
#include "d/d_file_select.h"
#include "m_Do/m_Do_MemCard.h"
#include "d/d_ovlp_fade3.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "Z2AudioLib/Z2SceneMgr.h"
#include "services.hpp"

namespace remaster::pacing::quality {
namespace {
DEFINE_HOOK(&jmessage_tSequenceProcessor::do_begin, MessageBegin);
DEFINE_HOOK(&jmessage_tSequenceProcessor::do_tag, MessageTag);
DEFINE_HOOK(&jmessage_tSequenceProcessor::do_isReady, MessageReady);
DEFINE_HOOK(&dMenu_save_c::memCardDataSaveWait, SaveWait);
DEFINE_HOOK(&dMenu_save_c::memCardDataSaveWait2, SaveWait2);
DEFINE_HOOK(&dMenu_save_c::memCardFormat, FormatWait);
DEFINE_HOOK(&dMenu_save_c::memCardFormatWait, FormatWait2);
DEFINE_HOOK(&dMenu_save_c::memCardMakeGameFile, CreateFileWait);
DEFINE_HOOK(&dMenu_save_c::memCardMakeGameFileWait, CreateFileWait2);
DEFINE_HOOK(&dFile_select_c::DataEraseWait, FileDataEraseWait);
DEFINE_HOOK(&dFile_select_c::DataEraseWait2, FileDataEraseWait2);
DEFINE_HOOK(&dFile_select_c::DataCopyWait, FileDataCopyWait);
DEFINE_HOOK(&dFile_select_c::DataCopyWait2, FileDataCopyWait2);
DEFINE_HOOK(&dFile_select_c::MemCardFormat, FileMemCardFormat);
DEFINE_HOOK(&dFile_select_c::MemCardFormatWait, FileMemCardFormatWait);
DEFINE_HOOK(&dFile_select_c::MemCardMakeGameFile, FileMemCardMakeGameFile);
DEFINE_HOOK(&dFile_select_c::MemCardMakeGameFileWait, FileMemCardMakeGameFileWait);
DEFINE_HOOK(&dFile_select_c::MemCardStatCheck, FileStatCheck);
DEFINE_HOOK(&dComIfGs_isItemFirstBit, ItemFirstBit);
DEFINE_HOOK(&daAlink_c::checkTreasureRupeeReturn, RupeeReturn);
DEFINE_HOOK_SYMBOL("dOvlpFd3_c::dOvlpFd3_c", void(dOvlpFd3_c*), TransitionConstruct);
DEFINE_HOOK(&dOvlpFd3_c::execFadeOut, TransitionFadeOut);
DEFINE_HOOK(&JFWDisplay::startFadeOut, FadeOut);
DEFINE_HOOK(&Z2SceneMgr::setFadeOutStart, AudioFadeOut);
DEFINE_HOOK(&Z2SceneMgr::load1stDynamicWave, AudioWaveWait);

DEFINE_HOOK(&dMsgObject_c::isSend, MessageSend);
u8 owned_message_send(dMsgObject_c* actor) {
    dComIfGp_setDoStatusForce(0, 0);
    u8 mesgCancelButton = dComIfGp_checkMesgCancelButton();
    if (mesgCancelButton) {
        dComIfGp_setMesgCancelButton(0);
    }
    if (actor->mShopWaitTimer != 0) {
        return 0;
    }
    jmessage_tReference* pRef = (jmessage_tReference*)actor->mpRenProc->getReference();
    if (pRef->getSendFlag() != 0) {
        if (pRef->getSendFlag() == 3) {
            if (dMsgObject_isMsgSend()) {
                dMsgObject_offMsgSendControl();
                dMsgObject_offMsgSend();
                return 1;
            }
            dMsgObject_onMsgSendControl();
            return 0;
        }
        if (pRef->getSendFlag() == 4) {
            if (actor->field_0x17e >= pRef->getDemoFrame()) {
                return 1;
            }
            return 0;
        }
        if (pRef->getSendFlag() == 5) {
            if (actor->getStatusLocal() == 21) {
                actor->setButtonStatusLocal();
                if (IF_DUSK((tuning::get().instantText && mDoCPd_c::getHoldB(0)) ||)
                    mDoCPd_c::getTrigA(0) != 0 || mDoCPd_c::getTrigB(0) != 0) {
                    return 2;
                }
                return 0;
            } else {
                return 1;
            }
        }
        if (pRef->getSendTimer() > 0) {
            pRef->decSendTimer();
        } else {
            if (pRef->getSendFlag() == 6) {
                pRef->setSendFlag(0);
            } else {
                return 1;
            }
        }
        if (pRef->getSendFlag() == 2) {
            actor->setButtonStatusLocal();
            if (IF_DUSK((tuning::get().instantText && mDoCPd_c::getHoldB(0)) ||)
                mDoCPd_c::getTrigA(0) != 0 || mDoCPd_c::getTrigB(0) != 0) {
                return 2;
            }
        }
    } else {
        actor->setButtonStatusLocal();
        if (actor->isHowlMessage()) {
            if (dMsgObject_isMsgSend()) {
                dMsgObject_offMsgSendControl();
                dMsgObject_offMsgSend();
                return 2;
            }
        } else {
            if (IF_DUSK((tuning::get().instantText && mDoCPd_c::getHoldB(0) && !actor->isShopItemMessage()) ||)
                mDoCPd_c::getTrigA(0) != 0 || mDoCPd_c::getTrigB(0) != 0) {
                return 2;
            }
            if (mesgCancelButton) {
                return 1;
            }
        }
    }
    if (actor->field_0x199 || actor->field_0x1a3) {
        return 2;
    } else {
        return 0;
    }
}
HookAction send_before(ModContext*,void* args,void* result,void*) {
 if(!is_active()) return HOOK_CONTINUE;
 auto* actor=mods::arg<dMsgObject_c*>(args,0);
 if(!actor || !actor->mpRenProc) return HOOK_CONTINUE;
 *static_cast<u8*>(result)=owned_message_send(actor);
 return HOOK_SKIP_ORIGINAL;
}

thread_local unsigned g_transitionConstructDepth = 0;

void accelerate_text(jmessage_tSequenceProcessor* sequence) {
    if (!is_active() || !tuning::get().instantText || !mDoCPd_c::getHoldB(PAD_1) || !sequence) return;
    sequence->field_0xb2 = 1;
    auto* reference = const_cast<jmessage_tReference*>(static_cast<const jmessage_tReference*>(sequence->getReference()));
    if (reference) reference->setSendTimer(0);
}
HookAction text_before(ModContext*, void* args, void*, void*) {
    accelerate_text(mods::arg<jmessage_tSequenceProcessor*>(args, 0));
    return HOOK_CONTINUE;
}
void text_after(ModContext*, void* args, void*, void*) {
    accelerate_text(mods::arg<jmessage_tSequenceProcessor*>(args, 0));
}
HookAction save_wait_before(ModContext*, void* args, void*, void*) {
    if (is_active() && tuning::get().instantSaves)
        mods::arg<dMenu_save_c*>(args, 0)->mWaitTimer = 0;
    // Real SaveSync/FormatSync and error handling still execute.
    return HOOK_CONTINUE;
}
HookAction file_wait_before(ModContext*, void* args, void*, void*) {
    if (is_active() && tuning::get().instantSaves) mods::arg<dFile_select_c*>(args, 0)->mWaitTimer = 0;
    return HOOK_CONTINUE;
}
HookAction file_stat_before(ModContext*, void* args, void*, void*) {
    if (!is_active() || !tuning::get().instantSaves) return HOOK_CONTINUE;
    auto* files = mods::arg<dFile_select_c*>(args, 0);
    const u32 status = mDoMemCd_getStatus(0);
    if (status != 1 && status != 4) return HOOK_CONTINUE;
    files->field_0x03b1 = 1;
    if (status == 1) { files->setInitSaveData(); files->dataSave(); }
    files->mCardCheckProc = dFile_select_c::MEMCARDCHECKPROC_AUTO_MAKE_GAMEFILE;
    return HOOK_SKIP_ORIGINAL;
}
HookAction item_first_before(ModContext*, void* args, void* result, void*) {
    if (!is_active() || !tuning::get().skipRupeeCutscenes) return HOOK_CONTINUE;
    const auto item = mods::arg<u8>(args, 0);
    if (item >= dItemNo_GREEN_RUPEE_e && item <= dItemNo_SILVER_RUPEE_e) {
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
HookAction rupee_return_before(ModContext*, void*, void* result, void*) {
    if (!is_active() || !tuning::get().noRupeeReturns) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
HookAction transition_construct_before(ModContext*, void*, void*, void*) {
    ++g_transitionConstructDepth;
    return HOOK_CONTINUE;
}
void transition_construct_after(ModContext*, void*, void*, void*) {
    if (g_transitionConstructDepth) --g_transitionConstructDepth;
}
HookAction fade_before(ModContext*, void* args, void*, void*) {
    if (is_active() && tuning::get().fastTransitions && g_transitionConstructDepth) {
        auto& duration = mods::arg_ref<int>(args, 1);
        duration = std::max(0, duration - 90);
    }
    return HOOK_CONTINUE;
}
HookAction transition_before(ModContext*, void* args, void*, void*) {
    if (is_active() && tuning::get().fastTransitions) {
        auto* fade = mods::arg<dOvlpFd3_c*>(args, 0);
        if (fade->mTimer == -1) {
            fade->mTimer = 0;
            mDoAud_setFadeOutStart(0);
        }
    }
    return HOOK_CONTINUE;
}
void audio_fade_after(ModContext*, void* args, void*, void*) {
    if (is_active() && tuning::get().fastTransitions) mods::arg<Z2SceneMgr*>(args, 0)->load1stWait = 1;
}
void audio_wave_after(ModContext*, void* args, void*, void*) {
    if (is_active() && tuning::get().fastTransitions) {
        auto* scene = mods::arg<Z2SceneMgr*>(args, 0);
        if (scene->load1stWait == -15) scene->load1stWait = -1;
    }
}
}

ModResult install() {
    ModResult r = mods::hook::add_pre<MessageSend>(send_before);
    if (r == MOD_OK) r = mods::hook::add_post<MessageBegin>(text_after);
    if (r == MOD_OK) r = mods::hook::add_post<MessageTag>(text_after);
    if (r == MOD_OK) r = mods::hook::add_pre<MessageReady>(text_before);
    if (r == MOD_OK) r = mods::hook::add_pre<SaveWait>(save_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<SaveWait2>(save_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FormatWait>(save_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FormatWait2>(save_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<CreateFileWait>(save_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<CreateFileWait2>(save_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileDataEraseWait>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileDataEraseWait2>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileDataCopyWait>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileDataCopyWait2>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileMemCardFormat>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileMemCardFormatWait>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileMemCardMakeGameFile>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileMemCardMakeGameFileWait>(file_wait_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FileStatCheck>(file_stat_before);
    if (r == MOD_OK) r = mods::hook::add_pre<ItemFirstBit>(item_first_before);
    if (r == MOD_OK) r = mods::hook::add_pre<RupeeReturn>(rupee_return_before);
    if (r == MOD_OK) r = mods::hook::add_pre<TransitionConstruct>(transition_construct_before);
    if (r == MOD_OK) r = mods::hook::add_post<TransitionConstruct>(transition_construct_after);
    if (r == MOD_OK) r = mods::hook::add_pre<FadeOut>(fade_before);
    if (r == MOD_OK) r = mods::hook::add_pre<TransitionFadeOut>(transition_before);
    if (r == MOD_OK) r = mods::hook::add_post<AudioFadeOut>(audio_fade_after);
    if (r == MOD_OK) r = mods::hook::add_post<AudioWaveWait>(audio_wave_after);
    return r;
}
}
