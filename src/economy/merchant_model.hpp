#pragma once
#include "progression/model.hpp"
#include <array>
#include <string_view>
namespace remaster::economy::merchants {
struct Relic {
    uint8_t id, reward, display;
    const char* name;
    const char* check;
    const char* stage;
    int room;
    const char* trade;
    const char* label;
};
// Finite, named artifacts. Each is tied to one permanent reward, not interchangeable money.
inline constexpr std::array<Relic, 4> kRelics = {{
    {0x15, 0x2c, 0x3d, "Goron Forge Relic", "chest:F_SP109:20", "R_SP109", 3, "trade:forge", "Malo: forge relic → Hylian Shield"},
    {0x37, 0x3e, 0x3e, "Surveyor's Lens", "chest:R_SP109:17", "R_SP109", 3, "trade:lens", "Malo: surveyor's lens → Hawkeye"},
    {0x38, 0x4f, 0x3d, "Deepwater Craft Relic", "chest:F_SP115:6", "R_SP109", 1, "trade:craft", "Barnes: craft relic → larger bomb capacity"},
    {0x39, 0x30, 0x30, "Twilight Sigil", "chest:F_SP117:2", "R_SP160", 0, "trade:sigil", "Malo Mart: twilight sigil → Magic Armor"},
}};
inline const Relic* relic_for_item(uint8_t id) { for (const auto& r : kRelics) if (r.id == id) return &r; return nullptr; }
inline const Relic* relic_for_check(std::string_view check) { for (const auto& r : kRelics) if (check == r.check) return &r; return nullptr; }
inline bool owns_relic(const progression::Model& model, const Relic& relic) { return model.claimed.contains(relic.check); }
inline bool redeemed(const progression::Model& model, const Relic& relic) { return model.claimed.contains(relic.trade); }
inline bool can_redeem(const progression::Model& model, const Relic& relic, std::string_view stage, int room, bool ownsReward, bool pending) {
    return stage == relic.stage && room == relic.room && owns_relic(model, relic) && !redeemed(model, relic) && !ownsReward && !pending;
}
}
