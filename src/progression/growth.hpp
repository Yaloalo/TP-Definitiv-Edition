#pragma once
#include "mods/api.h"
namespace remaster::progression::growth {
ModResult install();
bool ensure();
void tick();
void apply_health();
bool allocate(bool stamina);
}
