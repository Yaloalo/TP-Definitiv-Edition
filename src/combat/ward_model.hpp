#pragma once
#include "progression/treasure_model.hpp"
#include "resources/recharge_model.hpp"
namespace remaster::combat::ward {
inline unsigned capacity(const progression::Model& m) {
    return 3+unsigned(bool(m.equipmentUpgrades&1))+unsigned(bool(m.equipmentUpgrades&progression::treasures::kSunward));
}
inline float damage_scale(const progression::Model& m) {
    return m.equipmentUpgrades&progression::treasures::kMirrorWard?0.4f:0.5f;
}
inline int64_t recovery_time(const progression::Model& m,int64_t ms) {
    return m.equipmentUpgrades&progression::treasures::kSunward?std::max<int64_t>(100,ms*3/4):ms;
}
inline bool initialize(progression::Model& m) {
    if(m.wardReady)return false;
    m.wardReady=true;m.wardCharges=capacity(m);return true;
}
inline bool spend(progression::Model& m) {
    m.wardCharges=std::min(m.wardCharges,capacity(m));
    if(!m.wardCharges)return false;
    --m.wardCharges;return true;
}
}
