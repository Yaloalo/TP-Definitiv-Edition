# Sacred Flask, checkpoints and permanent rewards

Implemented in the Definitive Overhaul game mode, version 0.4.0. This is a build/test verified candidate; the new gameplay and stone placements still need in-game validation against Dusklight v2.0.3.

## Balance

- Start with three shared charges. Each charge heals three hearts (12 native quarter-heart units).
- Four Flask Shards add one charge, up to eight with default settings. Twenty specific former rupee chests supply the five capacity upgrades.
- Every two cleared dungeons add one heart to each charge's healing, up to seven hearts after eight dungeons.
- A charge is spent when drinking starts. Healing arrives at the native animation's drinking point. Damage interrupts the action; an interrupted windup loses its charge.
- Drinking uses the original bottle model/animation without starting the compulsory event or event camera. It grants no invulnerability and enemies continue acting.
- The flask occupies one bottle slot and uses the original red bottle icon, renamed Sacred Flask. Assign it to X or Y. Inventory counts and the HUD show charges.
- Human drinking requires Link on foot and out of swimming mode. Dismount or stand with Iron Boots to drink.
- Wolf Link uses **R + X** to sit and recover from the same flask charges. Default windup is two seconds, followed by 0.6 seconds of recovery. Movement/attacks are locked, enemies remain active, and damage interrupts healing without refunding the charge. Start on solid ground outside scripted actions. The windup has its own timing slider. Ordinary howling-stone interactions remain native.

Starting charges, capacity cap, base healing and enable/disable controls live in F1 → Mods → TP Definitive Overhaul. Config settings are independent of saved progression.

## Refills

Full health, flask charges and owned ammunition/supplies return at safe rest points, on a genuine death/retry and after boss victory. Boss completion hooks cover native indexed and current-stage boss flags; collecting boss rewards/cleared-dungeon progression supplies fallbacks.

**Sacred Rest Stones replace automatic room-entry refills.** Stand beside the visible stone and hold **L + R** for three uninterrupted seconds. By default, rest also requires ten seconds without damage, solid ground, an idle action and no living enemy within 3,000 game units. Movement, attacks, damage, events, opening a menu or releasing the buttons interrupt the rest. No invulnerability or enemy respawn/reset is added. Human and Wolf Link can both rest.

The Mods panel's Rest button requests the same three-second channel at a nearby stone; close the menu to begin. It does not refill instantly or work elsewhere. Proximity requires the loaded stone model, matching room, distance at most 260 units, height difference at most 100 units and a clear line of sight.

There are **32 configured sites**: the previous five peaceful rooms/locations and nine dungeon entrances, plus eighteen additions. New coverage includes Ordon Village, Coro's clearing, North Faron Woods, Sacred Grove, Death Mountain, Kakariko Village, Castle Town, Upper Zora's River, Iza's lodge, Zora's Domain approach, Lake Hylia, Gerudo Desert, Snowpeak, Hidden Village, a Hyrule Field shelter, and refuges inside Forest Temple, Goron Mines and Lakebed Temple. Boss arenas and the Cave of Ordeals have no stones.

Stones reuse the original **Sacred Meadow howling stone model (`WStoneF`, model 4)** at a smaller scale, through our own actor. They have no howling quest/event behavior and are nonblocking visual landmarks. Original howling stones are untouched. A short nearby HUD caption explains the rest control; world-map/minimap stone icons are not implemented.

The placement table is `src/healing/rest_sites.hpp`. Interior/entrance sites read fixed native PLYR spawn coordinates; outdoor/refuge anchors come from the local disc-derived TP_Tracker position catalogue. The placement code tries conservative offsets and checks floor height, water and walls. It never anchors a stone to Link's current position. Sites spawn on visiting their room, are tracked to prevent duplicates, and are cleaned up on mode/slot changes or feature disable. Unsafe or unavailable placements are skipped and retried rather than permitting an invisible refill. Automated checks do not establish that all 32 placements look correct or are reachable in-game.

Native entrance coordinates, transcribed from the community entrance catalogue:

| Stage | Room |
|---|---:|
| Forest Temple `D_MN05` | 22 |
| Goron Mines `D_MN04` | 1 |
| Lakebed Temple `D_MN01` | 0 |
| Arbiter's Grounds `D_MN10` | 0 |
| Snowpeak Ruins `D_MN11` | 0 |
| Temple of Time `D_MN06` | 0 |
| City in the Sky `D_MN07` | 0 |
| Palace of Twilight `D_MN08` | 0 |
| Hyrule Castle `D_MN09` | 11 |

## Emergency flask recovery

