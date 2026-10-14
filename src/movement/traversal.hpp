#pragma once

class daAlink_c;

// Traversal stamina (DESIGN_VISION.md §2 "connect the systems"): climbing and fast swimming drain
// stamina, so Link arrives at a fight winded after a long climb. Running dry only exhausts Link
// (no stamina actions until recovered); it does not make him fall or sink.
namespace remaster::movement::traversal {

// Called once per 30 Hz tick from player.cpp, before stamina::tick.
void tick(daAlink_c* link);

}  // namespace remaster::movement::traversal
