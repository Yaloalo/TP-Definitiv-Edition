#include <fmt/format.h>

#include "combat/damage.hpp"
#include "combat/ward.hpp"
#include "progression/treasures.hpp"
#include "combat/dodge.hpp"
#include "combat/manual_shield.hpp"
#include "combat/enemy_slow_motion.hpp"
#include "combat/boss_hard_mode.hpp"
#include "combat/stamina.hpp"
#include "combat/techniques.hpp"
#include "core/difficulty.hpp"
#include "core/player.hpp"
#include "core/tuning.hpp"
#include "hud/stamina_bar.hpp"
#include "hud/idle_fade.hpp"
#include "hud/damage_feedback.hpp"
#include "movement/responsiveness.hpp"
#include "movement/crawl.hpp"
#include "movement/horse.hpp"
#include "movement/sprint.hpp"
#include "movement/quality.hpp"
#include "pacing/quality.hpp"
#include "pacing/tears.hpp"
#include "input/mouse_controls.hpp"
#include "presentation/texture_packs.hpp"
#include "presentation/controller_ui.hpp"
#include "presentation/compass_module.hpp"
#include "presentation/compass.hpp"
#include "items/unrestricted.hpp"
#include "services.hpp"
#include "progression/rewards.hpp"
#include "progression/growth.hpp"
#include "progression/pause.hpp"
#include "progression/spring.hpp"
#include "progression/collections.hpp"
#include "healing/flask.hpp"
#include "healing/rest_stones.hpp"
#include "economy/currency.hpp"
#include "economy/rare.hpp"
#include "economy/conversations.hpp"
#include "economy/dialogue.hpp"
#include "economy/merchants.hpp"
#include "resources/ammo.hpp"
#include "healing/wolf.hpp"
#include "healing/snowpeak_hearts.hpp"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_SERVICE(GameModeService, svc_game_mode);
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(SaveService, svc_save);
IMPORT_SERVICE(ItemService, svc_item);
IMPORT_SERVICE(MessageService, svc_message);
IMPORT_SERVICE(ActorService, svc_actor);

