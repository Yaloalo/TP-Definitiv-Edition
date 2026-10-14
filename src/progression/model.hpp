#pragma once
#include <algorithm>
#include <array>
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
    bool growthReady = false;
    uint32_t growthFragments = 0, healthPoints = 0, staminaPoints = 0;
    uint32_t rareRupees = 0, equipmentUpgrades = 0;
    // Craft relics, ancient runes and Gilded Thread are finite upgrade components.
    std::array<uint32_t,3> relics{};
    uint32_t honors=0, wardCharges=3;
    bool wardReady=false;
    uint32_t growth_points() const { return growthFragments / 5; }
    uint32_t unassigned() const { return growth_points() - std::min(growth_points(), healthPoints + staminaPoints); }
    void migrate_growth(int nativeMaxLife) {
        if (growthReady) return;
        const uint32_t native = std::clamp(nativeMaxLife - 15, 0, 85);
        healthPoints = native / 5;
        // Round old small stamina bonuses up, never remove an earned benefit.
        staminaPoints = std::min<uint32_t>(5, (std::min<uint32_t>(100,(staminaFragments/4)*2) + 19) / 20);
        growthFragments = std::min<uint32_t>(110, native + staminaPoints * 5 + staminaFragments % 4);
        growthReady = true;
    }
    void add_growth(uint32_t fragments) { growthFragments = std::min<uint32_t>(110, growthFragments + std::min<uint32_t>(110,fragments)); }
    bool allocate(bool stamina) {
        if (!growthReady || !unassigned()) return false;
        auto& destination = stamina ? staminaPoints : healthPoints;
        if (destination >= (stamina ? 5u : 17u)) return false;
        ++destination; return true;
    }
    bool exchange(bool toStamina) {
        if (!growthReady) return false;
        auto& source = toStamina ? healthPoints : staminaPoints;
        auto& destination = toStamina ? staminaPoints : healthPoints;
        if (!source || destination >= (toStamina ? 5u : 17u)) return false;
        --source; ++destination; return true;
    }
    int native_max_life() const { return std::min<uint32_t>(100,(3+healthPoints)*5+growthFragments%5); }
    uint32_t capacity(int start, int cap) const {
        return std::clamp<uint32_t>(std::max(start, 1) + flaskShards / 4, 1, std::max(cap, 1));
    }
    uint32_t potency(int base) const { return std::clamp<uint32_t>(std::max(base, 1) + bosses / 2, 1, 20); }
    int stamina_bonus() const { return growthReady ? int(staminaPoints*20) : std::min<uint32_t>(100, (staminaFragments / 4) * 2); }
    bool claim(const char* name, uint8_t item) {
        // Unattributed drops and repeated minigames cannot become farmable upgrades.
        if (!name || !*name || std::char_traits<char>::length(name) > 256 || claimed.size() >= 512) return false;
        if (!claimed.insert(name).second) return false;
        if (item == 0x13) flaskShards = std::min<uint32_t>(48, flaskShards + 1);
        if (item == 0x14) {
            if (growthReady) add_growth(1);
            else staminaFragments = std::min<uint32_t>(200, staminaFragments + 1);
        }
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
    put(0x33464454); // TDF3; decoder accepts the prior TDF1/TDF2 layouts.
    for (auto v : {m.flaskShards, m.staminaFragments, m.bosses, m.charges, m.bottleSlot, uint32_t(m.claimed.size())}) put(v);
    for (const auto& name : m.claimed) { put(name.size()); out.insert(out.end(), name.begin(), name.end()); }
    for (auto v : {uint32_t(m.growthReady),m.growthFragments,m.healthPoints,m.staminaPoints,m.rareRupees,m.equipmentUpgrades}) put(v);
    for(auto v:m.relics)put(v);
    put(m.honors);put(m.wardCharges);put(uint32_t(m.wardReady));
    return out;
}
inline bool decode(const std::vector<uint8_t>& in, Model& out) {
    size_t at = 0;
    bool valid = true;
    auto get = [&]() { uint32_t v = 0; if (at + 4 > in.size()) { valid = false; return v; }
        for (int i = 0; i < 4; ++i) v |= uint32_t(in[at++]) << (i * 8); return v; };
    const auto magic = get();
    if (magic != 0x31464454 && magic != 0x32464454 && magic != 0x33464454) return false;
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
    if (magic == 0x32464454 || magic == 0x33464454) {
        const auto flag = get(); m.growthReady = flag == 1;
        m.growthFragments=get(); m.healthPoints=get(); m.staminaPoints=get(); m.rareRupees=get(); m.equipmentUpgrades=get();
        if (!valid || flag>1 || m.growthFragments>110 || m.healthPoints>17 || m.staminaPoints>5 ||
            m.healthPoints+m.staminaPoints>m.growth_points() || m.rareRupees>10000 || m.equipmentUpgrades>(magic==0x32464454?15u:255u) ||
            (!m.growthReady && (m.growthFragments || m.healthPoints || m.staminaPoints))) return false;
    }
    if(magic==0x33464454) {
        for(auto& v:m.relics)v=get();
        m.honors=get();m.wardCharges=get();const auto flag=get();m.wardReady=flag==1;
        if(!valid || flag>1 || m.wardCharges>5 || m.honors>1023 ||
            std::any_of(m.relics.begin(),m.relics.end(),[](auto v){return v>512;}))return false;
    }
    if (at != in.size()) return false;
    out = std::move(m); return true;
}
}
