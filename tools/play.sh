#!/usr/bin/env bash
# Launches the latest verified Definitive Overhaul package. Gameplay features belong to the mod;
# no built-in gameplay settings preset is required.
#
# Usage: tools/play.sh [extra Dusklight args...]
#   Opens Dusklight's start menu (disc image from config.json `backend.isoPath`), where the
#   "Definitive Overhaul" game mode is picked on the Play button's ‹ › selector.
#   Passing --dvd <image> skips the start menu and boots the vanilla game mode directly.
#
# The curated folder is read to build a local launch folder. Standalone copies
# of integrated gameplay modules are excluded, so each feature runs once.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
dusklight="${DUSKLIGHT_APPIMAGE:-$HOME/Games/Dusklight/Dusklight.AppImage}"
curated_mods="${DEFINITIVE_MODS:-$HOME/Games/Dusklight/definitive/mods}"
mods="$root/build/runtime-mods"
package="$(python3 "$root/tools/qa.py" --build "$root/build" --launch-package)"
python3 "$root/tools/prepare_mods.py" "$curated_mods" "$mods" "$package" >/dev/null

exec "$dusklight" --mods "$mods" -L debug "$@"
