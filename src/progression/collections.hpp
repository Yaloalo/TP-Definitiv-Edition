#pragma once
#include "services.hpp"
namespace remaster::progression::collections {
ModResult install();
bool enabled();
bool ensure();
int pairs();
int souls();
int unpresented();
bool present_bugs();
bool claim_rewards(bool bugs);
bool rewards_available(bool bugs);
bool resolve(const ItemCheckInfo*,ItemCheckResolution*);
void tick();
}
