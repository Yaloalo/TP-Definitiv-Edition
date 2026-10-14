#pragma once
#include "progression/treasure_model.hpp"
namespace remaster::economy::guild {
struct Recipe {
    const char* check;const char* label;const char* stage;int room;
    std::array<uint32_t,3> cost;uint32_t benefit,prerequisites;
    const char* artifact;uint32_t honor=0,priorHonors=0;
};
inline constexpr std::array<Recipe,8> kRecipes={{
    {"upgrade:scout-stitch","Scout's Stitch: 6 craft relics + 3 thread", "R_SP01",1,{6,0,3},progression::treasures::kScoutStitch,0,nullptr},
    {"upgrade:fletching","Bow Fletching: 8 craft relics + 6 thread", "R_SP109",3,{8,0,6},progression::treasures::kFletching,0,nullptr},
    {"upgrade:bomb-harness","Bomb Harness: 8 ancient runes + 6 thread", "R_SP109",1,{0,8,6},progression::treasures::kBombHarness,0,nullptr},
    {"upgrade:sunward","Sunward Weave: emblem + 6 runes + 4 thread", "R_SP160",0,{0,6,4},progression::treasures::kSunward,1,progression::treasures::kSunwardRelic},
    {"upgrade:mirror-ward","Mirror Ward: heart + 10 runes + 5 thread", "R_SP160",0,{0,10,5},progression::treasures::kMirrorWard,progression::treasures::kSunward,progression::treasures::kMirrorRelic},
    {"honor:craft-crest","Craft Crest: 40 craft relics", "R_SP109",3,{40,0,0},0,0,nullptr,64,0},
    {"honor:ancient-crest","Ancient Crest: 40 ancient runes", "R_SP160",0,{0,40,0},0,0,nullptr,128,0},
    {"honor:hyrule-keeper","Hyrule Keeper: 80 craft relics + 20 runes", "R_SP160",0,{80,20,0},0,0,nullptr,256,64|128}
}};
inline bool eligible(const progression::Model& m,const Recipe& r) {
    if(m.claimed.size()>=512 || m.claimed.contains(r.check) || (m.equipmentUpgrades&r.benefit) ||
        (m.equipmentUpgrades&r.prerequisites)!=r.prerequisites || (r.honor&&(m.honors&r.honor)) || (m.honors&r.priorHonors)!=r.priorHonors || (r.artifact&&!m.claimed.contains(r.artifact)))return false;
    for(size_t i=0;i<r.cost.size();++i)if(m.relics[i]<r.cost[i])return false;
    return true;
}
inline bool apply(progression::Model& m,const Recipe& r) {
    if(!eligible(m,r))return false;
    auto candidate=m;
    if(!candidate.claim(r.check,0xff))return false;
    for(size_t i=0;i<r.cost.size();++i)candidate.relics[i]-=r.cost[i];
    candidate.equipmentUpgrades|=r.benefit;candidate.honors|=r.honor;m=std::move(candidate);return true;
}
}
