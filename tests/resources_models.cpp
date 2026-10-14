#include "resources/recharge_model.hpp"
#include "core/slider_model.hpp"
#include "economy/merchant_model.hpp"
#include <cassert>
#include <limits>
#include <iostream>
using namespace remaster;
int main() {
    using namespace resources;
    Recharge arrow;
    int count=10;
    for(int tick=0;tick<179;++tick) assert(arrow.tick(count,30,150,30)==0);
    assert(arrow.tick(count,30,150,30)==1); // 5s quiet + 1s interval
    for(int tick=0;tick<29;++tick) assert(arrow.tick(count,30,150,30)==0);
    assert(arrow.tick(count,30,150,30)==1); // one more every second
    arrow.hit();
    for(int tick=0;tick<179;++tick) assert(arrow.tick(count,30,150,30)==0);
    assert(arrow.tick(count,30,150,30)==1);
    // Full capacity does not bank burst refills, and queued shots/refills use effective counts.
    for(int tick=0;tick<1000;++tick) assert(arrow.tick(30,30,150,30)==0);
    for(int tick=0;tick<29;++tick) assert(arrow.tick(29,30,150,30)==0);
    assert(arrow.tick(29,30,150,30)==1);
    Recharge water,regular;
    for(int tick=0;tick<360;++tick) assert(water.tick(0,15,300,150)==0);
    for(int tick=0;tick<359;++tick) assert(regular.tick(0,30,240,120)==0);
    assert(regular.tick(0,30,240,120)==1);
    for(int tick=360;tick<449;++tick) assert(water.tick(0,15,300,150)==0);
    assert(water.tick(0,15,300,150)==1);
    Recharge oil;
    assert(oil.tick(2195,2200,0,1,22)==5); // clamp a 1% refill at actual capacity
    assert(ticks_for_ms(100)==3 && ticks_for_ms(2000)==60 && ticks_for_ms(-1)==0);
    assert(ticks_for_ms(std::numeric_limits<int64_t>::max())==180000);
    for(int i=0x0a;i<=0x12;++i) assert(ammo_pickup(i));
    for(int i=0x16;i<=0x1d;++i) assert(ammo_pickup(i));
    for(int i:{0x13,0x14,0x15,0x20,0x21,0x22,0x43,0x4b,0x50,0x70,0x71,0x72}) assert(!ammo_pickup(i));
    int64_t value=0;
    assert(tuning::slider_value("5000.000000",0,60000,100,value)&&value==5000);
    assert(tuning::slider_value("5070",0,60000,100,value)&&value==5100);
    assert(tuning::slider_value("999999",100,60000,100,value)&&value==60000);
    assert(tuning::slider_value("-500",100,60000,100,value)&&value==100);
    for(auto text:{"nan","inf","1000oops","","--1"}) assert(!tuning::slider_value(text,0,60000,100,value));
    progression::Model save;
    using namespace economy::merchants;
    for(const auto& relic:kRelics) {
        assert(!can_redeem(save,relic,relic.stage,relic.room,false,false));
        assert(save.claim(relic.check,relic.id) && !save.claim(relic.check,relic.id));
        assert(can_redeem(save,relic,relic.stage,relic.room,false,false));
        assert(!can_redeem(save,relic,"wrong room",relic.room,false,false));
        assert(!can_redeem(save,relic,relic.stage,relic.room+1,false,false));
        assert(!can_redeem(save,relic,relic.stage,relic.room,true,false));
        assert(!can_redeem(save,relic,relic.stage,relic.room,false,true));
        assert(save.claim(relic.trade,0xff));
        assert(!can_redeem(save,relic,relic.stage,relic.room,false,false));
    }
    progression::Model loaded; assert(progression::decode(progression::encode(save),loaded));
    for(const auto& relic:kRelics) assert(owns_relic(loaded,relic)&&redeemed(loaded,relic));
    std::cout<<"Recharge timing, damage resets, caps, slider parsing and one-off merchant rewards passed\n";
}
