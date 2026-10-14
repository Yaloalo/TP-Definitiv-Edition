#pragma once
#include "progression/model.hpp"
#include "progression/treasure_table.hpp"
#include <charconv>
namespace remaster::progression::treasures {
inline constexpr uint8_t kRelicFind=0x4e, kGildedThread=0x52, kUpgradeGift=0x57;
inline constexpr uint32_t kSunward=8, kMirrorWard=16, kFletching=32, kScoutStitch=64, kBombHarness=128;
inline constexpr const char* kSunwardRelic="relic:sunward";
inline constexpr const char* kMirrorRelic="relic:mirror";
inline bool bug(std::string_view name) {
    if(!name.starts_with("bug:"))return false;
    int id=0;auto s=name.substr(4);auto parsed=std::from_chars(s.data(),s.data()+s.size(),id);
    return parsed.ec==std::errc{}&&parsed.ptr==s.data()+s.size()&&id>=192&&id<=215;
}
inline bool thread(Model& m,const char* name) {
    if(!name || !bug(name) || !m.claim(name,0xff))return false;
    m.relics[2]=std::min<uint32_t>(24,m.relics[2]+1);return true;
}
inline bool material(Model& m,const char* name,const Find* entry) {
    auto candidate=m;
    if(!candidate.claim(name,0xff))return false;
    const auto kind=entry?entry->kind:Kind::Craft;
    if(kind==Kind::Sunward || kind==Kind::Mirror) {
        const auto artifact=kind==Kind::Sunward?kSunwardRelic:kMirrorRelic;
        if(!candidate.claimed.contains(artifact)&&!candidate.claim(artifact,0xff))return false;
    } else if(kind==Kind::Craft || kind==Kind::Ancient) {
        const int resource=kind==Kind::Ancient?1:0;
        candidate.relics[resource]=std::min<uint32_t>(512,candidate.relics[resource]+1);
    } else return false;
    m=std::move(candidate);return true;
}
inline unsigned found(const Model& m) {
    unsigned count=0;for(const auto& r:kFinds)count+=m.claimed.contains(r.check);return count;
}
inline bool migrate(Model& m) {
    if(m.claimed.contains("migration:relic-ledger-v1"))return true;
    auto candidate=m;
    if(!candidate.claim("migration:relic-ledger-v1",0xff))return false;
    for(const auto& r:kFinds)if(m.claimed.contains(r.check)) {
        if(r.kind==Kind::Craft || r.kind==Kind::Ancient) {
            const int resource=r.kind==Kind::Ancient?1:0;
            candidate.relics[resource]=std::min<uint32_t>(512,candidate.relics[resource]+1);
        } else if(r.kind==Kind::Sunward || r.kind==Kind::Mirror) {
            const auto artifact=r.kind==Kind::Sunward?kSunwardRelic:kMirrorRelic;
            if(!candidate.claimed.contains(artifact)&&!candidate.claim(artifact,0xff))return false;
        }
    }
    for(const auto& name:m.claimed)if(bug(name))candidate.relics[2]=std::min<uint32_t>(24,candidate.relics[2]+1);
    m=std::move(candidate);return true;
}
inline float sprint_scale(const Model& m) { return m.equipmentUpgrades&kScoutStitch?0.9f:1.0f; }
inline int64_t refill_interval(const Model& m,int64_t ms,bool arrows) {
    const auto flag=arrows?kFletching:kBombHarness;
    return m.equipmentUpgrades&flag?std::max<int64_t>(100,ms*3/4):ms;
}
}