**Enabled by default**, following the owner's clarification. This restores **one charge only when the flask is empty**; it never fills the entire flask, heals directly or refills ammunition.

- Only outdoor/overworld stages (`F_`), while actual gameplay is running.
- First remain clear of damage and all living enemies within 5,000 units for **90 seconds**. Movement is allowed during this initial period.
- Then remain still on solid ground in an idle action for **30 additional seconds**. Movement, attacks or healing interrupt this final period.
- Damage or nearby enemies restart the safety delay. Menus/events earn no progress and interrupt the final period. Scene/room changes, save loads and slot changes reset recovery timing.
- Dungeons, boss stages and the Cave of Ordeals cannot grant emergency recovery.

This is a slow safeguard against becoming stranded with no healing. Full recovery comes from rest stones, boss victories or genuine death/retry. The Mods panel provides an independent toggle and timing sliders: 10–300 seconds for the safety delay and 5–300 seconds for the final rest, in one-second steps. Stone channel and no-hit-delay sliders remain independent.

## Containers and healing items

The first unused/empty bottle slot, searching from slot 14 backwards, becomes the flask. Existing water, oil, soup and quest contents are never overwritten to claim a slot. If all four bottles contain quest fluids, empty one to make room. The mod normalizes ordinary potions, milk, fairy and healing Chu jelly bottles to empty utility bottles; soup/hot-spring quest actions retain their native behavior, with positive bottle healing suppressed. Ordinary hearts and healing fairy spawns/grants are removed. Heart Pieces and Heart Containers still grant permanent health upgrades.

**Snowpeak exception:** Yeta and Yeto's post-boss ending retains its original heart bursts and their normal healing. Only hearts emitted by the exact ending/embrace cut steps are exempt; ordinary Snowpeak enemy and container hearts remain suppressed. Actor IDs retain the exception until collection, including after the cut returns. Disabling the flask does not remove this exception.

Three utility bottle slots remain available. A fourth utility bottle reward currently has no additional capacity effect; bottle acquisition does not upgrade the flask. Native shop shelves use sold-out signs when the quest/relic economy is enabled. Quest gifts and utility-fluid actions remain available. Existing shop dialogue can still refer to the original prices/potions. Reload the room after changing these toggles to refresh shop models. Dialogue presentation cleanup remains separate work.

## Renewable ammunition and supplies

Disposable arrows, seeds and all three bomb-type packs are removed. Named, permanent checks containing ammunition become permanent stamina rewards; loose repeatable drops cannot award upgrades. Equipment, bomb-bag/type unlocks, keys, heart upgrades and puzzle actors are preserved.

Every owned resource has two real sliders in **F1 → Mods → TP Definitive Overhaul**: delay since taking damage and interval between refills. Values use 0.1-second steps; delays can be 0–60 seconds and intervals 0.1–60 seconds. Defaults:

| Resource | No-hit delay | Refill interval | Amount |
|---|---:|---:|---:|
| Arrows | 5 s | 1 s | 1 |
| Slingshot seeds | 3 s | 1 s | 1 |
| Regular bombs | 8 s | 4 s | 1 per bag |
| Water bombs | 10 s | 5 s | 1 per bag |
| Bomblings | 10 s | 6 s | 1 per bag |
| Lantern oil | 3 s | 1 s | 1% of lantern capacity |
| Bee bait | 3 s | 2 s | 1 per bottle |

The first refill arrives after the delay **plus** one interval; arrows start after six seconds with defaults, then add one each second. Any damage resets all clocks. Menus, cutscenes and offline time earn no refills. Avoiding damage during active combat permits regeneration, as requested; this is not an out-of-combat detector.

Native capacities and permanent capacity upgrades are respected, including already queued spending/refills. The mod preserves a bomb bag's type when its last bomb is used and a bee bottle's type when its last bait is consumed. Zero-count supplies cannot be used until regenerated. Existing vanilla empty bags have lost their original type, so migration restarts those bags with regular bombs. Nothing refills equipment that Link does not own.

## Currency and rewards

Currency removal is independently configurable and defaults on. Disposable coin drops are rejected at the spawn/field-actor entry points. Persistent coin checks resolve to permanent rewards before their native collection flags are consumed. Coin grants/balance changes are suppressed and the rupee HUD is hidden without hiding small keys.

There is no buying equipment or supplies in the default economy. Native purchase selection is disabled; essential gear is earned through quests and unique exploration relics unlock specific permanent upgrades. Relics are finite named artifacts, not an interchangeable balance.

| Essential equipment | Requirement |
|---|---|
| Slingshot | Help Sera's cat and receive her gift |
| First bomb bag | Defeat Fyrus |
| Water bombs | Own a bomb bag and acquire Zora Armor |
| Bomblings | Own a bomb bag and clear Lakebed Temple |

