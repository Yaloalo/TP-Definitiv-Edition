// Ported from Dawnlight, src/enemy_slow_motion/scope.hpp
// Source commit: 9bb214d47617c9bc295d22facf36c35d17661e5a.
// Adapted to the Definitive Overhaul game mode, shared hooks and tuning.
#pragma once

#include <array>
#include <cstddef>

namespace remaster::combat::enemies {
template <typename Step, std::size_t N>
Step* active_enemy_scope(std::array<Step, N>& steps, std::size_t depth) {
    if (depth == 0 || depth > N) return nullptr;
    auto& step = steps[depth - 1];
    // An inactive nested execute masks its parent; never search farther down.
    return step.actor != nullptr && step.profile != nullptr ? &step : nullptr;
}
}
