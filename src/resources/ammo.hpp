#pragma once
#include "mods/api.h"
#include <cstdint>
class daAlink_c;
namespace remaster::resources::ammo {
ModResult install();
void tick(daAlink_c*);
void damage_received();
void death();
void slot_changed(uint32_t slot, bool fresh);
void reset();
void refill();
bool enabled();
bool removes_pickup(int item);
}
