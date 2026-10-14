#pragma once
#include "services.hpp"
namespace remaster::progression::treasures {
ModResult install();
bool ensure();
uint32_t message(uint32_t);
bool custom_item(uint8_t);
bool resolve(const ItemCheckInfo*,ItemCheckResolution*);
void tick();
float sprint_cost_scale();
int64_t recharge_interval(int64_t,bool arrows);
}
