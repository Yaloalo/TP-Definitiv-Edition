# Gameplay integration

Updated 2026-10-14. This is the owner's personal project; source commits are pushed to the configured repository, while game files and generated builds remain local.

The overhaul is one source project and one `.dusk`, with separate modules and
controls. We combine our own stamina, damage, difficulty and save-mode systems
with adapted community source. This means we can inspect, edit and rebuild the
gameplay; it does not mean all the implementation was written from scratch.

| Feature | Implementation | State |
|---|---|---|
| Game mode and separate save | `src/mod.cpp` | Existing |
| Stamina, dodge i-frames, damage and difficulty curve | `src/combat`, `src/core` | Existing |
| Human sprint | Roll-and-Sprint donor; `src/movement/sprint.cpp` | Integrated, stamina cost |
| Wolf sprint, climbing/swimming stamina | Dawnlight donor plus our stamina model | Integrated |
| Human/Wolf turning and quick pivots | Snappy Turns source adaptation; `src/movement/responsiveness.*` | Integrated; independent sliders, shared Wolf sprint hook |
| Faster crawling | Native animation/movement adaptation; `src/movement/crawl.*` | Integrated; 175% default speed slider |
| Responsive Epona handling | Owned scoped callbacks; `src/movement/horse.*` | Integrated; steering/acceleration/braking sliders, ordinary riding only |
| Idle HUD fade | Dawnlight source adaptation; `src/hud/idle_fade.*` | Integrated; gameplay HUD only, immediate activity reveal |
| Subtle damage feedback | Owned pulse and native GX edge rendering; `src/hud/damage_feedback.*` | Integrated; actual health loss, clear centre |
| Manual shield | Dawnlight/lazytweaks donors; `src/combat/manual_shield.*` | Integrated; R guards without targeting; R+B bashes after learning Shield Attack |
| Enemy Hard Mode | Dawnlight donor; `src/combat/enemy_hard_mode.*` and typed actor profiles | Integrated; 28 profiles, shorter attack waits, faster turning/chasing, item/attack variations; no HP scaling |
| Boss Hard Mode | Dawnlight donor; `src/combat/boss_hard_mode.*` | Integrated; Ook's boomerang, Diababa, Dangoro, Fyrus and Death Sword; this is the donor's coverage, not every boss |
| Unrestricted Items | `src/items/unrestricted.*` | Integrated with native fallbacks outside our mode |
| Modern controller UI | TP Classic Modern Controller UI donor; `src/presentation/controller_ui.*` | Integrated, including PlayStation/Xbox/Switch artwork and its editor |
| Twilight Compass | `src/presentation/compass*` | Integrated; fixed reserved config name; mirror orientation has an explicit toggle |
| Zora Swim + Dark Morpheel | Closed-source gameplay package | Kept standalone, by owner decision |
| TPTracker | Randomizer tracker | Excluded by owner decision; donor integration does not require the tracker |
| Flurry Rush | Owner decision, 2026-10-14 | Excluded; no Flurry Rush hooks or controls |
| Enemy/boss HP bars and damage numbers | Owner decision | Excluded |

Twilit Essentials itself is not installed or incorporated as a binary. Selected
features with open-source equivalents use those donors. Remaining behavior
rebuilds are separate work: expanded cutscene
skipping, human warping, Midna cancellation, sheathed spin and Epona controls/
camera. Former launcher preset behavior is now owned by our movement, pacing and input modules. `tools/play.sh` passes no built-in gameplay cvars.

Version 0.2.0 added owned movement/pacing/camera controls, the Sacred Flask, permanent flask/stamina rewards and currency removal. Version 0.3.0 adds shared-charge Wolf recovery, the original Snowpeak ending heart shower exception, adjustable ammunition/supply regeneration, ammunition drop removal, quest equipment rewards and four non-farmable relic trades. Shopping is disabled in the default economy. These modules are our code using native engine/SDK interfaces; they require no other gameplay mod settings. Details and limitations are in [HEALING_AND_REWARDS.md](HEALING_AND_REWARDS.md). These are verified candidates awaiting in-game checks.

