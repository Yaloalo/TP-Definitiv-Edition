#include "core/hooks.hpp"
#include "hud/stamina_bar.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "combat/stamina.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter_HIO.h"
#include "d/d_pane_class.h"
#include "services.hpp"

namespace remaster::hud::stamina_bar {
namespace {

// Runs from the 2D draw list every presentation frame (d_meter2.cpp:375), after the native HUD.
using MeterDraw = remaster::hooks::Shared1;

// The meter panes have transparent padding; overlap slightly for the intended visible spacing.
constexpr float kHeartPaneOverlap = 6.0f;
constexpr float kRowPaneOverlap = 6.0f;
// Seconds the full bar stays visible before fading out, and fade duration.
constexpr float kFullHoldSeconds = 1.5f;
constexpr float kFadeSeconds = 0.4f;
// Display smoothing: the model updates at 30 Hz, the HUD draws at presentation rate.
constexpr float kSmoothingPerSecond = 18.0f;

struct PaneState {
    CPaneMgr* pane = nullptr;
    float posX = 0.0f, posY = 0.0f, x = 0.0f, y = 0.0f;
    float width = 0.0f, height = 0.0f, scaleX = 1.0f, scaleY = 1.0f, alpha = 1.0f;
    bool visible = false;
};

PaneState save_pane(CPaneMgr* pane) {
    if (pane == nullptr || pane->getPanePtr() == nullptr) {
        return {};
    }
    return {pane, pane->getPosX(), pane->getPosY(), pane->getTranslateX(), pane->getTranslateY(),
        pane->getSizeX(), pane->getSizeY(), pane->getScaleX(), pane->getScaleY(), pane->getAlphaRate(),
        pane->getPanePtr()->isVisible()};
}

void restore_pane(const PaneState& s) {
    if (s.pane == nullptr || s.pane->getPanePtr() == nullptr) {
        return;
    }
    s.pane->move(s.posX, s.posY);
    s.pane->translate(s.x, s.y);
    s.pane->resize(s.width, s.height);
    s.pane->scale(s.scaleX, s.scaleY);
    s.pane->setAlphaRate(s.alpha);
    if (s.visible) {
        s.pane->getPanePtr()->show();
    } else {
        s.pane->getPanePtr()->hide();
    }
}

struct Bounds {
    float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
    bool valid = false;

