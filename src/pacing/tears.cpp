// Ported from Dusklight v2.0.3 src/d/actor/d_a_obj_drop.cpp.
#include "pacing/tears.hpp"
#include "core/tuning.hpp"
#include "d/actor/d_a_obj_drop.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_e_ym.h"
#include "d/d_com_inf_game.h"
#include "d/d_s_play.h"
#include "m_Do/m_Do_mtx.h"
#include "services.hpp"
namespace remaster::pacing::tears {
namespace {
DEFINE_HOOK(&daObjDrop_c::modeParentWait, ParentWait);
DEFINE_HOOK(&daObjDrop_c::modeWait, DropWait);
DEFINE_HOOK(&daE_YM_c::bckSet, InsectAnimation);
static const u16 l_sizuku_app_effect_list[] = {0x8388, 0x8389};
static BOOL checkGetCargoRide() {
 return daPy_getPlayerActorClass()->checkCargoCarry() &&
 strcmp(dComIfGp_getStartStageName(), "F_SP112") == 0 &&
 dComIfGs_isLightDropGetFlag(dComIfGp_getStartStageDarkArea());
}
int owned_modeParentWait(daObjDrop_c* actor) {
    fopAc_ac_c* pparent = fopAcM_SearchByID(actor->parentActorID);
    if (pparent != NULL) {
        if (fopAcM_GetProfName(pparent) == fpcNm_E_YM_e) {
            actor->current.pos = pparent->current.pos;
            return 1;
        }

        if (fopAcM_GetProfName(pparent) == fpcNm_E_YMB_e && !actor->mAppear) {
            return 1;
        }
    }

    if (daPy_getPlayerActorClass()->checkCargoCarry()) {
        actor->mModeAction = 50;
        actor->setMode(daObjDrop_c::MODE_WAIT_e);
        return 1;
    }

    switch (actor->mModeAction) {
    case 0:
        actor->current.angle.set(0, 0, 0);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 2; j++) {
                dComIfGp_particle_set(l_sizuku_app_effect_list[j], &actor->current.pos, &actor->current.angle,
                                      &actor->scale);
            }

            actor->current.angle.y += 0x5555;
        }

        actor->mModeAction = 1;

#if TARGET_PC
        actor->mModeTimer = tuning::get().fastTears ? 0 : 40;
        if (tuning::get().fastTears) {
            actor->current.pos.y += 100.0f;
        } else {
            actor->current.pos.y += 300.0f;
        }
#else
        actor->mModeTimer = 40;
        actor->current.pos.y += 300.0f;
#endif
        actor->mSound.startSound(Z2SE_OBJ_LIGHTDROP_APPEAR, 0, -1);
        break;
    case 1:
        if (cLib_calcTimer<s8>(&actor->mModeTimer) == 0) {
            actor->mModeAction = 2;
        }
        break;
    case 2:
        actor->createBodyEffect();
#if TARGET_PC
        actor->mModeTimer = tuning::get().fastTears ? 0 : 45;
#else
        actor->mModeTimer = 45;
#endif
        actor->mModeAction = 0;
        actor->setMode(daObjDrop_c::MODE_WAIT_e);
        break;
    }

