# Deliberate rewards and Magic Armor — 0.11.0

This milestone implements the owner's request for meaningful collectible trades,
controlled growth pacing and a complete non-currency Magic Armor rework. Native
story access, important rewards and existing earned benefits remain intact.
Runtime appearance, NPC navigation and complete story routes remain pending.

## Exploration and growth

`data/reward_catalog.json` explicitly assigns **278 persistent finds** with
original item IDs and native check names: 146 Hyrule Craft Relics, 84 Ancient
Runes, 14 Growth Fragments, 12 Flask Shards, 20 rare rupee caches and two unique
ward artifacts. The generated [placement list](REWARD_PLACEMENTS.md) records each
location. No randomizer or other gameplay mod is required at runtime.

Only deliberately assigned finds give growth. For a new save, native Heart
Pieces/Containers supply 85 fragments, first-pair/Poe blessings supply eleven,
and curated exploration supplies fourteen: **110 fragments = 22 blessings**.
Individual bugs give **one Gilded Thread**, not another growth fragment; all
24 bugs provide the 24 threads needed by the five functional recipes. Native
individual hand-in flags, pair gifts and curse scenes remain preserved.

Flask capacity keeps its twenty-shard budget: twelve world shards plus four
from six bug pairs and four from forty Poe souls. Early world shards span
Ordon/Faron/Forest/Kakariko; the middle set spans Goron/Lanayru/Lakebed/desert;
the final four span Arbiter's Grounds/Snowpeak/City/Palace. Collection awards
supplement those visits. Defaults still cap at eight charges; no extra capacity
comes from bottle acquisition or crafting.

Unknown persistent currency/ammunition checks have finite crafting value,
never blanket growth. Unnamed drops, shop previews and invalid bit IDs cannot
produce collectibles. Native keys, equipment, boss crystals/mirrors and heart
items are preserved. Repeated checks cannot pay again.

## Permanent artisan upgrades

Stand safely facing the relevant NPC and press A (or L+Y). Recipes show their
costs in the NPC dialogue and equipment menu. Functional upgrades are optional.
You cannot buy or farm their components, and each can be completed once.

| Upgrade | Artisan | Requirement and cost | Effect |
|---|---|---|---|
| Scout's Stitch | Sera, after Forest Temple | 6 Craft Relics + 3 Gilded Thread | Human/Wolf sprint costs 10% less stamina |
| Bow Fletching | Kakariko Malo, with Bow | 8 Craft Relics + 6 Thread | Arrow refill interval reduced by 25% |
| Bomb Harness | Barnes, with a bomb bag | 8 Ancient Runes + 6 Thread | All three bomb refill intervals reduced by 25% |
| Sunward Weave | Castle Town Malo Mart | Owned Armor, redeemed Sigil, Sunward Emblem, 6 Runes + 4 Thread | Five ward charges and recovery times reduced by 25% |
| Mirror Ward | Castle Town Malo Mart | Sunward Weave, Mirror Heart, 10 Runes + 5 Thread | Charged hits receive 60% rather than 50% damage reduction |

The **Sunward Emblem** replaces bombs in Arbiter's Grounds' first Spinner-room
small chest (`chest:D_MN10:28`). The **Mirror Heart** replaces currency in Palace
of Twilight's east-wing second-room northwest chest (`chest:D_MN08:29`). They
remain specific artifacts and never act as interchangeable money.

After functional upgrades, optional relic honors preserve a purpose for spare
finds: Malo's **Craft Crest** costs forty Craft Relics, Malo Mart's **Ancient
Crest** costs forty Runes, and the **Hyrule Keeper Crest** requires both and costs
eighty Craft Relics plus twenty Runes. All recipes together cost 134 Craft
Relics, all 84 Runes and all 24 Threads; normal placements leave twelve Craft
Relics as a buffer. Honors are permanent menu crests, not new costumes.

Collecting forty and 160 exploration treasures also earns Wayfarer/Archivist
honors. Native STAR, snowboard, Iza and Cave of Ordeals prizes keep their original
permanent item functions and add completion honors. Existing native completion
flags recover those honors on old saves.

## Rechargeable Magic Armor ward

Magic Armor has **three ward charges**, reducing one actual damaging hit by
50% per charge. Jovani's redeemed Twilight Sigil raises capacity to four.
Sunward raises it to five and improves recovery; Mirror strengthens protection.
Hits always deal at least one native damage unit. There is no invulnerability,
stamina cost, rupee drain, equipment durability or empty-wallet weight penalty.
When empty, the armor retains normal movement and provides no damage reduction.

By default, one charge returns after **twelve seconds** clear of damage and
living enemies within 3,000 units, then **six seconds** per charge. Sunward
reduces those times to nine and 4.5 seconds. Separate timing sliders and the
ward toggle are in F1. Rest, genuine same-slot death/retry and first boss
victories restore charges. Menus/events, scene changes and save loads reset
recovery clocks; normal loading and swapping tunics never refill spent charges.
Safe travel can recover the ward while another outfit is worn. No timers are
stored or progressed offline.

The mod owns final ward damage while retaining native hit reactions/flags,
including when a native armor preset is selected. Native armor invulnerability
and weight checks are disabled throughout this game mode, so Magic Armor cannot
spend rupees even if the economy toggle is off. Disabling the ward removes its
protection; it does not restore currency drain. Wolf form does not gain armor
protection. The HUD shows remaining charges while Magic Armor is worn.

## Equipment menu and persistence

Z on the native equipment/tunic pause screen opens four pages, navigated with
D-pad left/right: growth and gear, functional recipes/relics, optional relic
honors, and earned crests. A/X allocate growth only on the first page. B/Z
returns. The panel retains safe native submenu, save and external-UI gates.

The save blob now writes **TDF3** and still reads exact TDF1/TDF2 layouts. Existing
health/stamina allocations, fragments, flask charges/capacity, rare currency,
claimed checks, native gear and redeemed Sigil benefits remain. Older world
finds that now award crafting materials/artifacts receive their new collectible
credit once, without deducting their previously earned growth. Earlier bug growth
also remains while its hand-ins receive Thread credit once. A full ledger makes
migration fail atomically; unreadable data remains unavailable and unwritable.
# Economy follow-up (0.12.0)

Native merchant, donation and minigame messages now have an owned dialogue
module: forty explicit prompts/descriptions plus context-aware price cleanup
for loaded message resources. Native choice/control tags and activity rules
remain intact. Donation amount rendering is hidden while the original native
confirmation/cancellation flow still owns quest flags. Replacement prose is
English in all supported language slots; it is not a full localization.

Magic Armor purchases and Malo Mart relic recipes require the native shop
restoration milestone. Ward charge transitions drive the native armor glow and
sound without spending or writing either currency balance. Completing the first
Rollgoal series also awards a permanent Fishing Trial Crest in the pause ledger.

Validation: all 28 required suites and pinned-host package/import/hook checks
passed. Full runtime routes, message layout, controller behavior and ward visual
transitions still require playtesting; automated fixtures do not prove them.
