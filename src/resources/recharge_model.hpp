#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
namespace remaster::resources {
// Simulation time only. Menu time, turbo presentation frames and offline time earn no charges.
inline int ticks_for_ms(int64_t ms) { return int((std::clamp<int64_t>(ms, 0, 6000000) * 30 + 999) / 1000); }
struct Recharge {
    int quietTicks = 0;
    int progressTicks = 0;
    void hit() { quietTicks = progressTicks = 0; }
    int tick(int current, int maximum, int delayTicks, int intervalTicks, int amount = 1) {
        if (maximum <= 0 || current >= maximum) { progressTicks = 0; quietTicks = std::min(quietTicks + 1, std::max(0, delayTicks)); return 0; }
        // First refill arrives after delay + interval; time at full capacity cannot be banked.
        if (quietTicks < delayTicks) { ++quietTicks; progressTicks = 0; return 0; }
        if (++progressTicks < std::max(1, intervalTicks)) return 0;
        progressTicks = 0;
        return std::min(std::max(1, amount), maximum - std::max(0, current));
    }
};
inline bool ammo_pickup(int item) {
    // Native replenish packs only: equipment, bomb-bag/type unlocks and quest items survive.
    return (item >= 0x0a && item <= 0x12) || (item >= 0x16 && item <= 0x1d);
}
inline bool bomb_type(int item) { return item >= 0x70 && item <= 0x72; }
}
