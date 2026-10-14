#pragma once
#include <cstdint>
namespace remaster::combat::techniques {
// Simulation ticks, not rendered frames. Edges never survive focus/menu/device changes.
struct InputState {
    bool held = false, armed = false;
    int pending = 0, counter = 0;
    uint32_t device = 0;
    void sample(bool down, bool allowed, uint32_t connectedDevice) {
        if (!allowed || !connectedDevice || connectedDevice != device) {
            held = down; armed = false; pending = counter = 0;
            device = connectedDevice;
            return;
        }
        if (pending > 0) --pending;
        if (counter > 0) --counter;
        if (!down) armed = true;
        if (down && !held && armed) pending = 6;
        held = down;
    }
    void blocked(int window) { counter = window; }
    void consume() { pending = counter = 0; }
};
}
