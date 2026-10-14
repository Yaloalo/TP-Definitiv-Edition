# Remaining gameplay roadmap

Updated 2026-10-14, baseline 0.6.0. This compares the owner's
[design vision](DESIGN_VISION.md) with the actual
[implemented modules](INTEGRATION.md). Everything below is pending or only
partially implemented. Existing settings/helpers are a starting point, not a
claim that the larger redesign is complete. Graphics remain a separate track.

## Recommended next milestone: timed Shield Attack parries

Build on the existing R+B Shield Attack, learned-skill gate and shared stamina
cost. Add a configurable short parry window, an actual successful-contact check,
a readable success cue and a brief punish opportunity. Start with one common
sword-wielding enemy, then extend by actor type. Preserve normal guarding,
failed/late parries, unblockable attacks, training/story gates and native death
reactions. Wolf form and scripted scenes must remain isolated.

First validate timing, repeated contacts, multiple enemies, exhaustion,
damage/death, save/load and mode switching. Playtest the Forest Temple slice.
Commit/push that milestone after QA; add enemy posture and hidden-skill follow-ups
in later commits. A universal stagger response needs per-enemy work.

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
| 1 | Timed parries | Shield Attack rewards precise contact timing; one enemy first |
| 2 | Enemy posture/stagger | Recovering posture, weapon-specific impact and native stagger reactions; expand actor by actor |
| 3 | Hidden Skills rework | Helm Splitter parry follow-up, Back Slice counter, Ending Blow execution, risky Mortal Draw, Jump Strike posture break, stamina-heavy Great Spin |
| 4 | Wolf combat completeness | Appropriate attack/dodge stamina and recovery; preserve scripted abilities and required Wolf combat |
| 5 | Enemy behavior beyond donor hard mode | Variable/delayed combos, healing punishment, flanking and coordination without inflated HP |
| 6 | Combat item roles | Ball and Chain armor/posture breaking; Boomerang interrupts; Clawshot grappling; Bow weak points; tactical rechargeable bombs |
| 7 | Spinner and Dominion Rod usefulness | Off-rail movement/combat and statue interactions; new rails/puzzles require additional level/asset work |
| 8 | Complete permanent reward table | Unique item functionality, healing/stamina/armor upgrades and side-quest rewards; current replacement fragments/relics cover part of the vision |
| 9 | Finish the currency-free world | Rework price text, merchant interactions and individual quest/minigame funding flows coherently; current compatibility callbacks are a foundation |
| 10 | Shorten Ordon and repetitive Wolf pacing | Shorter mandatory chores/tutorials with character/story scenes preserved; milestone-specific event/flag patches |
| 11 | Reduce actual Tears of Light requirements | Existing collection is faster; fewer required tears needs regional completion/state changes and route testing |
| 12 | Armor identities and permanent upgrades | Expand Hero/Zora/Magic roles; stamina-based Magic Armor is already implemented |
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