Version 0.4.0 replaces room-entry rest with 32 configured Sacred Rest Stone sites and a deliberate L+R channel. The existing howling stone model is reused by our custom ActorService profile without native howling/events. Strict emergency recovery grants one charge when empty in safe overworld conditions, enabled by default by owner request. Bottle behavior remains unchanged; save blob format stays compatible. Actual model placement and interaction still need gameplay checks.

Version 0.5.0 adds human/Wolf turning and quick pivots, faster crawl speed, idle HUD fading and subtle damage feedback. Version 0.6.0 adds scoped Epona steering/acceleration/braking controls and a required default-build QA gate. Controls/defaults, source provenance, scope and pending playtests are in [MOVEMENT_AND_HUD.md](MOVEMENT_AND_HUD.md); the automated/story coverage matrix is in [QA.md](QA.md).

## Module boundaries

- `src/mod.cpp` owns lifecycle, service imports, mode registration and installer order.
- Every gameplay callback checks our mode before changing behavior. The controller
  editor remains available for configuration; its game callbacks are mode-scoped.
- Shared hook targets are declared once in `src/core/hooks.*`; modules register
  callbacks on the same target rather than emitting competing records.
- Stamina owns the shield-attack gate and cost. Manual shielding routes R+B
  through that gate; it cannot bypass exhaustion.
- Enemy profiles preserve damage/death timing and exclude scripted states.
  Their retained donor timing helpers always use speed 1; no combat slow-motion
  feature is enabled.
- Settings and controller resources are namespaced to avoid donor collisions.
- Source files keep donor provenance. Reference repos remain available locally.

## Launching without duplicate gameplay mods

`tools/play.sh` prepares `build/runtime-mods/` from the curated folder. It links
the current overhaul, Zora Swim and the graphics packages, and excludes the
standalone Unrestricted Items, Compass, controller UI, Roll-and-Sprint and Snappy Turns
packages because their features are integrated. It does not alter the curated
source folder. Only generated links listed in `.managed-links.json` are replaced.

## Validation

```sh
cmake --build build
ctest --test-dir build --output-on-failure
tools/check_symbols.sh build/tp_remaster.so /path/to/dusklight
python3 tools/check_hook_symbols.py build/tp_remaster.so /path/to/dusklight \
    --dynamic-source src/input/mouse_controls.cpp --dynamic-source src/core/tuning.cpp
```

The first symbol check covers linker imports. The second checks compiled named
hook metadata against the host's embedded manifest and rejects duplicates. It
requires `pyelftools` and `libzstd` and currently supports 64-bit little-endian ELF.
The 0.6.0 candidate passes fifteen suites and resolves 571 game imports, 26 named hook targets and 11 dynamic mouse/RmlUi targets against Linux x86_64 Dusklight v2.0.3. The default native build runs these checks plus a packaged-library/resource audit; current evidence and pending story routes are recorded in `build/qa/report.json`. See [QA.md](QA.md) for the detailed matrix and platform limits.

Tests execute production manual-shield callbacks with game-free fixtures,
reproduce the enemy Wolf knockdown/death timer race using the pinned game code,
check actor-scope/timer math, test flask/Wolf healing, Snowpeak heart provenance, resource clocks/queued counts/empty types, slider config updates, delayed quest grants, non-farmable relic trades, permanent reward/save validation, free story funding gates, native save sync and scoped transitions, and verify launch folder filtering and preservation
of source/unmanaged files. These checks do not replace in-game validation.

In-game checks still needed for the new modules: guard while untargeted and
targeted; release R; test learned R+B with full/exhausted stamina; test wolf and
vanilla modes; fight multiple enemies through knockdown, Wolf takedown and death;
test the five supported boss changes; compare item use, compass/minimap controls
and controller prompts in both modes; reload the mod and switch modes.
