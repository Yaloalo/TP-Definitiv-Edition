#pragma once
#include "mods/api.h"
class daAlink_c;
namespace remaster::healing::stones {
ModResult install();
void tick(daAlink_c*);
void damage_received();
void reset();
void shutdown();
bool busy();
bool rest_available(daAlink_c*);
void request_rest(daAlink_c*);
}