Quest rewards are queued once and recorded only after the host confirms delivery. Saving/loading before delivery keeps them eligible. Existing equipment is respected.

| Unique relic | Exploration check | Permanent reward | Merchant |
|---|---|---|---|
| Goron Forge Relic | Kakariko watchtower alcove (`chest:F_SP109:20`) | Hylian Shield | Kakariko Malo Mart |
| Surveyor's Lens | Watchtower chest (`chest:R_SP109:17`) | Hawkeye | Kakariko Malo Mart |
| Deepwater Craft Relic | Statue outside Lanayru Spring (`chest:F_SP115:6`) | Larger bomb capacities: 60 / 30 / 20 | Barnes |
| Twilight Sigil | Sacred Grove Spinner chest (`chest:F_SP117:2`) | Magic Armor | Castle Town Malo Mart |

**Trade interface in this candidate:** visit the matching shop room, stand still outside an event, then open F1 → Mods and press its relic upgrade button. Each trade requires its corresponding collected relic and can be completed once. Ownership and redemption persist per save slot. Native merchant dialogue has not yet been rewritten to present these trades; it can still mention prices. Relics currently reuse native acquisition models/icons and English messages in all supported language variants.

Minigame and story funding gates retain their native quest flow without a payment requirement. Donation input supplies full funding with one confirmation; original callbacks handle completion flags. Hena's direct fee checks receive a scoped virtual affordability value, never real saved currency. These are compatibility bridges, not a full rewrite of donation quest design or dialogue.

Magic Armor no longer uses currency or becomes heavy from an empty wallet. With available, non-exhausted stamina it halves incoming damage and costs 20 stamina per hit. With exhausted/disabled stamina it provides no damage reduction. Native host armor settings should stay at their default during validation to avoid unrelated native modifiers.

The twenty Flask Shard placements are in `src/progression/rewards.cpp`; the four relic placements in `src/economy/merchant_model.hpp` are disjoint. Other persistent rupee/ammunition chests/checks grant Stamina Fragments: four give +2 maximum stamina, up to +100. Unattributed/repeatable drops and shop previews never grant upgrades. Named rewards grant progress once per save slot, so repeated minigame rewards cannot be farmed.

## Persistence and verification

The SaveService stores a versioned byte blob per Definitive Overhaul save slot. It holds spent charges, bottle assignment, shards, fragments, cleared-dungeon milestones and the claimed-check ledger, including quest rewards, acquired relics and redeemed upgrades. The blob format remains compatible with 0.2.0. The mod updates the blob on each resource/progression change; the host writes it alongside successful game saves. Loading a saved game restores its saved charges. Only a genuine death sets the retry refill flag, scoped to the same slot. Ammunition quantities use native saves; elapsed recharge clocks are reset on load. Invalid progress blobs are rejected and left untouched.

Ten automated tests execute production healing/drop, Wolf recovery, Snowpeak, ammunition, slider/config, merchant reward, rest/recovery and currency/pacing callbacks with fixtures, plus the pinned native save-wait function. New checks cover deliberate rest channels, damage/movement/menu interruption, one-charge-only overworld recovery, scene isolation, physical proximity/height/line of sight, floor/water/wall validation, stable spawn anchors and deferred actor deletion/foreign-ID protection. Import, named-hook and dynamic-target checks validate compatibility with the pinned host, not runtime patching or gameplay.

In-game validation remains: visit all stone sites, check visibility/scale/access and ground placement; rest as human and Wolf, interrupt the channel, move out of range, try behind a wall and with nearby enemies; confirm no refill simply from entering a room; empty the flask and check the 90+30-second fallback, interruptions and dungeon exclusion. Also test human/Wolf healing under attack, supplies, death/retry, save/load/slot changes, shards/relics/quest rewards, slider persistence, water-bomb type retention, Snowpeak ending hearts, utility bottles, story funding gates, Magic Armor and vanilla mode. Confirm HUD positioning across heart rows and ultrawide settings.

The real timing sliders use the pinned host's RmlUi range controls through exported GetId/GetValue/SetValue methods. Their narrow signatures were checked against upstream [Element](https://github.com/mikke89/RmlUi/blob/master/Include/RmlUi/Core/Element.h), [input controls](https://github.com/mikke89/RmlUi/blob/master/Source/Core/Elements/ElementFormControlInput.cpp) and [range implementation](https://github.com/mikke89/RmlUi/blob/master/Source/Core/Elements/InputTypeRange.cpp); no RmlUi object layouts or native config objects are accessed. The current ABI verification covers the Linux x86_64 v2.0.3 release.
