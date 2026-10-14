#include "progression/model.hpp"
#include <cassert>
#include <iostream>
using namespace remaster::progression;
int main() {
    Model m; m.refill(3,8);
    assert(m.charges == 3 && m.potency(3) == 3);
    assert(m.spend(3,8) && m.spend(3,8) && m.spend(3,8) && !m.spend(3,8));
    assert(!m.claim(nullptr,0x13) && !m.claim("",0x13));
    for (int i=0;i<20;i++) {
        std::string name="chest:"+std::to_string(i);
        assert(m.claim(name.c_str(),0x13)); assert(!m.claim(name.c_str(),0x13));
    }
    assert(m.flaskShards == 20 && m.capacity(3,8)==8 && m.charges==0);
    m.refill(3,8); assert(m.charges==8); assert(m.spend(3,8));
    m.bosses=6; assert(m.potency(3)==6); m.bottleSlot=14;
    for(int i=0;i<12;i++) { auto name="npc:"+std::to_string(i); m.claim(name.c_str(),0x14); }
    assert(m.stamina_bonus()==6);
    auto bytes=encode(m); Model loaded; assert(decode(bytes,loaded));
    assert(loaded.charges==7 && loaded.flaskShards==20 && loaded.staminaFragments==12 && loaded.bottleSlot==14 && loaded.claimed==m.claimed);
    assert(loaded.capacity(3,5)==5); assert(loaded.spend(3,5) && loaded.charges==4);
    for(size_t i=0;i<bytes.size();i++) {
        auto truncated=bytes; truncated.resize(i);
        Model unchanged=loaded;
        assert(!decode(truncated,unchanged) && unchanged.charges==loaded.charges && unchanged.claimed==loaded.claimed);
    }
    auto corrupt=bytes; corrupt[0]^=0xff; assert(!decode(corrupt,loaded));
    corrupt=bytes; corrupt.push_back(0); assert(!decode(corrupt,loaded));
    corrupt=bytes; corrupt[24]=255; corrupt[25]=255; assert(!decode(corrupt,loaded));
    corrupt=bytes; corrupt[20]=10; assert(!decode(corrupt,loaded)); // invalid bottle slot
    Model slots[3]; slots[0]=m; slots[1].refill(3,8); slots[1].spend(3,8);
    assert(slots[0].charges==7 && slots[1].charges==2 && slots[2].flaskShards==0);
    // Migrate the actual old byte layout; do not infer version from file length.
    auto old=encode(m);old.resize(old.size()-24);old[0]=0x54;old[1]=0x44;old[2]=0x46;old[3]=0x31;
    Model legacy;assert(decode(old,legacy)&&!legacy.growthReady&&legacy.stamina_bonus()==6);
    legacy.migrate_growth(37);assert(legacy.healthPoints==4&&legacy.staminaPoints==1&&legacy.growthFragments==27);
    assert(legacy.stamina_bonus()>=6&&legacy.native_max_life()==37&&legacy.charges==7&&legacy.flaskShards==20);
    legacy.migrate_growth(15);assert(legacy.healthPoints==4); // migration is idempotent
    Model growth;growth.migrate_growth(15);assert(!growth.allocate(false)&&!growth.exchange(true));
    for(int i=0;i<5;++i){auto name="growth:"+std::to_string(i);assert(growth.claim(name.c_str(),0x14));}
    assert(growth.unassigned()==1&&growth.allocate(true)&&growth.stamina_bonus()==20&&growth.unassigned()==0);
    auto total=growth.growthFragments;assert(growth.exchange(false)&&growth.healthPoints==1&&growth.staminaPoints==0&&growth.growthFragments==total);
    for(int i=0;i<1000;++i){assert(growth.exchange(true));assert(growth.exchange(false));assert(growth.growthFragments==total&&growth.unassigned()==0);}
    growth.add_growth(1000);assert(growth.growthFragments==110);
    while(growth.allocate(true)){};while(growth.allocate(false)){};
    assert(growth.healthPoints==17&&growth.staminaPoints==5&&growth.unassigned()==0&&growth.native_max_life()==100);
    assert(!growth.allocate(false)&&!growth.exchange(true)&&!growth.exchange(false));
    Model complete;assert(decode(encode(growth),complete)&&complete.healthPoints==17&&complete.staminaPoints==5);
    std::cout << "Progression, reward deduplication, charge persistence and corrupt-save rejection passed\n";
}
