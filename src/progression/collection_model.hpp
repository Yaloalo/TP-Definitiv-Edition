#pragma once
#include "economy/rare_model.hpp"
namespace remaster::progression::collections {
struct Milestone { const char* check; const char* label; unsigned threshold; uint8_t item; unsigned growth,shards,emblem; };
inline constexpr std::array<Milestone,4> kBugs={{
    {"collection:bugs:1","First pair: growth blessing",1,economy::rare::kBlessing,5,0,0},
    {"collection:bugs:3","Three pairs: Deepwater Craft Relic",3,0x38,0,0,0},
    {"collection:bugs:6","Six pairs: flask capacity blessing",6,economy::rare::kFlaskBlessing,0,4,0},
    {"collection:bugs:12","Twelve pairs: Agitha's Golden Crest",12,economy::rare::kBlessing,0,0,2}
}};
inline constexpr std::array<Milestone,5> kPoes={{
    {"collection:poes:5","Five souls: growth fragment",5,0x14,1,0,0},
    {"collection:poes:10","Ten souls: growth blessing",10,economy::rare::kBlessing,5,0,0},
    {"collection:poes:20","Twenty souls: Twilight Sigil",20,0x39,0,0,0},
    {"collection:poes:40","Forty souls: flask capacity blessing",40,economy::rare::kFlaskBlessing,0,4,0},
    {"collection:poes:60","Sixty souls: Jovani's Spirit Crest",60,economy::rare::kBlessing,0,0,4}
}};
inline const Milestone* milestone(std::string_view name) {
    for(const auto& r:kBugs) if(name==r.check)return &r;
    for(const auto& r:kPoes) if(name==r.check)return &r;
    return nullptr;
}
inline bool apply(Model& m,const Milestone& r,int start,int cap) {
    if (!m.claim(r.check,0xff)) return false;
    m.add_growth(r.growth);
    const auto old=m.capacity(start,cap);
    m.flaskShards=std::min<uint32_t>(48,m.flaskShards+r.shards);
    const auto now=m.capacity(start,cap);
    m.charges=std::min<uint32_t>(now,m.charges+now-old);
    m.equipmentUpgrades|=r.emblem;
    return true;
}
}