namespace remaster {
namespace {

bool g_active = false;
UiElementHandle g_statusText = 0;

// Hooks are installed once in mod_initialize and stay inert unless the game mode is active, so
// vanilla saves are never affected.
ModResult on_activated(void*, ModError*) {
    g_active = true;
    movement::horse::reset();
    progression::load();
    healing::reset();
    hud::idle_fade::reset(); hud::damage_feedback::reset(); movement::responsiveness::reset();
    healing::stones::reset();
    healing::snowpeak::reset();
    resources::ammo::reset();
    economy::merchants::reset();
    combat::ward::reset();
    combat::techniques::reset();
    mods::log::info("Definitive Overhaul activated");
    return MOD_OK;
}

ModResult on_deactivated(void*, ModError*) {
    presentation::compass::set_compass_minimap_visibility(true);
    g_active = false;
    movement::horse::reset();
    healing::reset();
    hud::idle_fade::reset(); hud::damage_feedback::reset(); movement::responsiveness::reset();
    healing::stones::reset();
    healing::snowpeak::reset();
    resources::ammo::reset();
    economy::merchants::reset();
    combat::ward::reset();
    combat::techniques::reset();
    input::mouse_controls::reset();
    combat::enemies::reset_enemy_slow_motion();
    combat::enemies::reset_boss_hard_mode();
    mods::log::info("Definitive Overhaul deactivated");
    return MOD_OK;
}

struct Installer {
    ModResult (*install)();
    const char* name;
};

constexpr Installer kInstallers[] = {
    {progression::install, "permanent rewards and save state"},
    {progression::growth::install, "shared health and stamina growth"},
    {progression::pause::install, "equipment-menu growth and upgrades"},
    {progression::spring::install, "Ordonna growth redistribution"},
    {economy::rare::install, "rare Hylian currency"},
    {progression::treasures::install, "deliberate relic and exploration rewards"},
    {combat::ward::install, "Magic Armor rechargeable ward"},
    {progression::collections::install, "bug and Poe milestone gifts"},
    {economy::dialogue::install, "economy dialogue with native quest controls"},
    {economy::conversations::install, "character equipment conversations"},
    {resources::ammo::install, "renewable ammunition"},
    {economy::merchants::install, "quest and relic merchants"},
    {healing::wolf::install, "wolf recovery"},
    {healing::snowpeak::install, "Snowpeak ending hearts"},
    {healing::install, "renewable flask"},
    {healing::stones::install, "Sacred Rest Stones"},
    {economy::install, "currency removal"},
    {player::install, "player tick"},
    {combat::damage::install, "damage"},
    {combat::stamina::install, "stamina"},
    {combat::techniques::install, "heavy attacks and guard counters"},
    {combat::dodge::install, "dodge"},
    {combat::manual_shield::install, "manual shield"},
    {combat::enemies::initialize_enemy_slow_motion, "enemy timing"},
    {combat::enemies::install_boss_hard_mode_hooks, "boss hard mode"},
    {movement::sprint::install, "sprint"},
    {movement::responsiveness::install, "turning and pivots"},
    {movement::crawl::install, "crawl speed"},
    {movement::horse::install, "Epona handling"},
    {movement::quality::install, "movement quality"},
    {pacing::quality::install, "owned pacing"},
    {pacing::tears::install, "Tears of Light"},
    {input::mouse_controls::install, "mouse controls"},
    {hud::stamina_bar::install, "stamina bar"},
    {hud::idle_fade::install, "HUD idle fade"},
    {hud::damage_feedback::install, "damage feedback"},
    {items::unrestricted::install, "unrestricted items"},
    {presentation::controller_ui::install, "controller UI"},
    {presentation::compass::install, "compass"},
};

const GameModeDesc kGameMode = {
    .struct_size = sizeof(GameModeDesc),
    .game_mode_id = kGameModeId,
    .full_name = "Definitive Overhaul",
    .save_name = "tp_remaster",
    .user_data = nullptr,
    .on_activated = on_activated,
    .on_deactivated = on_deactivated,
};

ModResult build_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    svc_ui->pane_add_section(mod_ctx, panel, "Status");
    svc_ui->pane_add_text(mod_ctx, panel, "", &g_statusText);
    svc_ui->pane_add_section(mod_ctx, panel, "Graphics");
    if (ModResult result = presentation::texture_packs::build_controls(panel); result != MOD_OK) {
        return result;
    }
    ModResult result = tuning::build_controls(panel);
    if (result == MOD_OK) result = healing::build_controls(panel);
    if (result == MOD_OK) result = economy::merchants::build_controls(panel);
    if (result == MOD_OK) result = items::unrestricted::build_controls(panel);
    if (result == MOD_OK) result = presentation::compass::build_controls(panel);
    if (result == MOD_OK) result = presentation::controller_ui::build_controls(panel);
    return result;
}

ModResult update_panel(ModContext*, void*, ModError*) {
    std::string status;
    if (g_active) {
        const combat::stamina::State st = combat::stamina::state();
        status = fmt::format("Game mode active. Difficulty tier: {} (x{:.2f}), dungeons cleared: {}. "
                             "Stamina {:.0f}/{:.0f}{}",
            difficulty::tier_name(), difficulty::multiplier(), difficulty::dungeons_cleared(), st.current,
            st.maximum, st.exhausted ? " (exhausted)" : "");
    } else {
        status = "Inactive. Choose 'Definitive Overhaul' as game mode on the title screen.";
    }
    return svc_ui->elem_set_text(mod_ctx, g_statusText, status.c_str());
}

}  // namespace

bool is_active() {
    return g_active;
}

}  // namespace remaster

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* error) {
    using namespace remaster;
    if (ModResult result = tuning::register_vars(); result != MOD_OK) {
        return mods::set_error(error, result, "failed to register config vars");
    }

    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build_panel;
    panel.update = update_panel;
    if (ModResult result = svc_ui->register_mods_panel(mod_ctx, &panel); result != MOD_OK) {
        return mods::set_error(error, result, "failed to register mods panel");
    }

    for (const Installer& installer : kInstallers) {
        if (ModResult result = installer.install(); result != MOD_OK) {
            return mods::set_error(error, result, fmt::format("failed to install {} hooks", installer.name).c_str());
        }
    }

    if (ModResult result = svc_game_mode->register_game_mode(mod_ctx, &kGameMode); result != MOD_OK) {
        return mods::set_error(error, result, "failed to register game mode");
    }
    // After a Reload from the Mods window the mode may still be selected; pick up its state.
    bool active = false;
    if (svc_game_mode->is_active(mod_ctx, kGameModeId, &active) == MOD_OK) {
        g_active = active;
        if (active) progression::load();
    }

    mods::log::info("TP Definitive Overhaul loaded");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    remaster::progression::spring::tick();
    remaster::economy::rare::tick();
    remaster::progression::collections::tick();
    remaster::progression::treasures::tick();
    remaster::economy::conversations::tick();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    // Hooks, config vars, UI and the game mode registration are released by the loader.
    remaster::g_active = false;
    remaster::hud::idle_fade::reset(); remaster::hud::damage_feedback::reset(); remaster::movement::responsiveness::reset();
    remaster::healing::stones::shutdown();
    remaster::input::mouse_controls::reset();
    remaster::presentation::controller_ui::shutdown();
    remaster::presentation::compass::shutdown();
    remaster::items::unrestricted::shutdown();
    return MOD_OK;
}

}
