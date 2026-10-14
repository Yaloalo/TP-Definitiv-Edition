#pragma once
#include "mods/api.h"
#include <cstdint>
namespace remaster::input::combat_controller {
struct Sample { uint32_t device = 0; bool heavy = false; bool focused = false; };
ModResult install();
Sample sample();
}
