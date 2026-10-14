# Shared growth and rare-economy milestones

Owner-approved 2026-10-14. Hawkeye is granted with the Bow, including recovery
for existing saves. Lantern fuel stays renewable; there is no fuel purchase or
lantern upgrade in this redesign.

## Shared growth (0.7.0)

Five Growth Fragments form one blessing. Each blessing adds one heart or twenty
stamina. The three starting hearts and base stamina are protected; earned
allocations cap at seventeen additional hearts and five stamina upgrades.
Flask Shards and equipment upgrades remain separate.

Native Heart Pieces contribute one fragment; native Heart Containers contribute
five. Their original item-first and stage-life completion flags remain native.
Named grants are recorded once; native untagged grants use the pending health
counter fallback. No ordinary dropped healing heart becomes a growth fragment.

On the actual tunic/equipment pause screen, a summary shows health, stamina and
flask charges. **Z** opens Growth & Equipment. **A** assigns an unspent blessing
to health; **X** assigns it to stamina; **B/Z** returns. The sheet also shows
fragments, capacity/potency, bomb capacities, equipment and acquired relics.
This is separate from the F1 settings panel. Submenus, host windows and other
game modes cannot receive the sheet's inputs. Menu appearance/layout still
needs a runtime check alongside the controller UI and ultrawide presentation.

The `progression-v1` blob key now accepts explicit TDF1/TDF2 byte versions.
TDF1 migration preserves native heart/partial-heart progress, claimed checks,
flask shards/charges, bottle assignment and quest/equipment ownership. Existing
small stamina bonuses round up to a twenty-point allocation; they never round
down. Corrupt versions/fields remain rejected without overwriting their blob.
Changing health allocation cannot heal Link. No elapsed time is saved.

## Ordonna's Blessing (0.8.0)

After collecting Forest Temple's crystal, a one-time passive notice introduces
free redistribution at the existing rest stone in **Ordon Spring (F_SP104,
room 1)**. Stand safely beside the stone, then **hold L and press Y**. The
conversation offers one earned heart for twenty stamina or the reverse.
Starting stats, total earned blessings and fragments never change. Lowering
maximum health clamps current health; raising it does not heal.

The interaction uses the same stone visibility/proximity, idle/grounded,
no-enemy and no-hit safety checks as resting. The native Y item action is
consumed only if the conversation opens successfully. Buttons recheck stage,
slot and safety. Loading, changing modes/locations or becoming unsafe closes
the conversation. The pause menu remains the place to assign new blessings.

## Rare Hylian rupees (0.9.0)

Twenty named currency chests contain fifteen rupees each: **300 rupees** total.
Permanent equipment costs **60** for Malo's Hylian Shield and **180** for Castle
Town Magic Armor, leaving sixty spare. Hawkeye is a Bow acquisition gift. There
are no oil purchases. Cache names are in `src/economy/rare_model.hpp`; their
original currency items were checked against the local location catalogue.

The owned balance appears in the native HUD and equipment sheet and lives in
the versioned save blob. Native wallet limits, armor currency effects, loose
coins, repeated NPC/minigame payouts and death deductions cannot spend or mint
this balance. Required services keep the existing currency-free quest callbacks.
Purchase queues reserve funds; payment is recorded only after actual delivery.
Dispatch rechecks saved funds/ownership, including after an interrupted load.

Old saves receive credit for already opened cache locations once. Their former
craft/Twilight relic provenance is captured before reusing the chest locations.
Migration uses a transactional copy; a full ledger cannot repeatedly credit
money. The merchant conversation milestone supplies the player-facing purchase
choices. The purchase functions are not ordinary native shop stock.

## Collections and conversations (0.10.0)

Agitha gives one Growth Fragment for each golden bug welcomed. Her paired
milestones are one pair: five fragments; three pairs: Deepwater Craft Relic;
six pairs: four Flask Shards; twelve pairs: a Golden Crest displayed as a gold
G emblem on the equipment sheet. The native indexed hand-in flags remain the
source of truth. Already presented bugs receive their fragment once on migration.

