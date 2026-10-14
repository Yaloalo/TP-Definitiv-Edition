#pragma once
#include "mods/api.h"
class daAlink_c;
namespace remaster::combat::ward {
ModResult install();
bool enabled();
float absorb(daAlink_c*);
void damage_received();
void tick(daAlink_c*);
void refill();
void boss_victory(const char* stage);
void death();
void reset();
}
