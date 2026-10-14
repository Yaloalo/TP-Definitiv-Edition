#pragma once
#include "mods/api.h"
class daAlink_c;
namespace remaster::healing::wolf {
ModResult install();
void tick(daAlink_c*);
void interrupt(daAlink_c*);
void reset();
bool busy();
}
