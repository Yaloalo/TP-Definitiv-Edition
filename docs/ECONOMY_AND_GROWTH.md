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

## Subsequent approved milestones

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

- Rare rupees from curated one-time treasures/quests, without drops, farming,
  wallet limits, death loss or required story purchases.
- Merchant dialogue choices for permanent purchases and specific relic hand-ins.
- Agitha bug-pair milestones and Jovani Poe milestones granting shared growth,
  crafting/Twilight relics, flask upgrades and completion emblems.
- Redistribute existing shard/relic rewards rather than adding unlimited power.

Each milestone requires production regression tests, the complete build gate,
a focused commit and a push. Full story routes remain pending until played.
