#pragma once

// Service declarations shared by all translation units. The services themselves are imported
// (IMPORT_SERVICE) exactly once, in mod.cpp.
#include "mods/service.hpp"
#include "mods/svc/config.h"
#include "mods/svc/game_mode.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/host.h"
#include "mods/svc/log.hpp"
#include "mods/svc/resource.h"
#include "mods/svc/save.h"
#include "mods/svc/item.h"
#include "mods/svc/message.h"
#include "mods/svc/ui.h"
#include "mods/svc/actor.h"

namespace remaster {

// Identifier of the game mode registered by this mod.
inline constexpr const char* kGameModeId = "remaster";

// True while the Definitive Overhaul game mode is active. All hooks are installed once at load
// time (HookService::uninstall drops every callback this mod has on a target, so per-feature
// uninstalling is unsafe) and return early when this is false.
bool is_active();

}  // namespace remaster
