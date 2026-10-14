#pragma once
#include "progression/model.hpp"
#include <array>
#include <string_view>
namespace remaster::economy::rare {
inline constexpr uint8_t kRupeeCache=0x3a,kBlessing=0x3b,kFlaskBlessing=0x3c;
inline constexpr std::array<std::string_view,20> kCaches={
    "chest:F_SP108:28","chest:F_SP108:27","chest:F_SP108:63","chest:D_MN05:7","chest:D_MN05:24","chest:D_MN05:26",
    "chest:R_SP109:17","chest:F_SP109:20","chest:D_MN04:27","chest:D_MN04:24",
    "chest:F_SP115:6","chest:D_MN01:16","chest:F_SP113:30","chest:F_SP121:7",
    "chest:F_SP117:2","chest:D_MN10:3","chest:D_MN11:23","chest:D_MN06:45","chest:D_MN07:27","chest:D_MN08:36"
};
inline bool cache(std::string_view name) { return std::find(kCaches.begin(),kCaches.end(),name)!=kCaches.end(); }
inline constexpr int kCacheValue=15;
struct Offer { const char* check; const char* label; uint8_t item; int cost; const char* stage; int room; };
inline constexpr std::array<Offer,2> kOffers={{
    {"purchase:hylian-shield","Hylian Shield — 60 rupees",0x2c,60,"R_SP109",3},
    {"purchase:magic-armor","Magic Armor — 180 rupees",0x30,180,"R_SP160",0}
}};
inline const Offer* offer(std::string_view check) { for (const auto& item:kOffers) if (check==item.check) return &item; return nullptr; }
inline bool migrate_economy(progression::Model& model) {
    if (model.claimed.contains("migration:rare-economy-v1")) return true;
    auto candidate=model;
    // Snapshot old relic provenance before these locations become rupee caches.
    if (candidate.claimed.contains("chest:F_SP109:20") && !candidate.claimed.contains("legacy:forge") &&
        !candidate.claim("legacy:forge",0xff)) return false;
    if (candidate.claimed.contains("chest:R_SP109:17") && !candidate.claimed.contains("legacy:lens") &&
        !candidate.claim("legacy:lens",0xff)) return false;
    if (candidate.claimed.contains("chest:F_SP115:6") && !candidate.claimed.contains("collection:bugs:3") &&
        !candidate.claim("collection:bugs:3",0xff)) return false;
    if (candidate.claimed.contains("chest:F_SP117:2") && !candidate.claimed.contains("collection:poes:20") &&
        !candidate.claim("collection:poes:20",0xff)) return false;
    if (!candidate.claim("migration:rare-economy-v1",0xff)) return false;
    if (candidate.claimed.contains("trade:sigil")) candidate.equipmentUpgrades|=1;
    for (const auto& name:kCaches) if (model.claimed.contains(std::string(name)))
        candidate.rareRupees=std::min<uint32_t>(10000,candidate.rareRupees+kCacheValue);
    model=std::move(candidate); return true;
}
inline bool award_cache(progression::Model& model,const char* name) {
    if (!name || !cache(name) || !model.claim(name,0xff)) return false;
    model.rareRupees=std::min<uint32_t>(10000,model.rareRupees+kCacheValue); return true;
}
inline bool pay(progression::Model& model,const Offer& item) {
    if (model.rareRupees<uint32_t(item.cost) || !model.claim(item.check,0xff)) return false;
    model.rareRupees-=item.cost; return true;
}
}
