#!/usr/bin/env bash
# Switches the active HD texture pack. Packs live in ~/Games/Dusklight/texture-packs/<name>/; the
# active one is symlinked into Dusklight's texture_replacements folder (Aurora follows directory
# symlinks when scanning it). Only one full pack should be active: packs replace the same textures.
#
# Usage: tools/texture_pack.sh [status | off | <pack name>]
# Applies on the next game start. Mod-provided textures (e.g. Enhanced Hero's Tunic) always win over
# texture_replacements.
set -euo pipefail
packs="${TEXTURE_PACKS:-$HOME/Games/Dusklight/texture-packs}"
target="${TEXTURE_REPLACEMENTS:-$HOME/.local/share/TwilitRealm/Dusklight/texture_replacements}"
link="$target/active-pack"

status() {
    if [[ -L "$link" ]]; then
        echo "active: $(basename "$(readlink "$link")")"
    else
        echo "active: none"
    fi
    echo "available:"
    for dir in "$packs"/*/; do
        [[ -d "$dir" ]] && echo "  $(basename "$dir")"
    done
}

mkdir -p "$target"
case "${1:-status}" in
    status)
        status
        ;;
    off)
        rm -f "$link"
        status
        ;;
    *)
        pack="$packs/$1"
        if [[ ! -d "$pack" ]]; then
            echo "unknown pack '$1'" >&2
            status >&2
            exit 2
        fi
        ln -sfn "$pack" "$link"
        status
        ;;
esac
