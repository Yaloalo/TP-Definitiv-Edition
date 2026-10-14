#pragma once
#include "mods/api.h"
#include "mods/svc/ui.h"
class daAlink_c;
namespace remaster::healing {
ModResult install();
void tick(daAlink_c*);
void reset();
void damage_interrupted(daAlink_c*);
void save_slot_changed(uint32_t slot, bool fresh);
ModResult build_controls(UiElementHandle);
void normalize_bottles();
void refill(bool heal);
}
