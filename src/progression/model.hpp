#pragma once
#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace remaster::progression {
// Engine-independent rules; save slots hold progress and spent charges, never config values.
struct Model {
    uint32_t flaskShards = 0, staminaFragments = 0, bosses = 0, charges = 3;
    uint32_t bottleSlot = 255;
    std::set<std::string> claimed;
    uint32_t capacity(int start, int cap) const {
        return std::clamp<uint32_t>(std::max(start, 1) + flaskShards / 4, 1, std::max(cap, 1));
    }
    uint32_t potency(int base) const { return std::clamp<uint32_t>(std::max(base, 1) + bosses / 2, 1, 20); }
    int stamina_bonus() const { return std::min<uint32_t>(100, (staminaFragments / 4) * 2); }
    bool claim(const char* name, uint8_t item) {
        // Unattributed drops and repeated minigames cannot become farmable upgrades.
        if (!name || !*name || std::char_traits<char>::length(name) > 256 || claimed.size() >= 512) return false;
        if (!claimed.insert(name).second) return false;
        if (item == 0x13) flaskShards = std::min<uint32_t>(48, flaskShards + 1);
        if (item == 0x14) staminaFragments = std::min<uint32_t>(200, staminaFragments + 1);
        return true;
    }
    void refill(int start, int cap) { charges = capacity(start, cap); }
    bool spend(int start, int cap) {
        charges = std::min(charges, capacity(start, cap));
        if (!charges) return false;
        --charges;
        return true;
    }
};
inline std::vector<uint8_t> encode(const Model& m) {
    std::vector<uint8_t> out;
    auto put = [&](uint32_t v) { for (int i = 0; i < 4; ++i) out.push_back((v >> (i * 8)) & 255); };
    put(0x31464454); // TDF1, explicit little-endian version, no ABI-dependent padding.
    for (auto v : {m.flaskShards, m.staminaFragments, m.bosses, m.charges, m.bottleSlot, uint32_t(m.claimed.size())}) put(v);
    for (const auto& name : m.claimed) { put(name.size()); out.insert(out.end(), name.begin(), name.end()); }
    return out;
}
inline bool decode(const std::vector<uint8_t>& in, Model& out) {
    size_t at = 0;
    bool valid = true;
    auto get = [&]() { uint32_t v = 0; if (at + 4 > in.size()) { valid = false; return v; }
        for (int i = 0; i < 4; ++i) v |= uint32_t(in[at++]) << (i * 8); return v; };
    if (get() != 0x31464454) return false;
    Model m;
    m.flaskShards = get(); m.staminaFragments = get(); m.bosses = get(); m.charges = get(); m.bottleSlot = get();
    uint32_t n = get();
    if (!valid || n > 512 || m.flaskShards > 48 || m.staminaFragments > 200 || m.bosses > 9 || m.charges > 12 ||
        (m.bottleSlot != 255 && (m.bottleSlot < 11 || m.bottleSlot > 14))) return false;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t len = get();
        if (!valid || len == 0 || len > 256 || at + len > in.size()) return false;
        std::string name(reinterpret_cast<const char*>(in.data() + at), len); at += len;
        if (name.find('\0') != std::string::npos || !m.claimed.insert(name).second) return false;
    }
    if (at != in.size()) return false;
    out = std::move(m); return true;
}
}
