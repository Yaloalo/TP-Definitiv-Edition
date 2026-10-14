# Automated QA and story regression coverage

The owner requires this workflow **after every major change and every full
build**. The default native Linux CMake build runs the `qa` target automatically,
including when compilation is already up to date. A failed check fails the
build. The current local configuration requires the exact host ABI checks.

## Commands and evidence

Requirements: the normal build tools, Python 3, `c++`, GNU binutils (`nm`,
`strip`, `c++filt`), and `pyelftools` plus `libzstd` for the Linux hook manifest
audit. The local dependencies are already available. Supply the extracted
**Dusklight v2.0.3 Linux executable**, not the AppImage wrapper:

```sh
cmake -S . -B build \
  -DDUSKLIGHT_BINARY=/absolute/path/squashfs-root/usr/bin/dusklight \
  -DTP_QA_REQUIRE_HOST=ON
cmake --build build -j 2
python3 tools/qa.py --verify
```

`build/qa/report.json` records the source fingerprint, package hash, exact tested
host hash, test names/count, per-check results and every pending story route.
`ctest.xml` provides JUnit results; `tests.log`, `imports.log`, `hooks.log` and
`strip.log` retain the detailed evidence. The gate checks the library extracted
from the actual `.dusk` against a freshly stripped compiled library. It checks
the package ID/version, archive integrity, duplicate/unsafe paths and every
packaged resource against source. Native SDK dotfile exclusions are respected.

Only complete regression/package/host success produces `verified-candidate`.
Without a host, CI can produce `tests-and-package-passed`; ABI is explicitly
not run, and this is not a verified candidate. The launcher requires a verified
report and rejects changed source, changed packages and changed tested hosts.
A report is invalidated at the start of a new QA run, so a failure cannot retain
an earlier success. Source changes during testing fail the gate.

Run targeted tests while iterating, followed by the full default build after
each major stage. A direct `--target tp_remaster` compile and a standalone
`ctest` invocation are not the full QA gate. `BUILD_TESTING=OFF` is a compile-only
build and cannot establish QA evidence. Reloading an already running game also
needs the developer to check the report before pressing Reload.

## Automated coverage

Each row is a suite, containing multiple assertions/scenarios. The fixtures
compile production callback bodies with deterministic host doubles; selected
native functions are taken from the pinned source. They do not run the complete
engine, load game assets or traverse the story.

| Suite | What it verifies |
|---|---|
| `enemy_timing` | Enemy timing arithmetic and behavior profiles |
| `enemy_death_recovery` | Native Wolf knockdown/death recovery regression |
| `manual_shield` | Guard/bash callbacks and learned-skill/mode gates |
| `launch_folder` | Integrated donor exclusions and preservation of unmanaged packages |
| `progression_save_model` | Save encoding, corruption/truncation rejection, bounds, slot values and non-farmable claims |
| `flask_callbacks` | Vulnerable healing, interruption, charge limits, quest bottles and drop filtering |
| `currency_and_pacing` | Fees/donations, native save synchronization/error branches, scoped transitions and mode isolation |
| `resources_and_relic_models` | Recharge clocks, caps, slider input and permanent trade requirements |
| `wolf_resources_and_rewards` | Wolf healing, exact Snowpeak ending scopes, ammo/bait recharge, quest grants, native ownership, interrupted queues and message isolation |
| `rest_stones_and_emergency_recovery` | Ground/line checks, actor lifecycle/ID reuse, deliberate rest, threat/damage/menu gates and one-charge emergency recovery |
| `movement_hud_models` | Wrapped/clamped turns and time-based HUD/damage envelopes |
| `movement_hud_callbacks` | Actual turn/crawl/HUD/damage callbacks, pane restoration, action/menu/save gates and edge geometry |
| `epona_handling` | Native speed clamping, 22 ordinary-riding exclusion cases, reverse movement, native obstruction results, wrapped steering, nested calls and mode transitions |
| `story_progression` | All 256 item IDs through the reward resolver; preservation of keys/equipment/permanent health/native dungeon rewards; all 256 combinations of crystal/mirror bits; persistence; transformation unlock and ten safety gates |
| `qa_tooling` | Rejection of wrong versions, duplicate/unsafe entries, missing/stale resources, failed/partial reports and changed source/package/host |

`tests/required_suites.json` prevents missing or accidentally unregistered suites
from yielding a passing gate. Skipped tests and timeouts fail. Hook checks also
reject duplicate named hook records and missing/ambiguous dynamic targets.

## Runtime story routes

`tests/story_routes.json` lists fourteen routes covering Ordon through the final
battle, optional progression, and save/mode/mod compatibility. Every route is
currently **in-game-pending**. A passing fixture does not change that status.

For each route, preserve a save before the milestone. Check the normal trigger,
the permanent item/quest result, newly accessible exits/doors/warps, the next
story objective, save/reload, and death/retry. Revisit completed milestones to
ensure grants and relics cannot repeat. Include an overhaul save from the
previous candidate and a fresh file. Keep vanilla saves separate.

Especially sensitive checks are Sera's Slingshot milestone, donations/cannon
repair without currency, Fyrus's bomb bag, Zora Armor/water bombs, all crystal
and mirror rewards, the Master Sword transformation unlock, quest liquids,
Snowpeak's ending hearts, and mounted bridge/escort/finale sequences.

For Epona, compare default handling with the toggle off and all sliders at 100%:
start, stop, shallow/sharp/wrapped turns, reverse, walls, cliffs, fence jumps,
landing, mounting/dismounting, spurs, mounted targeting and bow aiming. Test
herding, horse races, King Bulblin, Telma's escort and the Zelda pursuit. Repeat
with keyboard/controller and 30/60/high presentation FPS. Native simulation
ticks retain the timing; visual/feel checks still need the host.

Record package SHA-256, host SHA-256/version, route, save slot, settings/mods,
steps, expected/observed outcome and log/video/screenshot evidence. Mark a route
passed only when those checks were actually performed; document failures with
a repeatable save and steps. Promote a candidate to a playable checkpoint only
after applicable routes and compatibility checks pass.

## Platform limits

The complete package/ABI gate currently supports Linux ELF. Native macOS builds
run the regression fixtures automatically; host ABI validation is pending.
Windows and cross-compiles produce compile/package artifacts and do not run
these native Unix fixtures. CI uploads Linux QA evidence even on failure and
does not imply runtime story validation.
