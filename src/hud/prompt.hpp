#pragma once
#include <string_view>
#include "mods/api.h"
namespace remaster::hud { void draw_prompt(std::string_view text); ModResult install_prompts(); }
