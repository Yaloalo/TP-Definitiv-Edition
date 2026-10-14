// Mouse capture and delta conversion adapted from Dusklight v2.0.3
// src/dusk/mouse.cpp and d_camera.cpp. SDL is the host's input backend;
// feature switches and sensitivities are owned by Definitive Edition.
#include "input/mouse_controls.hpp"
#include <algorithm>
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_camera.h"
#include "services.hpp"

namespace remaster::input::mouse_controls {
namespace {
struct SDL_Window;
struct Backend {
    SDL_Window* (*window)() = nullptr;
    uint64_t (*flags)(SDL_Window*) = nullptr;
    bool (*relative)(SDL_Window*) = nullptr;
    bool (*setRelative)(SDL_Window*, bool) = nullptr;
    bool (*grab)(SDL_Window*, bool) = nullptr;
    uint32_t (*read)(float*, float*) = nullptr;
    bool (*show)() = nullptr;
    bool (*hide)() = nullptr;
} g_backend;

DEFINE_HOOK_SYMBOL("dusk::mouse::read", void(), MouseRead);
DEFINE_HOOK(&dCamera_c::freeCamera, FreeCamera);
DEFINE_HOOK(&daAlink_c::setBodyAngleToCamera, AimAngles);
DEFINE_HOOK(&daAlink_c::setBowSight, BowSight);

constexpr uint64_t kInputFocus = 0x200; // SDL_WINDOW_INPUT_FOCUS, SDL3 ABI
constexpr float kPixelRadians = 0.0025f;
constexpr float kRadiansDegrees = 57.295779513f;
SDL_Window* g_ownedWindow = nullptr;
float g_dx = 0, g_dy = 0;
bool g_aimUsed = false, g_cameraUsed = false;

bool input_allowed() {
    bool visible = true;
    return is_active() && player::gameplay_running() &&
        svc_ui->is_any_document_visible(mod_ctx, &visible) == MOD_OK && !visible;
}

HookAction read_before(ModContext*, void*, void*, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    g_dx = g_dy = 0;
    g_aimUsed = g_cameraUsed = false;
    auto* window = g_backend.window();
    const bool want = window && input_allowed() && (g_backend.flags(window) & kInputFocus) &&
        (tuning::get().mouseCamera || (tuning::get().mouseAim && dCamera_c::isAimActive()));
    if (!want) {
        reset();
        return HOOK_SKIP_ORIGINAL;
    }
    const bool wasCaptured = g_backend.relative(window);
    if (!wasCaptured) {
        g_backend.grab(window, true);
        g_backend.setRelative(window, true);
    }
    if (!g_backend.relative(window)) return HOOK_SKIP_ORIGINAL;
    g_ownedWindow = window;
    g_backend.hide();
    g_backend.read(&g_dx, &g_dy);
    if (!wasCaptured) g_dx = g_dy = 0; // discard mode-switch motion
    return HOOK_SKIP_ORIGINAL;
}

HookAction camera_before(ModContext*, void* args, void* result, void*) {
    if (!is_active()) return HOOK_CONTINUE;
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    if (!camera || !input_allowed() || !tuning::get().mouseCamera || camera->mCamStyle == 70) {
        if (camera) camera->mCamParam.mManualMode = 0;
        *static_cast<bool*>(result) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    if (camera->mGear == 1) camera->mGear = 0;
    auto& params = camera->mCamParam;
    if (!params.mManualMode) {
        params.freeXAngle = camera->mViewCache.mDirection.mAzimuth.Degree();
        params.freeYAngle = camera->mViewCache.mDirection.mInclination.Degree();
    }
    const float stickX = camera->mPadInfo.mCStick.mLastPosX;
    const float stickY = camera->mPadInfo.mCStick.mLastPosY;
    if (stickX != 0 || stickY < 0 || (params.mManualMode && stickY != 0)) {
        params.mManualMode = 1;
        params.freeXAngle += stickX * 5.0f;
        params.freeYAngle -= stickY * 5.0f;
    }
    if (!g_cameraUsed && (g_dx != 0 || g_dy != 0) &&
        !dCamera_c::isAimActive() &&
        !dComIfGp_checkCameraAttentionStatus(dComIfGp_getPlayerCameraID(0), 0x8)) {
        params.mManualMode = 1;
        const float factor = kPixelRadians * kRadiansDegrees *
            static_cast<float>(tuning::get().mouseCameraSensitivityPercent) / 100.0f;
        params.freeXAngle -= g_dx * factor * (tuning::get().compassMirrorMode ? -1.0f : 1.0f);
        params.freeYAngle += g_dy * factor * (tuning::get().invertMouseY ? -1.0f : 1.0f);
        g_cameraUsed = true;
    }
    if (!params.mManualMode || !dComIfGp_getPlayer(0)) {
        *static_cast<bool*>(result) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    params.freeYAngle = std::clamp(params.freeYAngle, -30.0f, 50.0f);
    camera->mViewCache.mDirection.mAzimuth = cSAngle(params.freeXAngle);
    camera->mViewCache.mDirection.mInclination = cSAngle(params.freeYAngle);
    camera->mViewCache.mEye = camera->mViewCache.mCenter + camera->mViewCache.mDirection.Xyz();
    *static_cast<bool*>(result) = true;
    return HOOK_SKIP_ORIGINAL;
}

void aim_after(ModContext*, void* args, void* result, void*) {
    if (!input_allowed() || !tuning::get().mouseAim || g_aimUsed || !result || !*static_cast<BOOL*>(result)) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !link->mpHIO || !link->checkAimInputContext()) return;
    float factor = kPixelRadians * static_cast<float>(tuning::get().mouseAimSensitivityPercent) / 100.0f;
    if (link->checkWolfEyeUp()) factor *= 0.6f;
    if (dComIfGp_checkPlayerStatus0(0, 0x200000)) {
        const float zoom = dComIfGp_getCameraZoomScale(link->field_0x317c);
        if (zoom > 0) factor /= zoom;
    }
    const float yaw = -g_dx * factor * (tuning::get().compassMirrorMode ? -1.0f : 1.0f);
    const float pitch = g_dy * factor * (tuning::get().invertMouseY ? -1.0f : 1.0f);
    link->shape_angle.y += cM_rad2s(yaw);
    const int lower = !dComIfGp_checkPlayerStatus0(0, 0x202000) && link->mEquipItem == dItemNo_IRONBALL_e
        ? -10000 : link->mpHIO->mItem.m.mItemFPUpMaxUnk;
    int angle = link->mBodyAngle.x + cM_rad2s(pitch);
    if (link->checkNotItemSinkLimit() && angle > 0 && angle > link->mBodyAngle.x) angle = link->mBodyAngle.x;
    link->mBodyAngle.x = static_cast<s16>(std::clamp(angle, lower, int(link->mpHIO->mItem.m.mItemFPMaxUnk)));
    link->checkBodyAngleX(link->mBodyAngle.x);
    link->field_0x310a = link->mBodyAngle.x;
    link->field_0x310c = link->shape_angle.y;
    g_aimUsed = true;
}

void sight_after(ModContext*, void* args, void*, void*) {
    if (!is_active()) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link && tuning::get().aimingReticle && link->checkBowChargeWaitAnime() &&
        !dComIfGp_checkPlayerStatus0(0, 0x200000)) link->mSight.onDrawFlg();
    else if (link) link->mSight.offDrawFlg();
}

template<class Fn>
ModResult resolve(const char* name, Fn& fn) {
    void* address = nullptr;
    const ModResult r = svc_hook->resolve(mod_ctx, name, &address, nullptr);
    if (r == MOD_OK && address) fn = reinterpret_cast<Fn>(address);
    return address ? r : MOD_UNAVAILABLE;
}
}

void reset() {
    if (g_ownedWindow && g_backend.setRelative) {
        g_backend.setRelative(g_ownedWindow, false);
        g_backend.grab(g_ownedWindow, false);
        g_backend.show();
    }
    g_ownedWindow = nullptr;
    g_dx = g_dy = 0;
    g_aimUsed = g_cameraUsed = false;
}

ModResult install() {
    ModResult r = resolve("aurora::window::get_sdl_window", g_backend.window);
    if (r == MOD_OK) r = resolve("SDL_GetWindowFlags", g_backend.flags);
    if (r == MOD_OK) r = resolve("SDL_GetWindowRelativeMouseMode", g_backend.relative);
    if (r == MOD_OK) r = resolve("SDL_SetWindowRelativeMouseMode", g_backend.setRelative);
    if (r == MOD_OK) r = resolve("SDL_SetWindowMouseGrab", g_backend.grab);
    if (r == MOD_OK) r = resolve("SDL_GetRelativeMouseState", g_backend.read);
    if (r == MOD_OK) r = resolve("SDL_ShowCursor", g_backend.show);
    if (r == MOD_OK) r = resolve("SDL_HideCursor", g_backend.hide);
    if (r == MOD_OK) r = mods::hook::add_pre<MouseRead>(read_before);
    if (r == MOD_OK) r = mods::hook::add_pre<FreeCamera>(camera_before);
    if (r == MOD_OK) r = mods::hook::add_post<AimAngles>(aim_after);
    if (r == MOD_OK) r = mods::hook::add_post<BowSight>(sight_after);
    return r;
}
}
