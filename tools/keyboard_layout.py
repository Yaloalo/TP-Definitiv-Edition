#!/usr/bin/env python3
"""Writes a Dusklight keyboard layout (keyboard_bindings.dat) for laptop play without a mouse.

Dusklight ships no default keyboard bindings. This writes the binary file Aurora loads at startup
(extern/aurora/lib/dolphin/pad/pad.cpp: load_keyboard_bindings, format version 3) and enables the
keyboard for controller port 1.

Keys are SDL scancodes, i.e. physical positions: the layout is identical on QWERTY and QWERTZ.

Usage: keyboard_layout.py [output path]
Default output: ~/.local/share/TwilitRealm/Dusklight/keyboard_bindings.dat
Close Dusklight before running; it rewrites the file when bindings are changed in its menu.
"""

import pathlib
import struct
import sys

# SDL3 scancodes (USB HID usage IDs).
SC = {
    "A": 4, "D": 7, "E": 8, "I": 12, "J": 13, "K": 14, "L": 15, "Q": 20, "S": 22, "W": 26,
    "1": 30, "2": 31, "3": 32, "4": 33,
    "ENTER": 40, "SPACE": 44,
    "RIGHT": 79, "LEFT": 80, "DOWN": 81, "UP": 82,
    "LSHIFT": 225,
}
NONE = -1  # PAD_KEY_INVALID

# GameCube buttons (dolphin/pad.h), in the fixed order of Aurora's g_defaultKeys.
BUTTON_ORDER = [
    ("A", 0x0100), ("B", 0x0200), ("X", 0x0400), ("Y", 0x0800), ("START", 0x1000),
    ("Z", 0x0010), ("L", 0x0040), ("R", 0x0020),
    ("DPAD_UP", 0x0008), ("DPAD_DOWN", 0x0004), ("DPAD_LEFT", 0x0001), ("DPAD_RIGHT", 0x0002),
]
# Axes in the fixed order of g_defaultKeyAxis (PAD_AXIS_* 0..9).
AXIS_ORDER = [
    "STICK_RIGHT", "STICK_LEFT", "STICK_UP", "STICK_DOWN",
    "CAM_RIGHT", "CAM_LEFT", "CAM_UP", "CAM_DOWN",
    "TRIGGER_L", "TRIGGER_R",
]

LAYOUT_BUTTONS = {
    "A": "SPACE",        # action / roll / sidestep
    "B": "J",            # sword
    "X": "K",            # item X
    "Y": "L",            # item Y
    "START": "ENTER",    # pause / menu
    "Z": "E",            # Midna
    "L": "LSHIFT",       # Z-target (hold)
    "R": "Q",            # shield / crouch
    "DPAD_UP": "1",
    "DPAD_DOWN": "2",
    "DPAD_LEFT": "3",
    "DPAD_RIGHT": "4",
}
LAYOUT_AXES = {
    "STICK_UP": "W", "STICK_DOWN": "S", "STICK_LEFT": "A", "STICK_RIGHT": "D",
    "CAM_UP": "UP", "CAM_DOWN": "DOWN", "CAM_LEFT": "LEFT", "CAM_RIGHT": "RIGHT",
    # GameCube L/R are analog triggers with a digital click; press both with the same key.
    "TRIGGER_L": "LSHIFT", "TRIGGER_R": "Q",
}

MAGIC = b"KBND"
VERSION = 3
HEADER_ALIGN = 32
PORTS = 4


def port_block(enabled: bool) -> bytes:
    data = struct.pack("<B", 1 if enabled else 0)
    for name, pad_button in BUTTON_ORDER:
        key = LAYOUT_BUTTONS.get(name) if enabled else None
        # PADKeyButtonBinding {s32 scancode; u16 padButton;} + 2 bytes struct padding
        data += struct.pack("<iHxx", SC[key] if key else NONE, pad_button)
    for axis_index, name in enumerate(AXIS_ORDER):
        key = LAYOUT_AXES.get(name) if enabled else None
        # PADKeyAxisBinding {s32 scancode; u16 padAxis; s16 influence;}
        data += struct.pack("<iHh", SC[key] if key else NONE, axis_index, 0)
    return data


def build() -> bytes:
    header = MAGIC + struct.pack("<i", VERSION)
    header += b"\0" * (HEADER_ALIGN - len(header))
    return header + b"".join(port_block(port == 0) for port in range(PORTS))


def main() -> int:
    default = pathlib.Path.home() / ".local/share/TwilitRealm/Dusklight/keyboard_bindings.dat"
    out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else default
    out.parent.mkdir(parents=True, exist_ok=True)
    data = build()
    assert len(data) == HEADER_ALIGN + PORTS * (1 + 12 * 8 + 10 * 8), len(data)
    if out.exists():
        backup = out.with_suffix(".dat.bak")
        backup.write_bytes(out.read_bytes())
        print(f"backed up existing bindings to {backup}")
    out.write_bytes(data)
    print(f"wrote {out} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
