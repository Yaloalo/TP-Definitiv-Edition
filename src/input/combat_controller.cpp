#include "input/combat_controller.hpp"
#include "core/tuning.hpp"
#include "services.hpp"
#include "dolphin/pad.h"
namespace dusk { enum class ActionBinds; }

namespace remaster::input::combat_controller {
namespace {
// SDL3 public ABI. Opaque handles; never access the host's controller object layout.
int32_t (*g_instance)(uint32_t) = nullptr;
void* (*g_gamepad)(uint32_t) = nullptr;
bool (*g_button)(void*, int) = nullptr;
void* (*g_window)() = nullptr;
uint64_t (*g_flags)(void*) = nullptr;
PADButtonMapping* (*g_buttons)(uint32_t, uint32_t*) = nullptr;
PADAxisMapping* (*g_axes)(uint32_t, uint32_t*) = nullptr;
int (*g_actionButton)(dusk::ActionBinds, uint32_t) = nullptr;
template<class Fn> ModResult resolve(const char* name, Fn& fn) {
    void* address = nullptr;
    const auto result = svc_hook->resolve(mod_ctx, name, &address, nullptr);
    if (result == MOD_OK && address) fn = reinterpret_cast<Fn>(address);
    return address ? result : MOD_UNAVAILABLE;
}
}
Sample sample() {
    Sample out;
    auto* window = g_window();
    out.focused = window && (g_flags(window) & 0x200) != 0; // SDL_WINDOW_INPUT_FOCUS
    const auto id = g_instance(0);
    if (id <= 0) return out;
    auto* pad = g_gamepad(static_cast<uint32_t>(id));
    if (!pad) return out;
    out.device = static_cast<uint32_t>(id);
    const int button = static_cast<int>(tuning::get().heavyControllerButton);
    // v2.0.3 action bindings: camera, Midna, map, minimap, menu and turbo.
    // These are separate from the emulated GameCube button/axis mappings.
    for (int action = 0; action < 6; ++action)
        if (g_actionButton(static_cast<dusk::ActionBinds>(action), 0) == button) return out;
    // Respect user mappings. A button already assigned to any native control
    // must not trigger both actions. No changes to the host's mapping/presets.
    uint32_t count = 0;
    auto* buttons = g_buttons(0, &count);
    for (uint32_t i = 0; buttons && i < count; ++i)
        if (buttons[i].nativeButton == static_cast<uint32_t>(button)) return out;
    auto* axes = g_axes(0, &count);
    for (uint32_t i = 0; axes && i < count; ++i)
        if (axes[i].nativeButton == button) return out;
    out.available = true;
    out.heavy = g_button(pad, button);
    return out;
}
ModResult install() {
    auto r = resolve("aurora::input::get_instance_for_player", g_instance);
    if (r == MOD_OK) r = resolve("SDL_GetGamepadFromID", g_gamepad);
    if (r == MOD_OK) r = resolve("SDL_GetGamepadButton", g_button);
    if (r == MOD_OK) r = resolve("aurora::window::get_sdl_window", g_window);
    if (r == MOD_OK) r = resolve("SDL_GetWindowFlags", g_flags);
    if (r == MOD_OK) r = resolve("PADGetButtonMappings", g_buttons);
    if (r == MOD_OK) r = resolve("PADGetAxisMappings", g_axes);
    if (r == MOD_OK) r = resolve("dusk::getActionBindButton", g_actionButton);
    return r;
}
}
