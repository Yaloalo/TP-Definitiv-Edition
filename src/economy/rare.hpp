#pragma once
#include "services.hpp"
#include "economy/rare_model.hpp"
namespace remaster::economy::rare {
ModResult install();
void tick();
bool enabled();
bool ensure();
bool custom_item(uint8_t);
bool resolve(const ItemCheckInfo*,ItemCheckResolution*);
bool can_purchase(const Offer&);
bool purchase(const Offer&);
uint32_t message(uint32_t);
}