    void include(const Bounds& b) {
        if (!b.valid) {
            return;
        }
        if (!valid) {
            *this = b;
            return;
        }
        left = std::min(left, b.left);
        top = std::min(top, b.top);
        right = std::max(right, b.right);
        bottom = std::max(bottom, b.bottom);
    }
};

Bounds screen_bounds(CPaneMgr* pane) {
    if (pane == nullptr || pane->getPanePtr() == nullptr) {
        return {};
    }
    Mtx matrix;
    const Vec a = pane->getGlobalVtx(&matrix, 0, false, 0);
    const Vec b = pane->getGlobalVtx(&matrix, 3, false, 0);
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::max(a.x, b.x), std::max(a.y, b.y), true};
}

Bounds heart_bounds(dMeter2Draw_c* meter) {
    Bounds visible;
    Bounds all;
    for (CPaneMgr* heart : meter->mpLifeParts) {
        if (heart == nullptr || heart->getPanePtr() == nullptr) {
            continue;
        }
        const Bounds b = screen_bounds(heart);
        all.include(b);
        if (heart->getPanePtr()->isVisible()) {
            visible.include(b);
        }
    }
    return visible.valid ? visible : all;
}

Bounds meter_bounds(dMeter2Draw_c* meter) {
    Bounds b;
    b.include(screen_bounds(meter->mpMagicBase));
    b.include(screen_bounds(meter->mpMagicFrameL));
    b.include(screen_bounds(meter->mpMagicMeter));
    b.include(screen_bounds(meter->mpMagicFrameR));
    return b;
}

// Scales the meter like the hearts and places its top-left under the heart rows; `row` 1 leaves
// room for the native lantern/oxygen meter when that is visible.
void position_meter(dMeter2Draw_c* meter, int row) {
    const PaneState parent = save_pane(meter->mpMagicParent);
    const float lifeBaseScale = std::max(g_drawHIO.mLifeParentScale, 0.001f);
    const float relativeScale = g_drawHIO.mMagicMeterScale / lifeBaseScale;
    meter->mpMagicParent->scale(
        meter->mpLifeParent->getScaleX() * relativeScale, meter->mpLifeParent->getScaleY() * relativeScale);
    meter->mpMagicParent->paneTrans(parent.x, parent.y);

    const Bounds hearts = heart_bounds(meter);
    const Bounds frame = screen_bounds(meter->mpMagicFrameL);
    const Bounds bar = meter_bounds(meter);
    if (!hearts.valid || !frame.valid || !bar.valid) {
        return;
    }
    const float lifeScaleY = std::abs(meter->mpLifeParent->getScaleY());
    const float meterScaleY = std::abs(meter->mpMagicParent->getScaleY());
    const float rowStep = bar.bottom - bar.top - kRowPaneOverlap * meterScaleY;
    const float targetTop = hearts.bottom - kHeartPaneOverlap * lifeScaleY + static_cast<float>(row) * rowStep;
    meter->mpMagicParent->paneTrans(parent.x + hearts.left - frame.left, parent.y + targetTop - frame.top);
}

void draw_meter(dMeter2Draw_c* meter, float ratio, bool exhausted, float alpha) {
    const PaneState parent = save_pane(meter->mpMagicParent);
    const PaneState base = save_pane(meter->mpMagicBase);
    const PaneState frameL = save_pane(meter->mpMagicFrameL);
    const PaneState fill = save_pane(meter->mpMagicMeter);
    const PaneState frameR = save_pane(meter->mpMagicFrameR);
    auto* fillPicture = static_cast<J2DPicture*>(meter->mpMagicMeter->getPanePtr());
    const JUtility::TColor oldBlack = fillPicture->getBlack();
    const JUtility::TColor oldWhite = fillPicture->getWhite();

    // Native lantern/oxygen meter currently shown in this slot: move ours one row down.
    const int row = parent.visible ? 1 : 0;

    for (CPaneMgr* pane : {meter->mpMagicParent, meter->mpMagicBase, meter->mpMagicFrameL, meter->mpMagicMeter,
             meter->mpMagicFrameR}) {
        pane->getPanePtr()->show();
        pane->setAlphaRate(1.0f);
    }
    meter->mpMagicParent->setAlphaRate(alpha);

    if (exhausted) {
        meter->mpMagicMeter->setBlackWhite(JUtility::TColor(255, 190, 80, 255), JUtility::TColor(230, 105, 0, 255));
    } else {
        meter->mpMagicMeter->setBlackWhite(JUtility::TColor(100, 255, 100, 255), JUtility::TColor(0, 210, 0, 255));
    }

    const float frameSpan = meter->mpMagicFrameR->getInitPosX() - meter->mpMagicFrameL->getInitPosX();
    meter->mpMagicMeter->resize(meter->mpMagicMeter->getInitSizeX() * std::clamp(ratio, 0.0f, 1.0f),
        meter->mpMagicMeter->getInitSizeY());
    meter->mpMagicFrameR->move(frameSpan + meter->mpMagicFrameL->getInitPosX(), meter->mpMagicFrameL->getInitPosY());
    meter->mpMagicBase->resize(meter->mpMagicBase->getInitSizeX(), meter->mpMagicBase->getInitSizeY());

    position_meter(meter, row);
    meter->mpKanteraScreen->draw(0.0f, 0.0f, dComIfGp_getCurrentGrafPort());

    fillPicture->setBlackWhite(oldBlack, oldWhite);
    restore_pane(frameR);
    restore_pane(fill);
    restore_pane(frameL);
    restore_pane(base);
    restore_pane(parent);
}

// --- Presentation state (display smoothing + fade-when-full) ---
using Clock = std::chrono::steady_clock;
Clock::time_point g_lastDraw{};
float g_displayRatio = 1.0f;
float g_fullSeconds = 0.0f;
float g_alpha = 0.0f;

float frame_seconds() {
    const Clock::time_point now = Clock::now();
    float dt = g_lastDraw == Clock::time_point{} ? 0.0f : std::chrono::duration<float>(now - g_lastDraw).count();
    g_lastDraw = now;
    return std::clamp(dt, 0.0f, 0.25f);
}

void on_meter_draw_post(ModContext*, void* args, void*, void*) {
    if (!is_active() || !tuning::get().staminaEnabled || !player::gameplay_running()) {
        g_lastDraw = {};
        return;
    }
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr || meter->mpKanteraScreen == nullptr || meter->mpLifeParent == nullptr ||
        meter->mpMagicParent == nullptr || meter->mpMagicBase == nullptr || meter->mpMagicFrameL == nullptr ||
        meter->mpMagicMeter == nullptr || meter->mpMagicFrameR == nullptr) {
        return;
    }

    const float dt = frame_seconds();
    const combat::stamina::State st = combat::stamina::state();
    const float target = st.maximum > 0.0f ? st.current / st.maximum : 1.0f;
    g_displayRatio += (target - g_displayRatio) * std::min(1.0f, dt * kSmoothingPerSecond);

    const bool full = target >= 1.0f && !st.exhausted;
    g_fullSeconds = full ? g_fullSeconds + dt : 0.0f;
    const float targetAlpha = g_fullSeconds > kFullHoldSeconds ? 0.0f : 1.0f;
    const float step = kFadeSeconds > 0.0f ? dt / kFadeSeconds : 1.0f;
    g_alpha = targetAlpha > g_alpha ? std::min(targetAlpha, g_alpha + step) : std::max(targetAlpha, g_alpha - step);
    if (g_alpha <= 0.0f) {
        return;
    }
    draw_meter(meter, g_displayRatio, st.exhausted, g_alpha);
}

}  // namespace

ModResult install() {
    return mods::hook::add_post<MeterDraw>(on_meter_draw_post);
}

}  // namespace remaster::hud::stamina_bar
