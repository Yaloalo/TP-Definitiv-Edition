// Angle-step math adapted from dusklight-snappy-turns/src/mod.cpp.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
namespace remaster::movement::responsiveness {
inline int angle_distance(int16_t a,int16_t b) { return std::abs(int(int16_t(a-b))); }
inline int16_t extend_turn(int16_t before,int16_t after,int16_t target,float multiplier) {
    const int step=int16_t(after-before), remaining=int16_t(target-after);
    if (!step || multiplier<=1 || !remaining || (step>0)!=(remaining>0)) return after;
    int extra=int(float(step)*(std::clamp(multiplier,1.0f,5.0f)-1));
    if (std::abs(extra)>std::abs(remaining)) extra=remaining;
    return int16_t(after+extra);
}
}
