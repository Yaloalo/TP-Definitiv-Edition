# Movement responsiveness and combat presentation

Version 0.5.0 adds four modules to Definitive Overhaul. Their controls are in **F1 → Mods → TP Definitive Overhaul**. They need no standalone Snappy Turns/Dawnlight package or native gameplay setting. The Linux v2.0.3 candidate is build/test verified; gameplay and visual validation remain pending.

## Turning and pivots

Human and Wolf turning are enabled independently, each at **200%** by default. Sliders range from 100–500% in 25% steps. The module measures the native turn step and extends it toward the requested direction without overshooting; the original movement function always runs. It does not increase sprint/run top speed.

**Quick pivots** default on and can be disabled separately. Large direction reversals pivot instead of entering the native skid/turnaround; speed is halved on the pivot frame to retain a sense of momentum. Human and Wolf turn toggles also gate their respective pivots. A 100% turn setting retains the native angular step, while the independent pivot option may still affect reversals.

Only normal human/Wolf wait/move states are modified. Targeting, events/menus, airborne movement, ice, heavy boots/armor, swimming, climbing and attack states keep their native steering. Wolf wall corrections and the initial dash burst are preserved; sustained Wolf sprinting can use the new steering. Wolf reverse-facing and steep-slope pivot constraints are retained.

Implementation: `src/movement/responsiveness.*`, with angle math in `turn_model.hpp`. Adapted from the local open-source `dusklight-snappy-turns/src/mod.cpp`. The wolf movement target is shared with sprinting through `src/core/hooks.*`, with one metadata record and one original-function execution.

## Crawling

Faster crawling defaults on at **175%**, with a real speed slider ranging from 50–250% in 5% steps. The native animation rate is scaled once. Native crawl movement already depends on that rate, so it is not multiplied again. Forward/backward crawling, stick input and native collision checks remain in place.

Only normal human crawling is affected. Crawl entry/exit animations, automatic corner transitions, scripted events and Wolf movement are unchanged. Implementation: `src/movement/crawl.*`, adapting the native `getCrawlMoveAnmSpeed`/`getCrawlMoveSpeed` relationship in pinned `d_a_alink_crawl.inc`.

## Idle HUD fading

Enabled by default: after **five seconds** safely idle, fade over **0.8 seconds** to **0%** opacity. Idle delay, fade duration and minimum opacity have independent sliders. Movement, buttons, targeting, attacks, healing, rest channels or a nearby living enemy reveal the HUD immediately.

The HUD stays visible with health at or below 25%, an empty active flask, or stamina below full/exhausted. Changes to health/charges restart the idle delay. Automatic idle gestures can continue without restarting the timer. Menus, events, loading gaps and save loads reset the fade; elapsed menu time cannot hide the HUD on return.

Native HUD/minimap/button draws establish a scope. Screen and standalone-picture hooks snapshot their original pane alphas, scale only independent alpha-inheritance roots, then restore original alpha and cached colour-alpha fields. Controller layout changes happen before the fade snapshot and regain their original state afterwards. Inventory and menu screens outside the HUD scope are not faded. The custom stamina screen participates in the same scope; compass/location/chest markers and flask/rest text explicitly apply the shared opacity once.

Implementation: `src/hud/idle_fade.*` and the timing envelope in `feedback_model.hpp`, adapted from Dawnlight `src/hud_fade.cpp` and `hud_fade_state.hpp`. The shared screen/button/map hook declarations avoid duplicate records with the existing controller UI and compass.

## Damage feedback

Enabled by default: a brief dark-red edge tint at **25% maximum opacity**, fading over **0.5 seconds**. Opacity (0–60%) and duration have their own sliders. The screen centre remains clear; there is no continuous low-health pulse, blur, camera shake, slow-motion, health bar or damage number.

Feedback follows actual decreases in saved health while active gameplay runs, rather than attempted damage callbacks. Dodged/blocked attacks with no health loss and positive healing changes cannot trigger a fresh pulse. Repeated hits restart the pulse without stacking opacity. Menus, save/slot loads, scene/room changes, death and mode deactivation clear it.

Four feathered quads form a border without overlapping their corners. Bounds use the native logical viewport, including ultrawide offsets; border thickness follows viewport height. The effect draws before the native HUD, so item counters and prompts remain sharp. The native 2D graphics state is restored after drawing.

Implementation: `src/hud/damage_feedback.*`. The GX edge drawing path follows native 2D rendering and the rendering pattern in Dawnlight `integration/dusklight_edges.inc`; no donor slow-motion controller or framebuffer sampling is incorporated.

## Validation

Twelve automated tests cover the previous gameplay plus angle wraparound/clamping, nested movement calls, normal sprint speed, pivot/ice/wall/heavy/action gates, native crawl rate/movement coupling, fade timing at different presentation rates, immediate reveal, pane inheritance/restoration, menu/save isolation, actual-health-loss detection and clear-centre ultrawide edge geometry. Game imports, named hooks and dynamic targets are checked against the pinned Linux host.

In-game checks: walk/sprint/reverse as both forms; target, fight, swim and use Iron Boots; cross ice and walls and test the initial Wolf dash; crawl forwards/backwards through corners and tight passages; idle until the HUD disappears, then move/heal/rest/take damage; inspect the stamina bar, controller prompts, compass and minimap together; open inventory/map/Mods windows while faded; save/load/reload/switch modes; confirm edge tint visibility and restored HUD rendering at 4:3, widescreen and ultrawide. Test with the curated graphics packages. These checks have not yet been performed for this candidate.

## Epona handling (0.6.0)

**Responsive Epona handling** defaults on, with steering **150%**, acceleration
**140%** and braking **150%**. Each slider spans 100–300% in 5% steps. These are
our mod's persisted controls; no standalone Epona mod or native setting is needed.
Disable the toggle, or set all three sliders to 100%, to retain native handling.

The native horse movement function always runs. During that call only,
`cLib_chaseF` adjusts rates when its destination is this horse's forward speed.
The native chase function clamps to its target, retaining speed limits and
preventing stop overshoot. Steering extends the actual native angular step
toward the stick direction, clamping at the target and respecting angle wrap.
No shared horse HIO constants or persistent native speed limits are changed.

Only ordinary mounted wait/move states qualify. Scripted events/demo modes,
rodeo and autonomous call movement, bridge-specific flags, mounted aiming,
targeting, hanging, cow/collision reactions, airborne movement and jump/landing/
damage/turn/stop states retain native handling. Early obstacle return codes,
wall corrections and reverse steering are preserved. Spurs, maximum speed,
mount animations and horse camera behavior retain their native implementation.

Implementation: owned callbacks in `src/movement/horse.*`, reusing our existing
angle-step helper. The linear chase hook has one shared metadata owner with
enemy timing; the horse callback checks the exact speed pointer and call scope.

The fifteen-suite QA gate now runs after every full native build. Epona fixtures
cover native speed clamps, state gates, wrap, nested calls and mode isolation.
See [QA.md](QA.md) for the full automated matrix and pending runtime story and
horse checks. This candidate has not yet been playtested in-game.
