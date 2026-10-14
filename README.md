# TP Definitive Overhaul

A [Dusklight](https://github.com/TwilitRealm/dusklight) mod that turns Twilight Princess into a darker, harder,
more fluid game:

- Souls-inspired combat
- Breath of the Wild–inspired movement
- faster pacing
- progression without farming

**Docs:**

- Scope: [`../DESIGN_VISION.md`](../DESIGN_VISION.md)
- Feasibility and roadmap: [`../REMASTER_PLAN.md`](../REMASTER_PLAN.md)
- Integrated modules, source ownership and validation: [`docs/INTEGRATION.md`](docs/INTEGRATION.md)

Everything runs inside its own **game mode** ("Definitive Overhaul" on the title screen) with a separate save file.
Vanilla saves are never touched.

## Status

Combat core and the selected open-source gameplay modules are integrated.
The owner has confirmed the stamina bar in-game. New manual-shield, enemy/boss,
item, compass, controller UI, owned pacing/camera, flask, Wolf recovery, supplies and reward changes still need in-game validation.
Compiles against v2.0.3; linker imports and named hook targets are checked against
the release binary. See `docs/INTEGRATION.md` for coverage and tests.

| Feature | Notes |
|---|---|
| Game mode + separate save | "Definitive Overhaul" on the title screen; all hooks are inert outside it |
| Enemy damage scaling | default 200 %, multiplied by the difficulty curve |
| Difficulty curve | see below |
| Shorter post-hit invincibility | default 40 %; knockdowns keep the full duration |
| Stamina | roll, side roll, sidestep/backflip, sword swing, spin, jump attack, helm splitter, back slice, ending blow, shield attack. Souls rules: any stamina > 0 lets you act; running dry exhausts you until 30 % |
| Stamina bar | game's own meter under the hearts, green / orange when exhausted, fades out when full |
| Dodge i-frames | first 8 ticks (≈0.27 s) of rolls and sidesteps; vanilla dodges have none |
| Sprint | hold A after a roll (human) or during the wolf dash; drains 20 stamina/s; B while sprinting = jump slash. Ported from Roll-and-Sprint and Dawnlight |
| Traversal stamina | climbing and hanging drain 6/s while moving; fast swimming drains 15/s; running dry only exhausts (no falling) |
| Turning and crawling | 200% human/Wolf turn-step speed, optional quick pivots and 175% crawl speed; independently configurable |
| HUD and damage feedback | safe idle HUD fade after 5 s; immediate activity reveal; brief 25% red edge tint on actual health loss |
| Balance controls | every value is adjustable in the Mods window |
| Manual shield | hold R without targeting; R+B bashes after learning Shield Attack; uses the shared stamina gate |
| Enemy/Boss Hard Mode | Dawnlight source ports, independently configurable; donor coverage documented in `docs/INTEGRATION.md` |
| Unrestricted Items | integrated, with its optional interior/first-person/clawshot settings |
| Twilight Compass | integrated, configurable compass/location/chest markers; optional minimap and mirrored orientation |
| Modern controller UI | integrated PlayStation/Xbox/Switch artwork and configuration editor |
| Owned pacing and camera | dialogue, transitions, save waits, tears collection, climbing, quick transform, mouse camera/aim and reticle use our controls; no launcher preset |
| Sacred Flask | three starting charges, three hearts per charge; vulnerable human drinking or Wolf R+X recovery; 32 configured visible rest-stone sites (hold L+R), death/boss refills and exploration shards; strict one-charge emergency recovery enabled by default |
| Renewable supplies | arrows, seeds, three bomb types, lantern oil and bee bait; separate no-hit-delay and refill-interval sliders; disposable ammunition drops removed |
| Permanent rewards / no currency | ordinary heart/coin/fairy drops removed; Snowpeak ending heart shower preserved; checks become flask/stamina upgrades; quest equipment and four unique relic trades replace shopping; stamina-based Magic Armor |

**Difficulty curve** (`src/core/difficulty.cpp`):

- **Overworld:** scales with the number of dungeons cleared (0.5× → 1.8×).
- **Dungeons:** each dungeon has its own tier, from Forest Temple 0.75× up to Hyrule Castle 2.0× and Cave of
  Ordeals 2.5×.
- The higher of the two values applies.

**Not in milestone 1:**

- wolf dodge i-frames and wolf attack costs
- tired animation when exhausted
- parry and posture

See [`docs/HEALING_AND_REWARDS.md`](docs/HEALING_AND_REWARDS.md) for flask balance, checkpoint/reward placement, save behavior and current restrictions.
See [`docs/MOVEMENT_AND_HUD.md`](docs/MOVEMENT_AND_HUD.md) for turning, crawl controls, HUD fade, damage feedback and pending in-game checks.

## Build

Requirements: CMake ≥ 3.26, Ninja, a C++20 compiler, git. The first configure downloads the Dusklight v2.0.3
headers into `./dusklight`.

```sh
cmake -B build -G Ninja
cmake --build build
# → build/mods/tp_remaster.dusk
```

The default native build also runs the automated QA suite. For the complete
Linux gate, set `-DDUSKLIGHT_BINARY=/absolute/path/to/extracted/dusklight`
and `-DTP_QA_REQUIRE_HOST=ON` when configuring. Reports and logs are under
`build/qa/`. The launcher rejects packages without current complete QA evidence.
See [the QA coverage and story checks](docs/QA.md); runtime story checks remain
pending until actually played. Run the full gate after every major change.

`DUSKLIGHT_VERSION` in `CMakeLists.txt` **must match the Dusklight release you play on.**

Check that every imported game symbol exists in your Dusklight binary:

```sh
~/Games/Dusklight/Dusklight.AppImage --appimage-extract   # once, in a scratch directory
tools/check_symbols.sh build/tp_remaster.so squashfs-root/usr/bin/dusklight
```

## Test

```sh
tools/play.sh                     # start menu → pick "Definitive Overhaul" on the ‹ PLAY › selector
```

`play.sh` opens Dusklight's start menu with the disc from `config.json` (`backend.isoPath`). Passing
`--dvd <image>`, `--load-save` or `--stage` skips that menu and boots the *vanilla* game mode, so the overhaul
stays inactive.

`play.sh` uses our mod's gameplay controls without built-in `--cvar` overrides. It uses the Definitive
Edition mod folder (`docs/MODPACK.md`) to prepare `build/runtime-mods`, excluding
standalone copies of the integrated modules. Zora Swim/Dark Morpheel stays standalone.

Automated checks:

```sh
ctest --test-dir build --output-on-failure
```

**Steps:**

1. On the start menu, switch **‹ PLAY ›** with the touchpad or A/D until it reads **Definitive Overhaul**.
2. Start a new file. The mode has its own saves.
3. **F1** opens the Dusklight menu; the *Mods* tab holds this mod's panel (status, texture pack, balance).
4. After a rebuild, press **Reload** there.
5. With *Log damage and stamina events* on, the log (`~/.local/share/TwilitRealm/Dusklight/logs/`) records every
   hit and stamina change.

**First-session checklist.** Note what fails or feels off:

1. **Mod loads.** The Mods window lists *TP Definitive Overhaul* without an error, and the game mode appears on the
   title screen.
2. **Stamina bar.** It appears under the hearts after the first roll or swing and fades out when full. Check its
   position with the lantern lit.
3. **Gating.** Spam rolls until the bar runs dry (orange): A must no longer roll and B must no longer swing until
   the bar is back at 30 %.
4. **No softlocks.** Hold B to charge a spin attack while running dry. Link must not get stuck in the charge pose.
   Landing rolls after jumps must still work without costing stamina.
5. **Dodge i-frames.** Roll through a Bokoblin or Deku Baba attack: no damage during the start of the roll.
6. **Damage.** Ordon and Faron give vanilla damage (0.5× curve × 200 % = 1×); the Forest Temple gives 1.5×. Check
   the log for `damage a -> b`.
7. **Invincibility after hits.** Consecutive hits can land sooner than in vanilla. After a knockdown, Link still
   gets up safely.
8. **Vanilla untouched.** Load a normal save: no bar, vanilla damage.

## Controls (laptop, keyboard + touchpad)

Dusklight ships no default keyboard bindings. `tools/keyboard_layout.py` writes this layout to
`~/.local/share/TwilitRealm/Dusklight/keyboard_bindings.dat` and keeps a `.bak` of the previous file. Keys are
physical positions, so the layout is identical on QWERTZ.

| Key | GameCube | Function |
|---|---|---|
| W A S D | control stick | move (always full speed; keys cannot walk slowly) |
| Arrow keys | C-stick | camera |
| Space | A | action / roll / sidestep |
| J | B | sword |
| K / L | X / Y | items |
| Left Shift (hold) | L | Z-target |
| Q | R | shield / crouch |
| E | Z | Midna |
| Enter | Start | pause menu |
| 1 / 2 / 3 / 4 | D-pad ↑ ↓ ← → | D-pad |

**Touchpad:** the Definitive Overhaul uses its own mouse/touchpad aiming and camera controls (F1 → Mods). The separate vanilla launcher may still use native settings. Dusklight menus can be clicked with the
touchpad. Libinput's disable-while-typing may ignore the touchpad while keys are held; the arrow keys always work
for the camera.

**Other keys:** F11 fullscreen, Tab turbo, Ctrl+R reset (if enabled).

To rebind, use Dusklight's controller settings (they overwrite the file), or edit `LAYOUT_BUTTONS` /
`LAYOUT_AXES` in the script and rerun it with Dusklight closed.

**Launchers (rofi drun):**

- **Dusklight:** vanilla game with the disc preset, fullscreen and touchpad aiming/camera.
- **TP Definitive Overhaul:** `tools/play.sh` with the local `build/runtime-mods` folder and fullscreen; gameplay controls belong to the mod.

The disc image is `~/Games/Dusklight/disc/TP_GZ2P01_Europe.rvz` (GameCube PAL).

## Layout

```
src/mod.cpp            lifecycle, service imports, game mode, mods panel
src/services.hpp       shared service declarations
src/core/tuning.*      config vars (persisted) + UI controls
src/core/difficulty.*  progress/dungeon-based difficulty multiplier
src/core/player.*      per-tick Link hook, "gameplay running" check
src/combat/damage.*    incoming damage scaling, post-hit invincibility
src/combat/stamina.*   stamina model, input gating, action costs
src/combat/dodge.*     dodge invincibility windows
src/movement/sprint.*  human + wolf sprint (ports of Roll-and-Sprint / Dawnlight wolf_sprint)
src/movement/traversal.*  climbing / swimming stamina drain
src/healing/*         human flask, Wolf recovery, safe refills, Snowpeak ending exception
src/resources/*       damage-delayed ammunition/supply regeneration and timing model
src/progression/*     permanent reward replacement and save ledger
src/economy/*         currency suppression, quest equipment and unique relic upgrades
src/hud/stamina_bar.*  stamina meter drawing
src/presentation/texture_packs.*  in-game texture pack switcher (Mods window → Graphics)
tools/check_symbols.sh ABI check against a Dusklight binary
tools/play.sh          launch with build output + owned gameplay controls
tools/keyboard_layout.py  write the laptop keyboard layout
tools/texture_pack.sh  switch the active texture pack from the shell
docs/MODPACK.md        curated graphics mods and texture packs
```

## Credits

Code sources (see `../REMASTER_PLAN.md` §3 for the full list): Dawnlight, A Link Between Twilight, the Dusklight
Randomizer and further community mods in `../reference-mods/`.

Ported files name their source in a header comment. The project is private and not distributed.
