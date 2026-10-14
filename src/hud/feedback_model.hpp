#pragma once
#include <algorithm>
#include <cmath>
namespace remaster::hud {
// Timing envelope adapted from Dawnlight's hud_fade_state.hpp. Activity reveals
// immediately; long presentation gaps restart visible rather than fading unseen.
struct IdleEnvelope {
    double last=-1, since=-1;
    float level=1;
    float update(double now,bool enabled,bool idle,double delay,double duration,float minimum) {
        const double dt=last<0?0:now-last;
        const double previous=last; last=now;
        if (!std::isfinite(now) || !enabled || !idle || previous<0 || dt<0 || dt>0.25) {
            since=now; level=1; return 1;
        }
        if (since<0) since=now;
        const double fadeStart=since+std::max(0.0,delay);
        if (now<=fadeStart) level=1;
        else {
            const double elapsed=now-std::max(previous,fadeStart);
            level=std::max(0.0f,level-float(elapsed/std::max(0.001,duration)));
        }
        const float smooth=level*level*(3-2*level);
        minimum=std::clamp(minimum,0.0f,1.0f);
        return minimum+(1-minimum)*smooth;
    }
};
struct DamagePulse {
    double start=-1;
    void hit(double now) { start=now; }
    float opacity(double now,double duration,float intensity) const {
        if (start<0 || !std::isfinite(now) || now<start) return 0;
        const double t=(now-start)/std::max(0.001,duration);
        if (t>=1) return 0;
        return std::clamp(intensity,0.0f,0.6f)*float((1-t)*(1-t));
    }
};
}
