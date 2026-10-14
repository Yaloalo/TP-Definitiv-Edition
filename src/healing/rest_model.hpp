#pragma once
#include <algorithm>
namespace remaster::healing::stones {
// Simulation ticks only. Menus interrupt a rest; damage/threats interrupt recovery.
struct RestTimers {
    int quiet = 0, peace = 0, channel = 0, emergency = 0;
    bool latched = false;
    void damage() { quiet = peace = channel = emergency = 0; latched = false; }
    void advance(bool running, bool threat) {
        if (!running) { channel = emergency = 0; return; }
        quiet = std::min(quiet + 1, 180000);
        if (threat) peace = emergency = 0;
        else peace = std::min(peace + 1, 180000);
    }
    bool rest(bool running, bool safe, bool intent, int duration) {
        if (!intent) { channel = 0; latched = false; return false; }
        if (!running || !safe) { channel = 0; return false; }
        if (latched) return false;
        if (++channel < std::max(1, duration)) return false;
        channel = 0; latched = true; return true;
    }
    bool recover(bool running, bool safeOverworldIdle, bool empty, int delay, int duration) {
        if (!running || !safeOverworldIdle || !empty || peace < delay) { emergency = 0; return false; }
        if (++emergency < std::max(1, duration)) return false;
        emergency = 0; return true;
    }
};
}
