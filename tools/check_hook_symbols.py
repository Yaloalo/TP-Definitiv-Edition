#!/usr/bin/env python3
"""Check compiled named hook targets against a Dusklight ELF symbol manifest.

Requires pyelftools and libzstd. This complements check_symbols.sh; it does not
prove that a hook's runtime patch backend or in-game behavior succeeds.
"""
import argparse
import ctypes
import ctypes.util
import struct
import re
from collections import Counter
from pathlib import Path

from elftools.elf.elffile import ELFFile


def section(path, name):
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        if elf.elfclass != 64 or not elf.little_endian:
            raise ValueError("This checker currently supports little-endian 64-bit ELF files")
        result = elf.get_section_by_name(name)
        if result is None:
            raise ValueError(f"Missing {name} section: {path}")
        return result.data()


def manifest_symbols(binary):
    blob = section(binary, "symdb")
    magic, version, compression, raw_length, stored_length, _, _, count = struct.unpack_from("<8sIIQQI32sI", blob)
    if magic != b"SYMGEN\0\0" or version != 2:
        raise ValueError("Unsupported symbol manifest")
    payload = blob[72:72 + stored_length]
    if compression == 1:
        library = ctypes.util.find_library("zstd")
        if library is None:
            raise ValueError("Install libzstd to read the compressed symbol manifest")
        zstd = ctypes.CDLL(library)
        zstd.ZSTD_decompress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
        zstd.ZSTD_decompress.restype = ctypes.c_size_t
        output = ctypes.create_string_buffer(raw_length)
        size = zstd.ZSTD_decompress(output, raw_length, payload, stored_length)
        if size != raw_length:
            raise ValueError("Symbol manifest decompression failed")
        payload = output.raw
    elif compression != 0:
        raise ValueError("Unsupported symbol manifest compression")
    if len(payload) != raw_length or count * 24 > raw_length:
        raise ValueError("Invalid symbol manifest size")
    strings = payload[count * 24:]
    symbols = {}
    for index in range(count):
        _, address, offset, flags = struct.unpack_from("<QQII", payload, index * 24)
        name = strings[offset:strings.index(b"\0", offset)].decode()
        symbols[name] = (address, flags)
    return symbols


def named_hooks(mod):
    data = section(mod, "modmeta")
    offset = 0
    names = []
    while offset < len(data):
        size, kind, _ = struct.unpack_from("<HBB", data, offset)
        if size == 0:
            offset += 8  # linker alignment padding
            continue
        if size < 8 or size % 8 or offset + size > len(data):
            raise ValueError(f"Invalid mod metadata record at {offset}")
        if kind == 6:
            names.append(data[offset + 16:offset + size].split(b"\0", 1)[0].decode())
        offset += size
    return names


def check(mod, binary, dynamic_sources=()):
    symbols = manifest_symbols(binary)
    hooks = named_hooks(mod)
    errors = []
    for name, count in Counter(hooks).items():
        if count != 1:
            errors.append(f"Duplicate hook metadata: {name} ({count} records)")
        entry = symbols.get(name)
        if entry is None:
            errors.append(f"Missing hook target: {name}")
        elif not entry[0] or not entry[1] & 1 or entry[1] & (1 << 4):
            errors.append(f"Hook target is not unambiguous executable code: {name}")
    if errors:
        raise ValueError("\n".join(errors))
    print(f"OK: all {len(hooks)} named hook targets resolve uniquely in {binary.name}")
    dynamic = set()
    for source in dynamic_sources:
        dynamic.update(re.findall(r'\bresolve(?:_slider)?\("([^"]+)"', source.read_text()))
    for name in sorted(dynamic):
        entry = symbols.get(name)
        if entry is None or not entry[0] or not entry[1] & 1 or entry[1] & (1 << 4):
            raise ValueError(f"Dynamic target is missing or ambiguous: {name}")
    if dynamic:
        print(f"OK: all {len(dynamic)} dynamic function targets resolve uniquely in {binary.name}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mod", type=Path)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--dynamic-source", type=Path, action="append", default=[],
                        help="Also check literal resolve()/resolve_slider() targets in this source")
    options = parser.parse_args()
    check(options.mod, options.binary, options.dynamic_source)
