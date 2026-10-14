#pragma once
#include "economy/guild_model.hpp"
namespace remaster::economy::guild {
bool can_craft(const Recipe&);
bool craft(const Recipe&);
}
