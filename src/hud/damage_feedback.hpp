#pragma once
#include "mods/api.h"
class daAlink_c;
namespace remaster::hud::damage_feedback {
ModResult install();
void tick(daAlink_c*);
void reset();
}
