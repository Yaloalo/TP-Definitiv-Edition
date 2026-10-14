#pragma once
#include "mods/api.h"
class daAlink_c;
namespace remaster::combat::ward {
ModResult install();
bool enabled();
// A scoped presentation value; it never changes either wallet.
bool visual_rupees(uint16_t&);
float absorb(daAlink_c*);
void damage_received();
void tick(daAlink_c*);
void refill();
void boss_victory(const char* stage);
void death();
void reset();
}
