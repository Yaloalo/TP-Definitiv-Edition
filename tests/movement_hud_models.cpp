#include "movement/turn_model.hpp"
#include "hud/feedback_model.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace remaster;
int main() {
    using namespace movement::responsiveness;
    assert(extend_turn(0,100,1000,2)==200);
    assert(extend_turn(0,-100,-1000,2)==-200);
    assert(extend_turn(0,100,120,5)==120);
    assert(extend_turn(100,100,1000,5)==100); // native deliberately did not turn
    assert(extend_turn(0,100,-1000,5)==100); // target changed direction
    assert(extend_turn(32700,-32700,-32000,2)==-32564); // wrap through +/-180 degrees
    assert(angle_distance(-32768,0)==32768);
    for(int before=-32000;before<32000;before+=1000) {
        const int16_t after=int16_t(before+200),target=int16_t(before+300);
        assert(extend_turn(before,after,target,5)==target);
    }
    hud::IdleEnvelope fade;
    assert(fade.update(0,true,true,5,0.8,0)==1);
    for(int i=1;i<=50;++i) assert(fade.update(i*0.1,true,true,5,0.8,0)==1);
    const float halfway=fade.update(5.2,true,true,5,0.8,0);
    assert(halfway>0&&halfway<1);
    fade.update(5.4,true,true,5,0.8,0);fade.update(5.6,true,true,5,0.8,0);
    assert(fade.update(5.8,true,true,5,0.8,0)<0.001);
    assert(fade.update(5.81,true,false,5,0.8,0)==1); // activity reveals immediately
    assert(fade.update(50,true,true,5,0.8,0)==1); // menus/background cannot bank idle fade
    assert(fade.update(49,true,true,5,0.8,0)==1); // backwards time resets safely
    hud::IdleEnvelope floor;
    for(int i=0;i<100;++i) floor.update(i*0.1,true,true,1,1,0.25);
    assert(std::abs(floor.level)<0.001&&std::abs(floor.update(10,true,true,1,1,0.25)-0.25)<0.001);
    // Presentation rate does not change the fade's duration.
    hud::IdleEnvelope slow,fast;
    for(int i=0;i<=168;++i) slow.update(double(i)/30,true,true,5,0.8,0);
    for(int i=0;i<=672;++i) fast.update(double(i)/120,true,true,5,0.8,0);
    assert(std::abs(slow.level-fast.level)<0.001);
    hud::DamagePulse pulse;
    assert(pulse.opacity(10,0.45,0.25)==0);
    pulse.hit(10);assert(pulse.opacity(10,0.45,0.25)==0.25f);
    assert(std::abs(pulse.opacity(10.225,0.45,0.25)-0.0625)<0.0001);
    assert(pulse.opacity(10.5,0.45,0.25)==0&&pulse.opacity(9,0.45,0.25)==0);
    pulse.hit(10.1);assert(pulse.opacity(10.1,0.45,0.25)==0.25f); // repeat hits do not stack opacity
    assert(pulse.opacity(10.1,0.45,1)==0.6f);
    std::cout<<"Turn wraparound/clamping, immediate HUD reveal, presentation-rate fade and capped damage pulse passed\n";
}
