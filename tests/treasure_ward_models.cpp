#include "economy/guild_model.hpp"
#include "combat/ward_model.hpp"
#include <cassert>
#include <iostream>
using namespace remaster;
int main() {
    using namespace progression;using namespace treasures;
    std::array<unsigned,7> counts{};std::set<std::string> checks;
    for(const auto& r:kFinds){assert(checks.insert(r.check).second);++counts[size_t(r.kind)];}
    assert(kFinds.size()==278&&counts[size_t(Kind::Growth)]==14&&counts[size_t(Kind::Flask)]==12&&counts[size_t(Kind::Rupees)]==20);
    assert(counts[size_t(Kind::Craft)]==146&&counts[size_t(Kind::Ancient)]==84);
    Model fresh;fresh.migrate_growth(15);
    for(const auto& r:kFinds) {
        if(r.kind==Kind::Growth)assert(fresh.claim(r.check,0x14));
        else if(r.kind==Kind::Flask)assert(fresh.claim(r.check,0x13));
        else if(r.kind!=Kind::Rupees)assert(material(fresh,r.check,&r));
    }
    assert(fresh.growthFragments==14&&fresh.flaskShards==12&&fresh.relics[0]==146&&fresh.relics[1]==84);
    fresh.add_growth(85+11);assert(fresh.growthFragments==110); // native + curated + collection budget
    for(int i=192;i<216;++i){auto name="bug:"+std::to_string(i);assert(thread(fresh,name.c_str()));assert(!thread(fresh,name.c_str()));}
    assert(fresh.relics[2]==24&&fresh.growthFragments==110);
    assert(!economy::guild::eligible(fresh,economy::guild::kRecipes[3]));fresh.equipmentUpgrades|=1;
    for(const auto& r:economy::guild::kRecipes){assert(economy::guild::apply(fresh,r));assert(!economy::guild::apply(fresh,r));}
    assert(fresh.relics[2]==0&&fresh.relics[0]==12&&fresh.relics[1]==0&&fresh.honors==448);
    assert(sprint_scale(fresh)==0.9f&&refill_interval(fresh,1000,true)==750&&refill_interval(fresh,4000,false)==3000);
    assert(combat::ward::initialize(fresh)&&fresh.wardCharges==5&&!combat::ward::initialize(fresh));
    assert(combat::ward::damage_scale(fresh)==0.4f&&combat::ward::recovery_time(fresh,12000)==9000);
    for(int i=0;i<5;++i)assert(combat::ward::spend(fresh));assert(!combat::ward::spend(fresh));
    auto bytes=encode(fresh);Model loaded;assert(decode(bytes,loaded)&&loaded.relics==fresh.relics&&loaded.equipmentUpgrades==fresh.equipmentUpgrades&&loaded.wardReady&&loaded.wardCharges==0);
    // Exact older layouts preserve all earned benefits, spent flask charges and native provenance.
    Model legacy;legacy.migrate_growth(35);legacy.add_growth(10);legacy.charges=1;legacy.flaskShards=7;legacy.equipmentUpgrades=1;
    legacy.claim("chest:D_MN10:28",0x14);legacy.claim("chest:D_MN08:29",0x14);legacy.claim("chest:D_MN05:26",0x14);legacy.claim("bug:192",0x14);
    auto v2=encode(legacy);v2.resize(v2.size()-24);v2[3]=0x32;assert(decode(v2,loaded));auto growth=loaded.growthFragments;auto hearts=loaded.healthPoints;
    assert(migrate(loaded)&&loaded.growthFragments==growth&&loaded.healthPoints==hearts&&loaded.charges==1&&loaded.flaskShards==7&&loaded.equipmentUpgrades==1);
    assert(loaded.claimed.contains(kSunwardRelic)&&loaded.claimed.contains(kMirrorRelic)&&loaded.relics[2]==1);auto migrated=encode(loaded);assert(migrate(loaded)&&encode(loaded)==migrated);
    assert(combat::ward::initialize(loaded)&&loaded.wardCharges==4);
    // Failed migration or a full ledger cannot take resources or partially insert artifacts.
    Model full;for(int i=0;i<512;++i){auto name="full:"+std::to_string(i);assert(full.claim(name.c_str(),255));}
    auto before=encode(full);assert(!migrate(full)&&!material(full,"chest:test:1",nullptr)&&!economy::guild::apply(full,economy::guild::kRecipes[0])&&encode(full)==before);
    for(size_t i=0;i<bytes.size();++i){auto cut=bytes;cut.resize(i);Model target=loaded;assert(!decode(cut,target)&&encode(target)==encode(loaded));}
    auto corrupt=bytes;corrupt[corrupt.size()-8]=6;assert(!decode(corrupt,loaded)); // invalid ward quantity
    std::cout<<"Curated 110-fragment/20-shard budgets, finite components, all recipes, ward tiers, legacy migration and corruption safety passed\n";
}
