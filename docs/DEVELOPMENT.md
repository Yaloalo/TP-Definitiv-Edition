# Incremental development and playable builds

`DESIGN_VISION.md` is the design baseline. Feature implementation and coverage
live in `INTEGRATION.md`; remaining work and sequencing are in `ROADMAP.md`.
The approved combat/AI direction and recorded owner decisions are recorded in
[COMBAT_AND_ENEMY_AI.md](COMBAT_AND_ENEMY_AI.md). The owner has approved the choices recorded there and authorized implementation.

## Build states

1. **Candidate:** source compiles and the mod packages successfully.
2. **Verified candidate:** applicable automated tests pass, all linker imports
   resolve, and all named hooks resolve uniquely against the exact host version.
3. **Playtested:** the owner verifies loading, behavior, scene transitions,
   save/reload, wolf/human forms and vanilla-mode isolation in-game.
4. **Playable checkpoint:** preserve that playtested `.dusk` separately before
   replacing the development package with the next change.

A successful compile or symbol check is never described as in-game validation.
The current consolidated build is a verified candidate; the new modules have
not yet passed the owner's in-game checks. The v0.3.0 candidate includes owned launcher behavior, the Sacred Flask, Wolf recovery, renewable supplies, permanent exploration rewards, quest equipment and unique relic trades. Preserve the prior candidate archives until a new build passes gameplay checks.

The v0.4.0 candidate adds physical rest-stone actors, broader checkpoint coverage and strict emergency flask recovery. Treat the 32 placement entries as configured sites until model loading, ground position and reachability are confirmed in-game.

The v0.5.0 candidate adds turning/pivots, crawl speed, HUD fade and damage feedback. Automated fixtures verify native crawl-rate coupling and the actual callbacks, including temporary pane-alpha restoration. They do not prove the final feel or visuals in the release host. Playtest alongside the controller UI, stamina meter, compass and curated graphics packages before marking it a playable checkpoint.

## Development loop

The owner requires automated QA **after every major change and every full
build**. The default `cmake --build build -j 2` runs the required suites declared
in `tests/required_suites.json`,
package/library/resource validation and configured host symbol checks. Inspect
`build/qa/report.json`; local builds require the pinned host. See [QA.md](QA.md)
for setup, evidence, detailed coverage and pending runtime story routes.

- Implement one mechanic or compatibility fix at a time in its module.
- Keep its balance numbers/configuration independent of other modules.
- Run targeted tests while iterating, then the full default build/QA gate after each major stage and the final changes.
- Inspect the diff, commit each major change with its validation, push to the owner's repository, and verify the remote branch. Do not rewrite pushed history.
- Playtest in the Ordon → Faron Woods → Forest Temple slice first.
- Preserve the accepted package under `build/playtested/` with a descriptive
  name and record the host version and settings that were tested.
- Continue to the next mechanic only after checking the previous change's
  interaction with movement, stamina, saving and the surrounding modules.

Zora Swim/Dark Morpheel is the approved standalone gameplay dependency. Graphics
packages remain separate. The launcher filters integrated donor packages so
features are never applied twice.

## Next implementation priorities

The current integration supplies combat controls and donor enemy behavior, not
the complete Souls-style system. With the owner's choices approved,
the next combat milestone is one complete encounter using stamina, hidden enemy
stance, timed Shield Attack and integrated learned-skill follow-ups. Shared
intent selection and encounter coordination follow through native adapters.
The flask/checkpoint resource and permanent reward modules are implemented;
their validation sequence is in `HEALING_AND_REWARDS.md`. Increased aggression alone is not a substitute
for delayed attacks, enemy coordination or the planned boss designs.

Pacing must shorten mandatory friction while preserving story milestones and
optional character interactions. Renewable resources must limit encounters
without creating farming between attempts. Reward replacements must grant
permanent capabilities; rupee removal now uses a replacement reward table and retains native quest completion flows.

Features involving new geometry, models or animations, and renderer-level HDR,
stay separately tracked until their required asset/tool support is available.
