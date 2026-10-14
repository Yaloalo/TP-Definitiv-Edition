#!/usr/bin/env bash
# Verifies that every game symbol the built mod imports exists in a Dusklight binary.
# Usage: tools/check_symbols.sh <path to mod .so> <path to extracted dusklight binary>
# Extract the binary once with: Dusklight.AppImage --appimage-extract  (→ squashfs-root/usr/bin/dusklight)
set -euo pipefail
mod_so="$1"
game_bin="$2"
for f in "$mod_so" "$game_bin"; do [[ -f "$f" ]] || { echo "not found: $f" >&2; exit 2; }; done
missing=$(comm -23 \
    <(nm -D --undefined-only "$mod_so" | awk '{print $2}' | sort -u) \
    <(nm -D --defined-only "$game_bin" | awk '{print $3}' | sort -u) \
    | grep -v -E '@(GLIBC|GLIBCXX|CXXABI|GCC)_|^_ITM_|^__gmon_start__$' || true)
if [[ -n "$missing" ]]; then
    echo "Missing symbols (mod will fail to load on this Dusklight build):"
    echo "$missing" | c++filt
    exit 1
fi
echo "OK: all $(nm -D --undefined-only "$mod_so" | wc -l) imported symbols resolve in $(basename "$game_bin")"
