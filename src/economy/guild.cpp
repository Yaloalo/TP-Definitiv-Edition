#include "economy/guild.hpp"
#include "economy/merchants.hpp"
#include "progression/treasures.hpp"
#include "progression/rewards.hpp"
#include "core/player.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/actor/d_a_alink.h"
#include "resources/recharge_model.hpp"
#include <cstring>
namespace remaster::economy::guild {
namespace {
bool equipment(const Recipe& r) {
    using namespace progression::treasures;
    if(r.honor)return true;
    if(r.benefit==kScoutStitch)return dComIfGs_isCollectCrystal(0);
    if(r.benefit==kFletching)return dComIfGs_isItemFirstBit(dItemNo_BOW_e);
    if(r.benefit==kSunward||r.benefit==kMirrorWard)return dComIfGs_isCollectClothes(2);
    for(int i=0;i<3;++i) {
        const auto item=dComIfGs_getItem(SLOT_15+i,false);
        if(resources::bomb_type(item)||item==dItemNo_BOMB_BAG_LV1_e)return true;
    }
    return false;
}
}
bool can_craft(const Recipe& r) {
    auto* link=daAlink_getAlinkActorClass();
    return merchants::enabled()&&progression::ready()&&link&&player::gameplay_running()&&
        std::strcmp(dComIfGp_getStartStageName(),r.stage)==0&&fopAcM_GetRoomNo(link)==r.room&&
        equipment(r)&&eligible(progression::model(),r);
}
bool craft(const Recipe& r) {
    if(!progression::treasures::ensure()||!can_craft(r)||!apply(progression::model(),r))return false;
    progression::persist();
    UiToastDesc notice=UI_TOAST_DESC_INIT;notice.title_rml=r.honor?"A lasting honor":"Artisan's work completed";
    notice.body_rml=r.label;svc_ui->push_toast(mod_ctx,&notice);
    return true;
}
}
