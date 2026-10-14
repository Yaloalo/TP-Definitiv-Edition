#pragma once
#include "mods/api.h"
class daItem_c;
namespace remaster::healing::snowpeak {
ModResult install();
bool allows_spawn();
bool allows_actor(daItem_c*);
bool allows_grant();
void reset();
}
