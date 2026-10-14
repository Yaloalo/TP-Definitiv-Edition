#pragma once
#include "services.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter_map.h"
#include "d/d_meter_button.h"
#include "JSystem/J2DGraph/J2DScreen.h"

namespace remaster::hooks {
struct EnemySound : mods::NamedHook<"Z2CreatureEnemy::startCreatureSound",
    Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8)> {
    static void* resolved_target();
};
struct ItemSelection : mods::Hook<&daAlink_c::checkNewItemChange> { static void* resolved_target(); };
struct StickData : mods::Hook<&daAlink_c::setStickData> { static void* resolved_target(); };
struct WolfMovement : mods::Hook<&daAlink_c::setSpeedAndAngleWolf> { static void* resolved_target(); };
struct MapDraw : mods::Hook<&dMeterMap_c::draw> { static void* resolved_target(); };
struct ButtonDraw : mods::Hook<&dMeterButton_c::draw> { static void* resolved_target(); };
struct ScreenDraw : mods::Hook<&J2DScreen::draw> { static void* resolved_target(); };
struct Shared0 : mods::Hook<&daAlink_c::checkItemAction> { static void* resolved_target(); };
struct Shared1 : mods::Hook<&dMeter2Draw_c::draw> { static void* resolved_target(); };
struct Shared2 : mods::Hook<&cLib_chaseF> { static void* resolved_target(); };
struct Shared3 : mods::Hook<&cLib_addCalcAngleS2> { static void* resolved_target(); };
struct Shared4 : mods::Hook<&fopAcM_createChild> { static void* resolved_target(); };
}