Jovani gives one fragment at five souls, five at ten, a Twilight Sigil at twenty,
four Flask Shards at forty and a Spirit Crest at sixty (a blue S equipment emblem).
These are menu honors, not new costumes. His original curse/cat/story scenes
remain available through **Discuss the curse**. Native twenty/sixty-soul grants
share the milestone ledger, preventing duplicate gifts and repeatable currency.

Barnes exchanges the Craft Relic for bomb capacities **60 / 30 / 20**, after
Link owns a bag. Castle Town Malo Mart exchanges the Sigil for Magic Armor
using **15 rather than 20 stamina per absorbed hit**, after the armor is owned.
The fifty-percent damage reduction stays unchanged. Former Forge/Lens artifacts
remain redeemable for compatible older saves; new saves obtain Shield with rare
rupees and Hawkeye with Bow. The original ledger does not encode chest item
identity: unredeemed Forge/Lens provenance from an intermediate 0.9.0 save
cannot be distinguished from its new rupee caches; those two artifacts are
only reconstructed when migrating a pre-0.9 economy. Already redeemed armor
benefits survive intermediate saves.

There are **twelve world shards plus eight collection shards**: twenty total,
five upgrades, eight default flask charges. Old earned shards are retained,
with capacity still capped. Other named coin/ammunition checks still award
Growth Fragments, up to twenty-two blessings total; a fully curated growth
pacing table remains future work. The collection grants themselves total
thirty-five fragments, seven blessings.

Stand safely facing the relevant nearby NPC and press **A** (or **L+Y**) to
open this mod's dialogue. Malo, Barnes, Castle Town Malo Mart, Agitha and Jovani
present permanent choices. Coro explains renewable fuel after the native Lantern
gift; Sera's new dialogue starts after the native cat/milk reward; Castle Town's
Goron craftsman explains the new gear/relic rules. Prices and progression are
shown here and in the actual equipment pause menu, with no F1 purchase buttons.

**Talk about other matters / Discuss the curse** closes the mod dialogue;
press A again within fifteen seconds for the native conversation. This preserves
native quest/cutscene processing. Scene, slot, NPC identity, distance and threat
are rechecked by dialog actions. Failure to open a dialog leaves native input
intact. Original fallback dialogues can still mention obsolete shop prices;
rewriting every message/minigame/donation is outside this milestone.

The full automated gate covers collection gift budgets, all 4,096 pair
combinations, indexed native flags, duplicates, failed queues, pure previews,
corrupt/full ledgers and save changes, alongside actor-context/input/dialog
regressions. Appearance, controller navigation, NPC placement and complete
story routes still need in-game validation.

## Review corrections (0.10.1)

The gameplay and architecture reviews found two native integration faults that
function-only mocks missed. Already claimed Jovani rewards now present an inert
acknowledgment instead of NONE: his native actor-waiting branch can continue,
with no duplicate gift. Native duplicate bug hand-ins use the same safe pattern.
The regression executes the pinned presentation function and Pouya branch.
Ordonna's L+Y interaction now runs before native stick dispatch and masks only
its own Y edge while checking actual stone idle safety; failed dialogs restore
input and unrelated simultaneous actions still prevent the interaction.

Failed blob size probes, oversized blobs, failed/changed reads and corrupt bytes
leave progression unavailable and unwritable. Synthetic purchase/trade/gift
checks explicitly cancel across disabled settings or mode changes; the outer
resolver handles them before its native-mode guard. Native vanilla sources
continue to fall through. Queue cancellation clears pending reservations.

There is one **Overhauled economy** toggle for coin suppression, finite currency
and permanent purchases. A separate rare-currency toggle would leave equipment
unavailable when the rest of the economy remained enabled, so it was removed.
Quest/relic gifts have their own control; they do not remove permanent purchase
choices. Barnes's hints avoid future boss/item spoilers, and obsolete legacy
relics report that their equipment is already owned.
