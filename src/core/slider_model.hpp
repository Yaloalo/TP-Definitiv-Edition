#pragma once
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string_view>
namespace remaster::tuning {
inline bool slider_value(std::string_view input, int64_t min, int64_t max, int64_t step, int64_t& out) {
    double value = 0;
    auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc() || end != input.data() + input.size() || !std::isfinite(value)) return false;
    value = std::clamp(value, double(min), double(max));
    const auto spacing = std::max<int64_t>(1, step);
    out = std::clamp<int64_t>(min + int64_t(std::llround((value - min) / spacing)) * spacing, min, max);
    return true;
}
}
