#pragma once

#include "mods/api.h"
class daAlink_c;

namespace remaster::combat::manual_shield {
ModResult install();
bool allow_guard_attack(daAlink_c* link);
}