    return 1;
}
HookAction before_modeParentWait(ModContext*, void* args, void* result, void*) {
 if (!is_active()) return HOOK_CONTINUE;
 auto* actor = mods::arg<daObjDrop_c*>(args,0);
 if (!actor || !daPy_getPlayerActorClass()) return HOOK_CONTINUE;
 *static_cast<int*>(result) = owned_modeParentWait(actor);
 return HOOK_SKIP_ORIGINAL;
}
int owned_modeWait(daObjDrop_c* actor) {
    daPy_py_c* pplayer = daPy_getPlayerActorClass();

    cXyz collect_pos(pplayer->current.pos);
    cXyz spF0;
    mDoMtx_stack_c::copy(pplayer->getWolfBackbone2Matrix());
    mDoMtx_stack_c::multVecZero(&collect_pos);

    switch (actor->mModeAction) {
    case 0:
        if (actor->mModeTimer > 0) {
            actor->mModeTimer--;
        } else {
            actor->mTargetPos = actor->home.pos;
            actor->speedF = 0.0f;
            actor->mModeAction = 1;
        }
        break;
    case 1:
    case 2:
    case 50:
        #if TARGET_PC
        if (tuning::get().fastTears) {
            f32 player_dist = actor->current.pos.abs(daPy_getPlayerActorClass()->current.pos);
            f32 home_dist = actor->current.pos.abs(actor->home.pos);

            if (checkGetCargoRide() && player_dist < 1000.0f) {
                actor->mTargetPos = pplayer->current.pos;
                actor->mTargetPos.y += 100.0f;

                f32 temp = 3000.0f - home_dist;
                if (temp < 0.0f) {
                    temp = 0.0f;
                }

                cLib_chaseF(&actor->speedF, (temp / 3000.0f) * 10.0f * (temp / 3000.0f), 1.0f);
            } else {
                actor->mTargetPos = actor->home.pos;
                cLib_chaseF(&actor->speedF, 2.0f, 0.5f);
            }
        } else {
            cLib_chaseF(&actor->speedF, 7.5f, 0.4f);
        }
        #else
        cLib_chaseF(&actor->speedF, 7.5f, 0.4f);
        #endif

        if (actor->mModeAction == 1) {
            cLib_chasePos(&actor->current.pos, actor->mTargetPos, actor->speedF);
        }

        actor->speed.y = -cM_scos((KREG_S(3) + 1000) * actor->mCounter) * 2.5f;
        actor->current.pos.y += actor->speed.y;

        if (actor->mTargetPos.abs(actor->current.pos) < 30.0f) {
            actor->mModeAction = 2;
        }

        if (actor->checkGetArea()) {
            for (int i = 0; i < 3; i++) {
                actor->mLinePos[i] = actor->current.pos;
                actor->mLineMovePos[i] = cXyz::Zero;
                actor->mTargetAngle[i] = csXyz::Zero;
            }

            actor->createLineEffect();
            actor->mModeAction = 3;
        }
        break;
    case 3: {
        actor->removeBodyEffect();

        static const s16 target_rel_angle[] = {-0x4000, 0x0000, 0x4000};
        static const s16 target_angle_x[] = {0x0000, 0x2000, 0x0000};

        for (int i = 0; i < 3; i++) {
            actor->mTargetAngle[i].x = target_angle_x[i];
            actor->mTargetRelAngle[i] = target_rel_angle[i];
        }

        actor->mModeAction = 4;
        actor->mDrawInTimer = 60;
        actor->mSound.startSound(Z2SE_OBJ_LIGHTDROP_DRAW_IN, 0, -1);
    } break;
    case 4: {
        actor->mDrawInTimer--;
        for (int i = 0; i < 3; i++) {
            actor->speedF = 60.0f;
            if (daPy_getPlayerActorClass()->checkCargoCarry()) {
                actor->speedF = 250.0f;
            }

            f32 var_f25 = cM_scos(actor->mTargetAngle[i].x) * actor->speedF;
            f32 var_f24 = cM_ssin(actor->mTargetAngle[i].x) * actor->speedF;

            s16 sp150 = cM_atan2s(collect_pos.x - actor->mLinePos[i].x, collect_pos.z - actor->mLinePos[i].z);
            sp150 += actor->mTargetRelAngle[i];

            cXyz target;
            target.x = var_f25 * cM_ssin(sp150);
            target.y = var_f24;
            target.z = var_f25 * cM_scos(sp150);

            f32 step_scale = KREG_F(10) + 0.5f;
            f32 step_max = KREG_F(11) + 5.0f;
            f32 step_min = KREG_F(12) + 1.0f;

            cLib_addCalc(&actor->mLineMovePos[i].x, target.x, step_scale, step_max, step_min);
            cLib_addCalc(&actor->mLineMovePos[i].y, target.y, step_scale, step_max, step_min);
            cLib_addCalc(&actor->mLineMovePos[i].z, target.z, step_scale, step_max, step_min);
            cLib_addCalcAngleS(&actor->mTargetRelAngle[i], 0, 10, 900, 200);

            cXyz prev_line_pos(actor->mLinePos[i]);
            actor->mLinePos[i].x += actor->mLineMovePos[i].x;
            actor->mLinePos[i].y += actor->mLineMovePos[i].y;
            actor->mLinePos[i].z += actor->mLineMovePos[i].z;

            cM3dGLin line;
            cM3dGSph sph;
            line.SetStartEnd(prev_line_pos, actor->mLinePos[i]);
            sph.Set(collect_pos, KREG_F(17) + 50.0f);

            Vec sp114;
            if (cM3d_Cross_LinSph(&line, &sph, &sp114)) {
                actor->mLinePos[i] = collect_pos;
                actor->mLineIsCross[i] = true;
            }

            cLib_addCalcAngleS(
                &actor->mTargetAngle[i].x,
                cM_atan2s(collect_pos.y - actor->mLinePos[i].y, actor->mLinePos[i].absXZ(collect_pos)), 5, 1500,
                500);
        }

        if (actor->mLineIsCross[0] || actor->mLineIsCross[1] || actor->mLineIsCross[2] || actor->mDrawInTimer == 0) {
            pplayer->onWolfLightDropGet();
            actor->removeLineEffect();
            actor->mDeleteTimer = 70;
            actor->checkCompleteDemo();
            actor->mModeAction = 5;
        }
    } break;
    case 5:
        if (cLib_calcTimer<u8>(&actor->mDeleteTimer) == 0) {
            actor->mModeAction = 6;

            if (actor->chkDemoMode() == daObjDrop_c::DEMOMODE_COMPLETE_WAIT_e) {
                fopAcM_delete(actor);
            }
        }
        break;
    case 6:
        break;
    }

    if (actor->mModeAction == 0 || actor->mModeAction == 1 || actor->mModeAction == 2 || actor->mModeAction == 3) {
        actor->mSound.startLevelSound(Z2SE_OBJ_LIGHTDROP_FLOAT, 0, -1);
    }

    if (actor->mModeAction == 2) {
        if (actor->mTargetPos.absXZ(actor->current.pos) < (KREG_F(4) + 500.0f) && pplayer->getSpeedF() != 0.0f) {
            actor->mCcSph.SetC(actor->current.pos);
            dComIfG_Ccsp()->Set(&actor->mCcSph);
        }

        cXyz* cc_movep = actor->mCcStts.GetCCMoveP();
        if (cc_movep != NULL) {
            cXyz move_vec;
            cXyz sp12C;

            if (!cc_movep->isZero()) {
                actor->current.angle.y = cM_atan2s(cc_movep->x, cc_movep->z);
                actor->speedF = 5.0f;
                sp12C = cXyz::Zero;
            } else {
                f32 var_f26 = KREG_F(2) + 1.0f;
                if (actor->current.pos.absXZ(actor->mTargetPos) < 10.0f) {
                    var_f26 = 0.0f;
                    actor->speedF = 0.0f;
                }

                s16 sp154 = cM_atan2s(actor->mTargetPos.x - actor->current.pos.x, actor->mTargetPos.z - actor->current.pos.z);
                sp12C.set(cM_ssin(sp154) * var_f26, 0.0f, cM_scos(sp154) * var_f26);
            }

            move_vec.set(cM_ssin(actor->current.angle.y) * actor->speedF, 0.0f,
                         cM_scos(actor->current.angle.y) * actor->speedF);
            move_vec += sp12C;
            actor->speedF = std::sqrt(move_vec.x * move_vec.x + move_vec.z * move_vec.z);
            if (actor->speedF > 5.0f) {
                actor->speedF = 5.0f;
            }

            actor->current.pos.x += move_vec.x;
            actor->current.pos.z += move_vec.z;
            *cc_movep = cXyz::Zero;
        }
    }

    return 1;
}
HookAction before_modeWait(ModContext*, void* args, void* result, void*) {
 if (!is_active()) return HOOK_CONTINUE;
 auto* actor = mods::arg<daObjDrop_c*>(args,0);
 if (!actor || !daPy_getPlayerActorClass()) return HOOK_CONTINUE;
 *static_cast<int*>(result) = owned_modeWait(actor);
 return HOOK_SKIP_ORIGINAL;
}
HookAction insect_before(ModContext*, void* args, void*, void*) {
 if (is_active() && tuning::get().fastTears && mods::arg<int>(args,1)==6)
  mods::arg_ref<float>(args,4) = 2.0f;
 return HOOK_CONTINUE;
}
}
ModResult install() {
 auto r=mods::hook::add_pre<ParentWait>(before_modeParentWait);
 if(r==MOD_OK) r=mods::hook::add_pre<DropWait>(before_modeWait);
 if(r==MOD_OK) r=mods::hook::add_pre<InsectAnimation>(insect_before);
 return r;
}
}
