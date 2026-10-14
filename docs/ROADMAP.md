# Remaining gameplay roadmap

Updated 2026-10-14. The committed reward/ward baseline is 0.11.0;
the 0.12.0 economy/dialogue follow-up remains in progress. This compares the owner's
[design vision](DESIGN_VISION.md) with the actual
[implemented modules](INTEGRATION.md). Status is stated per item below.
Existing settings/helpers are a starting point, not a
claim that the larger redesign is complete. Graphics remain a separate track.

## Next combat milestone: stamina, hidden stance and integrated skills

The owner approved the Souls/Elden Ring-inspired direction and shared enemy AI
architecture in [COMBAT_AND_ENEMY_AI.md](COMBAT_AND_ENEMY_AI.md). Resolve its six
approval questions before implementing the new combat systems. Finish and
validate the separate pending economy/dialogue milestone first.

Build a complete encounter against one supported melee actor: stamina decisions,
light/heavy stance pressure, timed Shield Attack, a stance break and an eligible
punish. Integrate the learned skills as parts of that loop. Keep native death,
armor, training and story reactions intact. Then add shared intent selection
and a small group director; expand through per-enemy adapters.

Validate contacts, multihits, exhaustion, break recovery, unsupported actors,
actor lifecycle, save/load and mode switching. Playtest controls and readability
before scaling up. A shared stance model does not make every native stagger or
death transition universally safe.

## Smaller, contained improvements

| Change | Current gap | Relative scope |
|---|---|---|
| Full-speed indoor movement | Turning/crawl/sprint are integrated; indoor speed restrictions are a separate change | Small to medium; check narrow passages and scripted walking |
| Horse camera/manual camera persistence | Steering/acceleration/braking exist; camera behavior is native | Medium; preserve aiming, targeting, jumps and events |
| Faster Midna interaction cancellation | Quick transformation exists; repeated interaction waiting remains | Small to medium; preserve dialogue and quest callbacks |
| Human warping | Still separate from quick transformation | Medium; respect portal unlocks, restricted areas and story transitions |
| More flexible item assignments | Unrestricted item use exists; extra item sets/Z-slot controls are separate | Medium; preserve utility bottles, controller prompts and native actions |
| Additional safe cutscene skipping | Faster text/transitions exist; expanded skipping is incomplete | Medium to large; event completion must survive every supported skip |
| Polish refill sites and economy dialogue | Rest sites and quest/relic systems exist; positions, reachability and native price text need runtime checks | Medium; finish one region at a time |

## Main systems still to implement

| Order | Change | Deliverable and dependency |
|---|---|---|
| 1 | First combat encounter | Approved stamina rules, hidden stance, timed Shield Attack and supported break/punish against one actor; pending design answers |
| 2 | Shared enemy AI | Encounter director, utility decisions, capability profiles and native adapters; prove fair one-on-one and group behavior |
| 3 | Full Hidden Skill integration | All seven learned skills have distinct combat jobs; expand stance/contact/reaction support rather than add isolated bonuses |
| 4 | Wolf combat completeness | Appropriate attack/dodge stamina and recovery; preserve scripted abilities and required Wolf combat |
| 5 | Broader enemy behavior | Expand the shared AI through actor-specific adapters; variable combos, observable healing reactions, flanking and coordination without inflated HP |
| 6 | Combat item roles | Ball and Chain armor/posture breaking; Boomerang interrupts; Clawshot grappling; Bow weak points; tactical rechargeable bombs |
| 7 | Spinner and Dominion Rod usefulness | Off-rail movement/combat and statue interactions; new rails/puzzles require additional level/asset work |
| 8 | Validate and extend permanent rewards | The 278-location curated table, finite relic trades, growth/flask placements and save migration are implemented in 0.11.0; runtime pacing and additional combat/item capabilities remain |
| 9 | Finish the rare-economy world | Rework price text, merchant interactions and individual quest/minigame funding flows coherently; rare-currency purchases and main collection/merchant conversations are implemented; native fallback dialogue and secondary services remain |
| 10 | Shorten Ordon and repetitive Wolf pacing | Shorter mandatory chores/tutorials with character/story scenes preserved; milestone-specific event/flag patches |
| 11 | Reduce actual Tears of Light requirements | Existing collection is faster; fewer required tears needs regional completion/state changes and route testing |
| 12 | Armor identities and permanent upgrades | Rechargeable Magic Armor ward and its upgrade tiers are implemented in 0.11.0, replacing currency/stamina drain; Hero/Zora roles and runtime balance remain |
| 13 | Boss redesigns | One boss per milestone; current donor changes cover only selected bosses. Full Ganondorf duel redesign comes after parry/posture systems |
| 14 | Encounter/world design | Threatening overworld encounters, useful secrets, shortcuts and exploration rewards; Cave of Ordeals and Castle should test established combat systems |

## Validation comes with each milestone

Run the [QA gate](QA.md) after every major change and full build, then create a
focused commit and push it. Preserve prior candidates and keep the story-route
catalogue current. All fourteen full runtime routes remain pending; fixtures
cannot establish that every story flag or whole-game transition works.

For the next gameplay milestone, prioritize a verified playable
Ordon → Faron → Forest Temple slice alongside parry work. Confirm flask/rest
reachability, quest equipment, bottle interactions and Epona handling before
expanding the same changes across the entire game.

## Separate graphics track and exclusions

Textures, lighting/shadows/reflections, distance, ultrawide presentation and
renderer-level HDR need their own compatibility/performance evaluation. HDR is
not established as available through this mod's current interfaces.

Keep Flurry Rush, enemy/boss HP bars, damage numbers, XP/levels, durability,
farmable progression and inflated enemy HP excluded. Preserve the original
story, music, art direction and atmosphere. Zora Swim/Dark Morpheel remains the
approved standalone gameplay exception.
