#include "core/difficulty.hpp"

#include <cstring>

#include "core/tuning.hpp"
#include "d/d_com_inf_game.h"

namespace remaster::difficulty {
namespace {

struct Tier {
    const char* stagePrefix;  // 6-char dungeon prefix; boss/miniboss rooms add a suffix (D_MN05A)
    const char* name;
    float multiplier;
};

// Dungeon tiers in story order. Index = dungeons cleared before entering it on a normal playthrough.
constexpr Tier kDungeonTiers[] = {
    {"D_MN05", "Forest Temple", 0.75f},
    {"D_MN04", "Goron Mines", 0.9f},
    {"D_MN01", "Lakebed Temple", 1.0f},
    {"D_MN10", "Arbiter's Grounds", 1.3f},
    {"D_MN11", "Snowpeak Ruins", 1.5f},
    {"D_MN06", "Temple of Time", 1.6f},
    {"D_MN07", "City in the Sky", 1.7f},
    {"D_MN08", "Palace of Twilight", 1.8f},
    {"D_MN09", "Hyrule Castle", 2.0f},
};

constexpr Tier kCaveOfOrdeals = {"D_SB01", "Cave of Ordeals", 2.5f};

// Overworld tier by number of dungeons cleared (0..8).
constexpr float kProgressMultiplier[] = {0.5f, 0.75f, 0.9f, 1.0f, 1.3f, 1.5f, 1.6f, 1.7f, 1.8f};
constexpr const char* kProgressName[] = {
    "Introduction", "Early game", "Early game", "Mid game", "Mid game",
    "Late game", "Late game", "Late game", "Endgame",
};

const Tier* current_dungeon() {
    const char* stage = dComIfGp_getStartStageName();
    if (stage == nullptr) {
        return nullptr;
    }
    for (const Tier& tier : kDungeonTiers) {
        if (std::strncmp(stage, tier.stagePrefix, 6) == 0) {
            return &tier;
        }
    }
    if (std::strncmp(stage, kCaveOfOrdeals.stagePrefix, 6) == 0) {
        return &kCaveOfOrdeals;
    }
    return nullptr;
}

}  // namespace

int dungeons_cleared() {
    int count = 0;
    // Crystal 0-2: Forest Temple, Goron Mines, Lakebed; crystal 3: Palace of Twilight
    // (d_a_obj_bosswarp.cpp, d_meter2.cpp). Mirror 0: Arbiter's Grounds; 1-3: Snowpeak, ToT, City.
    for (u8 i = 0; i < 4; ++i) {
        count += dComIfGs_isCollectCrystal(i) ? 1 : 0;
        count += dComIfGs_isCollectMirror(i) ? 1 : 0;
    }
    return count;
}

float multiplier() {
    if (!tuning::get().difficultyCurve) {
        return 1.0f;
    }
    float result = kProgressMultiplier[dungeons_cleared()];
    if (const Tier* dungeon = current_dungeon(); dungeon != nullptr && dungeon->multiplier > result) {
        result = dungeon->multiplier;
    }
    return result;
}

const char* tier_name() {
    if (!tuning::get().difficultyCurve) {
        return "Flat (curve disabled)";
    }
    if (const Tier* dungeon = current_dungeon(); dungeon != nullptr) {
        return dungeon->name;
    }
    return kProgressName[dungeons_cleared()];
}

}  // namespace remaster::difficulty
