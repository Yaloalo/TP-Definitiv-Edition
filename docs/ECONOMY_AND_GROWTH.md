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

## Subsequent approved milestones

- Free redistribution at a Light Spirit spring, introduced after Forest Temple.
- Rare rupees from curated one-time treasures/quests, without drops, farming,
  wallet limits, death loss or required story purchases.
- Merchant dialogue choices for permanent purchases and specific relic hand-ins.
- Agitha bug-pair milestones and Jovani Poe milestones granting shared growth,
  crafting/Twilight relics, flask upgrades and completion emblems.
- Redistribute existing shard/relic rewards rather than adding unlimited power.

Each milestone requires production regression tests, the complete build gate,
a focused commit and a push. Full story routes remain pending until played.
