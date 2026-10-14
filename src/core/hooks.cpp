#include "core/hooks.hpp"
namespace remaster::hooks {
DEFINE_HOOK_SYMBOL("Z2CreatureEnemy::startCreatureSound",
    Z2SoundHandlePool*(Z2CreatureEnemy*, JAISoundID, u32, s8), EnemySoundMetadata);
void* EnemySound::resolved_target() { return EnemySoundMetadata::resolved_target(); }
DEFINE_HOOK(&daAlink_c::checkNewItemChange, ItemSelectionMetadata);
void* ItemSelection::resolved_target() { return ItemSelectionMetadata::resolved_target(); }
DEFINE_HOOK(&daAlink_c::setStickData, StickDataMetadata);
void* StickData::resolved_target() { return StickDataMetadata::resolved_target(); }
DEFINE_HOOK(&daAlink_c::setSpeedAndAngleWolf, WolfMovementMetadata);
void* WolfMovement::resolved_target() { return WolfMovementMetadata::resolved_target(); }
DEFINE_HOOK(&dMeterMap_c::draw, MapDrawMetadata);
void* MapDraw::resolved_target() { return MapDrawMetadata::resolved_target(); }
DEFINE_HOOK(&dMeterButton_c::draw, ButtonDrawMetadata);
void* ButtonDraw::resolved_target() { return ButtonDrawMetadata::resolved_target(); }
DEFINE_HOOK(&J2DScreen::draw, ScreenDrawMetadata);
void* ScreenDraw::resolved_target() { return ScreenDrawMetadata::resolved_target(); }
DEFINE_HOOK(&daAlink_c::checkItemAction, Shared0Metadata);
void* Shared0::resolved_target() { return Shared0Metadata::resolved_target(); }
DEFINE_HOOK(&dMeter2Draw_c::draw, Shared1Metadata);
void* Shared1::resolved_target() { return Shared1Metadata::resolved_target(); }
DEFINE_HOOK(&cLib_chaseF, Shared2Metadata);
void* Shared2::resolved_target() { return Shared2Metadata::resolved_target(); }
DEFINE_HOOK(&cLib_addCalcAngleS2, Shared3Metadata);
void* Shared3::resolved_target() { return Shared3Metadata::resolved_target(); }
DEFINE_HOOK(&fopAcM_createChild, Shared4Metadata);
void* Shared4::resolved_target() { return Shared4Metadata::resolved_target(); }
}
