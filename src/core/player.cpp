#include "core/player.hpp"
#include "core/hooks.hpp"

#include "combat/dodge.hpp"
#include "combat/ward.hpp"
#include "combat/stamina.hpp"
#include "d/actor/d_a_alink.h"
#include "movement/sprint.hpp"
#include "movement/traversal.hpp"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_object.h"
#include "services.hpp"
#include "healing/flask.hpp"
#include "resources/ammo.hpp"
#include "economy/merchants.hpp"
#include "hud/idle_fade.hpp"
#include "hud/damage_feedback.hpp"
#include "progression/growth.hpp"

namespace remaster::player {
namespace {

using LinkExecute=hooks::LinkExecute;

void on_execute_post(ModContext*, void* args, void*, void*) {
    if (!is_active()) {
        return;
    }
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr) {
        return;
    }
    progression::growth::tick();
    healing::tick(link);
    resources::ammo::tick(link);
    combat::ward::tick(link);
    economy::merchants::tick();
    // Drains first, so stamina::tick sees this tick's spending before it regenerates.
    movement::sprint::tick(link);
    movement::traversal::tick(link);
    combat::stamina::tick(link);
    combat::dodge::tick(link);
    hud::idle_fade::tick(link);
    hud::damage_feedback::tick(link);
}

}  // namespace

ModResult install() {
    return mods::hook::add_post<LinkExecute>(on_execute_post);
}

bool input_allowed() {
    bool visible = true;
    return gameplay_running() && svc_ui->is_any_document_visible(mod_ctx, &visible) == MOD_OK && !visible;
}

bool gameplay_running() {
    return dMeter2Info_getWindowStatus() == 0 && dMeter2Info_getPauseStatus() == 0 && !dComIfGp_isPauseFlag() &&
           !dComIfGp_event_runCheck() && !dMeter2Info_isShopTalkFlag() && !dMsgObject_isTalkNowCheck();
}

}  // namespace remaster::player
