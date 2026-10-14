# Project workflow

The owner requires the automated QA suite after every major change and every
full build. Keep gameplay changes in this mod's modules and F1 controls. Do not
reintroduce dependence on launcher/native gameplay presets. Zora Swim/Dark
Morpheel is the approved standalone gameplay exception. Flurry Rush is excluded.

Use `cmake --build build -j 2` for the default build, which runs the QA target.
Run that full gate after each major implementation stage, before moving on to
the next stage, and after the final changes. Inspect `build/qa/report.json` and
the logs. Fix failed checks before calling the build verified. A direct library
target or standalone `ctest` does not run all package/host checks.

For the local Linux host, configure `DUSKLIGHT_BINARY` to an extracted Dusklight
v2.0.3 executable and `TP_QA_REQUIRE_HOST=ON`. See `docs/QA.md` for commands and
coverage. Keep the native source/headers pinned to the runtime release.

Add meaningful regression tests for new mechanics and story-sensitive changes.
Use production callbacks/native functions in fixtures where feasible. Update
`tests/required_suites.json` when adding a required suite and the story-route
catalogue when progression coverage changes. Preserve previous candidates.

Automated fixtures and symbol checks are not complete story playthroughs.
Record runtime routes as pending until actually played, with host/package hash,
save slot, settings, expected/observed behavior and evidence. Continue authorized
implementation while runtime checks are unavailable; describe the remaining
validation clearly. Never claim all story flags or compatibility are proven by
mock fixtures. Do not overwrite the separate vanilla saves.
