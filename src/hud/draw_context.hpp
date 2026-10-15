#pragma once
#include <algorithm>
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_graphic.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"

namespace remaster::hud {
// Native panes leave their own matrices/scissor active. Custom overlays must
// establish projection BEFORE drawing, then restore the game's current port.
class DrawContext {
    J2DGrafContext* previous_;
    J2DOrthoGraph port_;
public:
    explicit DrawContext(bool menu = false)
        : previous_(dComIfGp_getCurrentGrafPort()),
          port_(0, 0, FB_WIDTH, FB_HEIGHT, -1, 1) {
        if (menu) {
            // Fit a 608 x 448 sheet without stretching on wide/tall displays.
            const float ratio = mDoGph_gInf_c::getWidthF()/mDoGph_gInf_c::getHeightF();
            const float width = std::max(608.0f, 448.0f*ratio);
            const float height = width/ratio;
            port_.setOrtho((608-width)/2, (448-height)/2, width, height, -1, 1);
        } else {
            port_.setOrtho(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(), -1, 1);
        }
        port_.setPort();
    }
    ~DrawContext() { if (previous_) previous_->setPort(); }
    DrawContext(const DrawContext&) = delete;
    DrawContext& operator=(const DrawContext&) = delete;
};
}
