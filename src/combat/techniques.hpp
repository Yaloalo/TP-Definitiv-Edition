#pragma once
#include "mods/api.h"
#include <cstdint>
class daAlink_c;
namespace remaster::combat::techniques {
enum class Attack { Ordinary, Heavy, GuardCounter };
ModResult install();
void reset();
void damage_received();
// Called at the native item-action decision boundary, before shield-bash dispatch.
bool try_action(daAlink_c* link, int& result);
int64_t finish_cost(int64_t ordinary);
Attack current_attack(const daAlink_c* link);
uint64_t attack_serial();
}
