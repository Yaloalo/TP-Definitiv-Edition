#!/usr/bin/env python3
"""Build a launch folder without loading standalone copies of integrated modules."""
import argparse
import json
from pathlib import Path

INTEGRATED = {
    "com.dusklight.twilight_compass.dusk",
    "dev.baxlek.unrestricted_items.dusk",
    "org.dusklight.tp_classic_buttons.dusk",
    "roll_and_sprint.dusk",
    "snappy_turns.dusk",
    "tp_remaster.dusk",
}


def prepare(source: Path, destination: Path, overhaul: Path) -> Path:
    source = source.resolve()
    destination = destination.resolve()
    overhaul = overhaul.resolve()
    if not source.is_dir():
        raise ValueError(f"Mod source folder not found: {source}")
    if not overhaul.is_file():
        raise ValueError(f"Build the overhaul first: {overhaul}")
    if source == destination:
        raise ValueError("Launch folder must differ from the source folder")
    destination.mkdir(parents=True, exist_ok=True)
    manifest = destination / ".managed-links.json"
    previous = json.loads(manifest.read_text()) if manifest.exists() else []
    wanted = {p.name: p.resolve() for p in source.glob("*.dusk") if p.name not in INTEGRATED}
    wanted["tp_remaster.dusk"] = overhaul
    # Refuse to overwrite files not created by this tool.
    for name in wanted:
        path = destination / name
        if path.exists() and not (name in previous and path.is_symlink()):
            raise ValueError(f"Unmanaged file in launch folder: {path}")
    for name in previous:
        if Path(name).name != name:
            raise ValueError("Invalid managed link name")
        path = destination / name
        if path.is_symlink():
            path.unlink()
    for name, target in wanted.items():
        (destination / name).symlink_to(target)
    manifest.write_text(json.dumps(sorted(wanted), indent=2) + "\n")
    return destination


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("overhaul", type=Path)
    options = parser.parse_args()
    print(prepare(options.source, options.destination, options.overhaul))
