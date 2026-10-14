#pragma once
#include <algorithm>
#include <cstdint>
namespace remaster::combat::stance {
struct State {
    float pressure = 0, lastImpact = 0;
    uint64_t lastAttack = 0;
    int quietTicks = 0, recoveryTicks = 0;
    bool contact(uint64_t attack, float impact, float resistance) {
        if (!attack || impact <= 0 || recoveryTicks > 0) return false;
        // A parry can upgrade the same Shield Attack's ordinary bash impact;
        // repeated hurt-sphere contacts cannot multiply it or refresh recovery.
        const float added = attack == lastAttack ? std::max(0.0f, impact - lastImpact) : impact;
        if (added <= 0) return false;
        lastAttack = attack; lastImpact = impact; quietTicks = 0;
        pressure += added;
        if (pressure < std::max(1.0f, resistance)) return false;
        pressure = 0; recoveryTicks = 60;
        return true;
    }
    void tick() {
        if (recoveryTicks > 0) --recoveryTicks;
        quietTicks = std::min(91, quietTicks + 1);
        if (quietTicks > 90) pressure = std::max(0.0f, pressure - 0.5f);
    }
};
}
