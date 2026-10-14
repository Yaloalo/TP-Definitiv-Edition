#pragma once

#include "mods/api.h"
#include "mods/svc/ui.h"

// In-game texture pack switcher (Mods window → TP Definitive Overhaul → Graphics).
//
// Packs are folders in ~/Games/Dusklight/texture-packs/<name>/. The active one is a symlink
// `active-pack` inside Dusklight's texture_replacements folder (shared with tools/texture_pack.sh,
// so the choice persists and both tools agree). After switching, the host's
// dusk::texture_replacements::reload() rescans the folder, the same call the built-in
// "Use Texture Pack" setting makes, so the change applies live. Works with or without the game mode.
namespace remaster::presentation::texture_packs {

// Adds the pack dropdown to a panel. Rescans the packs folder each time the panel is built.
ModResult build_controls(UiElementHandle pane);

}  // namespace remaster::presentation::texture_packs
