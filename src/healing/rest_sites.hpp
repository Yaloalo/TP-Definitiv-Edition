#pragma once
#include <array>
#include <string_view>
namespace remaster::healing::stones {
struct Site {
    const char* stage;
    int room;
    const char* name;
    bool nativeSpawn;
    float x = 0, y = 0, z = 0;
};
// Native-spawn sites read actual PLYR records from the loaded room. Fixed anchors are
// transcribed from TP_Tracker's disc-derived positions.json/hint_signs.json, with
// conservative offsets. All placements must pass floor, water and wall checks.
inline constexpr std::array<Site, 32> kSites = {{
    {"R_SP01",4,"Link's house",true}, {"R_SP01",7,"Link's basement",true},
    {"F_SP104",1,"Ordon Spring",true}, {"R_SP109",0,"Renado's sanctuary",true},
    {"R_SP109",2,"Elde Inn",true},
    {"D_MN05",22,"Forest Temple entrance",true}, {"D_MN04",1,"Goron Mines entrance",true},
    {"D_MN01",0,"Lakebed Temple entrance",true}, {"D_MN10",0,"Arbiter's Grounds entrance",true},
    {"D_MN11",0,"Snowpeak Ruins entrance",true}, {"D_MN06",0,"Temple of Time entrance",true},
    {"D_MN07",0,"City in the Sky entrance",true}, {"D_MN08",0,"Palace of Twilight entrance",true},
    {"D_MN09",11,"Hyrule Castle entrance",true},
    {"F_SP103",0,"Ordon Village",false,518,88,-860},
    {"F_SP108",4,"Coro's clearing",false,-13659,0,-14367},
    {"F_SP108",6,"North Faron Woods",true},
    {"F_SP117",1,"Sacred Grove clearing",false,0,1700,-5435},
    {"F_SP110",0,"Death Mountain trail",true},
    {"F_SP109",0,"Kakariko Village",false,900,86,6665},
    {"F_SP116",0,"Castle Town",false,1589,-100,2818},
    {"F_SP126",0,"Upper Zora's River",false,-1116,400,-102956},
    {"F_SP112",0,"Iza's river lodge",true},
    {"F_SP113",1,"Zora's Domain approach",true},
    {"F_SP115",0,"Lake Hylia bluff",false,-106184,-17110,56667},
    {"F_SP124",0,"Gerudo Desert trail",true},
    {"F_SP114",0,"Snowpeak trail",true},
    {"F_SP128",0,"Hidden Village",false,2200,105,-1200},
    {"F_SP121",0,"Hyrule Field shelter",false,-200,1000,4250},
    {"D_MN05",0,"Forest Temple upper refuge",false,-1972,3150,7611},
    {"D_MN04",17,"Goron Mines refuge",false,11394,2879,-17913},
    {"D_MN01",2,"Lakebed Temple refuge",false,-4765,-1,272},
}};
inline bool matches(const Site& site, std::string_view stage, int room) { return stage == site.stage && room == site.room; }
}
