#pragma once
#include "mods/api.h"
#include "mods/svc/ui.h"
#include "mods/svc/item.h"
class daAlink_c;
namespace remaster::economy::merchants {
ModResult install();
ModResult build_controls(UiElementHandle);
void tick();
void reset();
bool enabled();
bool custom_item(uint8_t);
bool resolve(const ItemCheckInfo*, ItemCheckResolution*);
void on_give(const ItemGiveInfo*);
uint32_t message(daAlink_c*, uint32_t nativeId);
}
