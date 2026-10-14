#pragma once
#include "mods/api.h"
#include "progression/model.hpp"
namespace remaster::progression {
inline constexpr uint8_t kFlaskShard = 0x13, kStaminaFragment = 0x14;
ModResult install();
Model& model();
bool ready();
void persist();
void load();
int stamina_bonus();
}
