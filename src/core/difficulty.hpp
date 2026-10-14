#pragma once

// Difficulty curve (DESIGN_VISION.md §9): challenge grows with progress instead of being flat.
namespace remaster::difficulty {

// Number of dungeons cleared so far (0..8), derived from Fused Shadow / Mirror Shard save bits.
int dungeons_cleared();

// Multiplier applied on top of the tuning percentages. 1.0 = reference (around Lakebed Temple).
// Inside a dungeon the dungeon's own tier applies; elsewhere the tier follows progress. The higher
// of the two wins, so revisiting early areas late in the game stays challenging.
float multiplier();

// Short human-readable description of the current tier, for the mod panel.
const char* tier_name();

}  // namespace remaster::difficulty
