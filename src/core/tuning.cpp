#include "core/tuning.hpp"

#include "services.hpp"
#include "core/slider_model.hpp"
#include <string>
#include <fmt/format.h>

namespace remaster::tuning {
namespace {

Values g_values;

// One entry per config var, in panel order. Exactly one of intTarget/boolTarget is set.
// A non-null `section` starts a new heading in the Mods panel.
struct VarSpec {
    const char* section;
    const char* name;
    const char* label;
    int64_t* intTarget;
    bool* boolTarget;
    int64_t min, max, step;
    const char* suffix;
    ConfigVarHandle handle;
    bool slider = false;
    UiElementHandle sliderLabel = 0;
    int64_t lastSliderValue = -1;
};

#define INT_VAR(section, field, label, min, max, step, suffix) \
    {section, #field, label, &g_values.field, nullptr, min, max, step, suffix, 0}
#define SLIDER_VAR(section, field, label, min) {section, #field, label, &g_values.field, nullptr, min, 60000, 100, " ms", 0, true}
#define LONG_SLIDER_VAR(section, field, label, min) {section, #field, label, &g_values.field, nullptr, min, 300000, 1000, " ms", 0, true}
#define PERCENT_SLIDER(section, field, label, min, max, step) {section, #field, label, &g_values.field, nullptr, min, max, step, "%", 0, true}
#define BOOL_VAR(section, field, label) {section, #field, label, nullptr, &g_values.field, 0, 0, 0, nullptr, 0}

VarSpec g_specs[] = {
    INT_VAR("Damage", enemyDamagePercent, "Enemy damage", 25, 1000, 25, "%"),
    INT_VAR(nullptr, postHitInvincibilityPercent, "Invincibility after normal hits", 0, 100, 10, "%"),
    BOOL_VAR(nullptr, difficultyCurve, "Difficulty grows with progress"),

    BOOL_VAR("Stamina", staminaEnabled, "Stamina system"),
    INT_VAR(nullptr, staminaMax, "Maximum stamina", 25, 500, 5, nullptr),
    INT_VAR(nullptr, staminaRegenPerSecond, "Regeneration per second", 5, 200, 5, nullptr),
    INT_VAR(nullptr, staminaRegenDelayMs, "Regeneration delay", 0, 3000, 100, " ms"),
    INT_VAR(nullptr, exhaustionRecoverPercent, "Exhaustion ends at", 0, 100, 5, "%"),
    INT_VAR(nullptr, costRoll, "Cost: roll", 0, 100, 1, nullptr),
    INT_VAR(nullptr, costSidestep, "Cost: sidestep / backflip", 0, 100, 1, nullptr),
    INT_VAR(nullptr, costSwing, "Cost: sword swing", 0, 100, 1, nullptr),
    INT_VAR(nullptr, costSpin, "Cost: spin attack", 0, 100, 1, nullptr),
    INT_VAR(nullptr, costTechnique, "Cost: jump attack / hidden skills", 0, 100, 1, nullptr),
    INT_VAR(nullptr, costShieldAttack, "Cost: shield attack", 0, 100, 1, nullptr),

    BOOL_VAR("Movement", sprintEnabled, "Sprint (hold A after a roll / during wolf dash)"),
    INT_VAR(nullptr, sprintSpeedPercent, "Sprint speed", 80, 200, 5, "%"),
    INT_VAR(nullptr, sprintCostPerSecond, "Sprint stamina per second", 0, 100, 1, nullptr),
    INT_VAR(nullptr, climbCostPerSecond, "Climbing stamina per second", 0, 100, 1, nullptr),
    INT_VAR(nullptr, swimDashCostPerSecond, "Fast swimming stamina per second", 0, 100, 1, nullptr),
    BOOL_VAR(nullptr, fastClimbing, "Faster climbing and ladders"),
    BOOL_VAR(nullptr, noMissClimbing, "Remove random climbing slip animations"),
    BOOL_VAR(nullptr, quickTransform, "Quick transform (R + Y, after unlocking)"),
    BOOL_VAR("Movement responsiveness", humanTurning, "Faster human turning"),
    PERCENT_SLIDER(nullptr, humanTurnSpeedPercent, "Human turn speed", 100, 500, 25),
    BOOL_VAR(nullptr, wolfTurning, "Faster Wolf turning"),
    PERCENT_SLIDER(nullptr, wolfTurnSpeedPercent, "Wolf turn speed", 100, 500, 25),
    BOOL_VAR(nullptr, quickPivots, "Quick pivots on direction reversal"),
    BOOL_VAR(nullptr, fastCrawling, "Faster crawling"),
    PERCENT_SLIDER(nullptr, crawlSpeedPercent, "Crawl speed", 50, 250, 5),
    BOOL_VAR("Epona", horseHandling, "Responsive Epona handling"),
    PERCENT_SLIDER(nullptr, horseTurnSpeedPercent, "Epona steering response", 100, 300, 5),
    PERCENT_SLIDER(nullptr, horseAccelerationPercent, "Epona acceleration", 100, 300, 5),
    PERCENT_SLIDER(nullptr, horseBrakingPercent, "Epona braking", 100, 300, 5),
    BOOL_VAR("HUD and damage feedback", idleHudFade, "Fade HUD while safely idle"),
    SLIDER_VAR(nullptr, hudIdleDelayMs, "Idle time before fading", 500),
    SLIDER_VAR(nullptr, hudFadeDurationMs, "Fade duration", 100),
    PERCENT_SLIDER(nullptr, hudIdleOpacityPercent, "HUD opacity while idle", 0, 75, 5),
    BOOL_VAR(nullptr, damageFeedback, "Subtle red edges on damage"),
    PERCENT_SLIDER(nullptr, damageFeedbackOpacityPercent, "Damage feedback opacity", 0, 60, 5),
    SLIDER_VAR(nullptr, damageFeedbackDurationMs, "Damage feedback duration", 100),

    BOOL_VAR("Pacing", instantText, "Hold B for instant dialogue text"),
    BOOL_VAR(nullptr, fastTransitions, "Faster scene transitions"),
    BOOL_VAR(nullptr, instantSaves, "Remove artificial save waiting"),
    BOOL_VAR(nullptr, fastTears, "Faster Tears of Light collection"),
    BOOL_VAR(nullptr, skipRupeeCutscenes, "Skip rupee pickup explanations"),
    BOOL_VAR(nullptr, noRupeeReturns, "Keep rupee chest collected when wallet is full"),

    BOOL_VAR("Camera and aiming", aimingReticle, "Bow and slingshot aiming reticle"),
    BOOL_VAR(nullptr, mouseAim, "Mouse / touchpad aiming"),
    BOOL_VAR(nullptr, mouseCamera, "Mouse / touchpad camera"),
    BOOL_VAR(nullptr, invertMouseY, "Invert mouse / touchpad vertical axis"),
    INT_VAR(nullptr, mouseAimSensitivityPercent, "Mouse aiming sensitivity", 10, 300, 10, "%"),
    INT_VAR(nullptr, mouseCameraSensitivityPercent, "Mouse camera sensitivity", 10, 300, 10, "%"),

    INT_VAR("Dodge", dodgeInvincibilityTicks, "Dodge invincibility", 0, 30, 1, " ticks (1/30 s)"),

    BOOL_VAR("Combat depth", manualShield, "Manual shield (hold R; R + B to bash)"),
    BOOL_VAR(nullptr, combatTechniques, "Heavy attacks, guard counters and learned Great Spin"),
    BOOL_VAR(nullptr, hiddenStance, "Hidden stance and timed parries (Bokoblins)"),
    INT_VAR(nullptr, bokoblinStance, "Sturdy Bokoblin stance (ordinary: 60%)", 25, 150, 5, nullptr),
    INT_VAR(nullptr, parryWindowTicks, "Shield Attack parry window", 2, 10, 1, " ticks (1/30 s)"),
    INT_VAR(nullptr, heavyControllerButton, "Heavy button: 7=L3, 8=R3, 9=LB, 10=RB (must be unassigned)", 7, 10, 1, nullptr),
    INT_VAR(nullptr, costHeavy, "Cost: heavy sword attack", 1, 100, 1, nullptr),
    INT_VAR(nullptr, costGuardCounter, "Cost: guard counter", 1, 100, 1, nullptr),
    INT_VAR(nullptr, costBlock, "Cost: blocked impact", 1, 100, 1, nullptr),
    INT_VAR(nullptr, costGreatSpin, "Cost: Great Spin", 1, 100, 1, nullptr),
    BOOL_VAR(nullptr, enemyHardMode, "Dawnlight Enemy Hard Mode"),
    BOOL_VAR(nullptr, bossHardMode, "Dawnlight Boss Hard Mode"),

    BOOL_VAR("Items and presentation", unrestrictedItems, "Unrestricted Items"),
    BOOL_VAR(nullptr, controllerUi, "Modern controller UI"),
    BOOL_VAR(nullptr, compassMirrorMode, "Mirror compass (match Dusklight Mirror Mode)"),

    BOOL_VAR("Healing and economy", flaskEnabled, "Sacred Flask (renewable bottle healing)"),
    BOOL_VAR(nullptr, removeHeartDrops, "Remove disposable heart drops"),
    BOOL_VAR(nullptr, removeCurrency, "Overhauled economy: rare treasure and permanent purchases"),
    INT_VAR(nullptr, flaskStartingCharges, "Starting flask charges", 1, 8, 1, nullptr),
    INT_VAR(nullptr, flaskMaximumCharges, "Maximum flask charges", 1, 12, 1, nullptr),
    INT_VAR(nullptr, flaskHealingHearts, "Starting healing per charge", 1, 10, 1, " hearts"),
    BOOL_VAR("Sacred Rest Stones", restStones, "Visible refill stones (hold L + R nearby)"),
    SLIDER_VAR(nullptr, restNoHitDelayMs, "Time without damage before resting", 0),
    SLIDER_VAR(nullptr, restChannelMs, "Uninterrupted time to rest", 500),
    BOOL_VAR("Emergency flask recovery", passiveFlaskRecovery, "Recover one charge when empty (safe overworld only)"),
    LONG_SLIDER_VAR(nullptr, passiveFlaskDelayMs, "Time clear of enemies and damage", 10000),
    LONG_SLIDER_VAR(nullptr, passiveFlaskRestMs, "Additional time standing still", 5000),

    BOOL_VAR("Resource regeneration", ammoRecharge, "Regenerate owned ammunition and supplies"),
    BOOL_VAR(nullptr, removeConsumableDrops, "Remove arrows, seeds and bomb supply drops"),
    BOOL_VAR("Magic Armor", magicWard, "Rechargeable ward protection (no currency drain)"),
    SLIDER_VAR(nullptr, wardRecoveryDelayMs, "Safe time before ward recovery", 1000),
    SLIDER_VAR(nullptr, wardRecoveryIntervalMs, "Time per ward charge", 1000),
    BOOL_VAR("Merchants", economyDialogue, "Rewritten economy and minigame dialogue"),
    BOOL_VAR(nullptr, merchantRewards, "Earn equipment from quests and unique relics"),
    SLIDER_VAR("Arrows", arrowRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, arrowRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Slingshot seeds", seedRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, seedRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Regular bombs", bombRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, bombRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Water bombs", waterBombRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, waterBombRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Bomblings", bomblingRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, bomblingRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Lantern oil (1% per refill)", oilRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, oilRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Bait (bees)", baitRechargeDelayMs, "Delay after taking damage", 0),
    SLIDER_VAR(nullptr, baitRechargeIntervalMs, "Time per refill", 100),
    SLIDER_VAR("Wolf recovery", wolfHealingWindupMs, "Vulnerable healing windup (R + X)", 500),

    BOOL_VAR("Debug", debugLog, "Log damage and stamina events"),
};

#undef SLIDER_VAR
#undef LONG_SLIDER_VAR
#undef PERCENT_SLIDER
#undef INT_VAR
#undef BOOL_VAR

// RmlUi's real range widgets, bound to our ConfigService handles. The pinned host exports these
// narrow methods; no RmlUi object layouts or host config objects are accessed.
DEFINE_HOOK_SYMBOL("Rml::ElementFormControlInput::OnUpdate", void(void*), SliderUpdate);
const std::string& (*g_sliderId)(const void*) = nullptr;
std::string (*g_sliderGet)(const void*) = nullptr;
void (*g_sliderSet)(void*, const std::string&) = nullptr;
void slider_after(ModContext*, void* args, void*, void*) {
    const auto* input = mods::arg<void*>(args, 0);
    if (!input || !g_sliderId) return;
    const auto& id = g_sliderId(input);
    constexpr std::string_view prefix = "tp-definitive-timing-";
    if (!std::string_view(id).starts_with(prefix)) return;
    const auto name = std::string_view(id).substr(prefix.size());
    for (auto& spec : g_specs) {
        if (!spec.slider || name != spec.name) continue;
        int64_t value;
        if (!slider_value(g_sliderGet(input), spec.min, spec.max, spec.step, value)) return;
        if (value != spec.lastSliderValue) {
            if (svc_config->set_int(mod_ctx, spec.handle, value) != MOD_OK) return;
            spec.lastSliderValue = value;
        } else if (*spec.intTarget != value) {
            // External/reset changes update the thumb without recreating an input being dragged.
            g_sliderSet(const_cast<void*>(input), std::to_string(*spec.intTarget));
            spec.lastSliderValue = *spec.intTarget;
        }
        if (spec.sliderLabel) {
            const auto label = std::string_view(spec.suffix)=="%" ? fmt::format("{}: {}%",spec.label,*spec.intTarget) :
                fmt::format("{}: {:.1f} s", spec.label, double(*spec.intTarget) / 1000.0);
            svc_ui->elem_set_text(mod_ctx, spec.sliderLabel, label.c_str());
        }
        return;
    }
}
template<class Fn>
ModResult resolve_slider(const char* name, Fn& function) {
    void* address = nullptr;
    auto result = svc_hook->resolve(mod_ctx, name, &address, nullptr);
    if (result != MOD_OK || !address) return MOD_UNAVAILABLE;
    function = reinterpret_cast<Fn>(address); return MOD_OK;
}
ModResult install_sliders() {
    auto r = resolve_slider("Rml::Element::GetId[abi:cxx11]", g_sliderId);
    if (r == MOD_OK) r = resolve_slider("Rml::ElementFormControlInput::GetValue[abi:cxx11]", g_sliderGet);
    if (r == MOD_OK) r = resolve_slider("Rml::ElementFormControlInput::SetValue", g_sliderSet);
    if (r == MOD_OK) r = mods::hook::add_post<SliderUpdate>(slider_after);
    UiStyleHandle style = 0;
    if (r == MOD_OK) r = svc_ui->register_styles(mod_ctx, UI_SCOPE_WINDOW,
        ".tp-timing-slider { display: block; width: 95%; height: 28dp; margin: 6dp 0; }"
        ".tp-timing-slider slidertrack { width: 100%; height: 8dp; background-color: #47534b; margin-top: 9dp; }"
        ".tp-timing-slider sliderbar { width: 16dp; height: 26dp; background-color: #e7cf93; border: 1dp #fff0c8; border-radius: 3dp; }"
        ".tp-timing-slider sliderbar:hover { background-color: #fff0c8; }", &style);
    return r;
}

void on_changed(ModContext*, ConfigVarHandle, const ConfigVarValue* value, const ConfigVarValue*, void* userData) {
    auto* spec = static_cast<VarSpec*>(userData);
    if (spec->intTarget != nullptr) {
        *spec->intTarget = value->int_value;
    } else {
        *spec->boolTarget = value->bool_value;
    }
}

}  // namespace

const Values& get() {
    return g_values;
}

ModResult register_vars() {
    for (VarSpec& spec : g_specs) {
        ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
        desc.name = spec.name;
        if (spec.intTarget != nullptr) {
            desc.type = CONFIG_VAR_INT;
            desc.default_int = *spec.intTarget;
        } else {
            desc.type = CONFIG_VAR_BOOL;
            desc.default_bool = *spec.boolTarget;
        }
        ModResult result = svc_config->register_var(mod_ctx, &desc, &spec.handle);
        if (result != MOD_OK) {
            return result;
        }
        // Values from config.json are applied at registration without a callback; read them now.
        if (spec.intTarget != nullptr) {
            svc_config->get_int(mod_ctx, spec.handle, spec.intTarget);
        } else {
            svc_config->get_bool(mod_ctx, spec.handle, spec.boolTarget);
        }
        result = svc_config->subscribe(mod_ctx, spec.handle, on_changed, &spec, nullptr);
        if (result != MOD_OK) {
            return result;
        }
    }
    return install_sliders();
}

ModResult build_controls(UiElementHandle pane) {
    for (VarSpec& spec : g_specs) {
        if (spec.section != nullptr) {
            if (ModResult result = svc_ui->pane_add_section(mod_ctx, pane, spec.section); result != MOD_OK) {
                return result;
            }
        }
        if (spec.slider) {
            const auto label = std::string_view(spec.suffix)=="%" ? fmt::format("{}: {}%",spec.label,*spec.intTarget) :
                fmt::format("{}: {:.1f} s", spec.label, double(*spec.intTarget) / 1000.0);
            ModResult r = svc_ui->pane_add_text(mod_ctx, pane, label.c_str(), &spec.sliderLabel);
            const auto markup = fmt::format("<input type=\"range\" class=\"tp-timing-slider\" id=\"tp-definitive-timing-{}\" min=\"{}\" max=\"{}\" step=\"{}\" value=\"{}\" />", spec.name, spec.min, spec.max, spec.step, *spec.intTarget);
            spec.lastSliderValue = *spec.intTarget;
            if (r == MOD_OK) r = svc_ui->pane_add_rml(mod_ctx, pane, markup.c_str(), nullptr);
            if (r != MOD_OK) return r;
            continue;
        }
        UiControlDesc control = UI_CONTROL_DESC_INIT;
        control.label = spec.label;
        control.binding = UI_BINDING_CONFIG_VAR;
        control.config_var = spec.handle;
        if (spec.intTarget != nullptr) {
            control.kind = UI_CONTROL_NUMBER;
            control.min = spec.min;
            control.max = spec.max;
            control.step = spec.step;
            control.suffix = spec.suffix;
        } else {
            control.kind = UI_CONTROL_TOGGLE;
        }
        ModResult result = svc_ui->pane_add_control(mod_ctx, pane, &control, nullptr);
        if (result != MOD_OK) {
            return result;
        }
    }
    return MOD_OK;
}

}  // namespace remaster::tuning
