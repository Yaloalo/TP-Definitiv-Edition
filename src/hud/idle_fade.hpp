#pragma once
#include "mods/api.h"
class daAlink_c;
namespace remaster::hud::idle_fade {
ModResult install();
void tick(daAlink_c*);
void reset();
float opacity();
}
