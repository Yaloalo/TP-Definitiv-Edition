# Combat and enemy AI design

Updated 2026-10-14. The owner approved a Souls/Elden Ring-inspired direction:
stamina-based combat, hidden enemy stance, committed heavy actions, meaningful
Hidden Skills and a shared enemy decision architecture. This document records
that direction; it does not claim every system below is implemented. Current
prototype coverage is recorded in [COMBAT_IMPLEMENTATION.md](COMBAT_IMPLEMENTATION.md).

The owner approved D1 A, D2 A, D3 B, D4 A, D5 A and D6 B. Implementation is
authorized, including a dedicated heavy move and a distinct guard counter.
A full Xbox controller is available; preserve essential native controls while
using the additional physical inputs. Existing economy and
dialogue work is a separate milestone. This document supersedes the proposed
Sekiro direction and the old parry-only implementation sequence.

## Approved design boundaries

- Preserve TP's atmosphere, sword identity, items, story and encounter variety.
- Build combat around positioning, stamina, attack commitment and readable
  openings. Timed parries are useful, but continuous deflection is not mandatory.
- Give enemies hidden stance resistance alongside health. Heavy actions,
  counters and appropriate items are effective ways to break it.
- Integrate all seven learned Hidden Skills into the main combat loop, rather
  than adding isolated damage bonuses. Preserve learning and training events.
- Coordinate enemy intentions through shared logic and capability profiles;
  use small native adapters rather than rewrite every complete state machine.
- Preserve earned upgrades in existing saves. Combat tuning must not silently
  remove health, stamina, skills, flask capacity, equipment or relic ownership.
- Keep Flurry Rush, visible enemy/boss health bars, damage numbers, XP, loot
  tiers, durability, farming and inflated enemy HP excluded. Enemy stance is
  also hidden in the normal HUD.
- Own these systems in Definitive Edition with module controls. Do not depend
  on external gameplay presets. Zora Swim/Dark Morpheel remains the exception.

## Link's combat loop

Observe an attack, choose spacing/guard/evasion, create an opening, then choose
between a quick hit and a committed stance-breaking action. Sustained pressure
can produce a break and a strong punish. Retreating gives stamina recovery but
allows the enemy's stance to recover. A blocked hit, a parry, an ordinary hit
reaction and a full stance break are distinct outcomes.

Link uses health and stamina. Do not add a second full player posture meter on
top of stamina-based blocking. Blocking spends stamina on contact; exhaustion
may cause a brief native guard-break reaction. Dodges and powerful techniques
have costs. Ordinary sword swings have a small stamina cost (D1 A).
Successful parries should be efficient, rewarding alternatives to blocking;
exact costs, windows, refunds and exhaustion rules require prototype tuning.

Inputs must preserve targeting, item use and the learned skill gestures where
practical. Do not assign a new heavy-attack button or replace held sword input
without checking the existing spin charge and controller mappings. A new
generic heavy sword move is optional; existing moves can establish the system.

Guard counters are proposed committed attacks after an ordinary successful
block. They are not parries, cannot cancel every enemy combo safely, and must
have a clear input window and interruptible recovery. The owner approved their inclusion in the first milestone (D6 B). The native
animation and control binding still require implementation verification.

Flask healing has a readable commitment. Enemies can react to its visible
animation after a delay; they cannot know the player pressed heal before it
becomes observable. Keep safe flask/ammo recovery and ward recovery consistent
with the encounter's actual danger; do not accidentally permit free refills
merely because the director temporarily withholds an attack opportunity.

## Every Hidden Skill has a job

The roles below are the design targets. Costs, damage and actor support are
tuning work. Native learning gates stay (D4 A); Great Spin loses its full-health condition
while retaining its learned flag and stamina cost (D5 A).

| Skill | Integrated role | Limits and interactions |
|---|---|---|
| Ending Blow | Converts an eligible knockdown/stance break into a decisive execution opportunity | Preserve native target eligibility and death/event handling. Tougher enemies receive a supported punish instead of an unconditional instant kill. Never bypass a boss phase. |
| Shield Attack | Timed active parry with substantial stance impact and a brief counter opening | Distinguish correct contact timing from an ordinary shield bash. Failed attempts retain recovery. Attacks declare whether they can be parried; no universal projectile/grab/boss parry assumption. |
| Helm Splitter | Committed follow-up after a successful shield/parry opening, with strong stance pressure | Remains useful against guarded fronts without becoming a freely repeatable launcher. Requires its learned flag and a compatible target reaction. |
| Back Slice | Evasive positional counter that reaches a vulnerable side or rear | Reward correct positioning and timing; do not grant invulnerability for its entire movement or bypass every enemy's armor rules. |
| Mortal Draw | High-risk opening/whiff punish with high health damage and meaningful stance impact | Retain the sheathed-sword setup and exposure. It cannot be instantly chained from ordinary attacks or guarantee a kill against elites/bosses. |
| Jump Strike | Charged area attack and major stance-pressure tool | Wind-up, landing recovery and stamina cost create commitment. Each target receives only its intended hit events; multiple collision callbacks must not multiply stance damage. |
| Great Spin | Expensive crowd control that makes space against coordinated groups | Keep per-target stance pressure below a dedicated single-target breaker unless testing supports otherwise. Its full-health requirement is removed (D5 A). Prevent cancel/refund loops. |

Normal attacks maintain pressure and reliable health damage. The ordinary jump
attack can be an early committed option before Jump Strike is learned. Spin
attacks remain useful before Great Spin. No encounter should require a skill
the player has not had a fair opportunity to learn; optional later techniques
provide alternatives, not arbitrary progression locks on ordinary enemies.

## Hidden enemy stance

Use a shared model with per-profile resistance, recovery delay/rate and attack
impact categories. Keep stance separate from damage and from short flinches.

- Only accepted combat contacts contribute: distinguish a body hit, shielded
  hit, invulnerable target, parry and special item interaction. A blocked hit
  may cause stance pressure if the enemy profile permits it.
- Maintain pressure through appropriate light hits; heavy techniques and
  counters provide stronger impact. Exact values are provisional until tested.
- Begin recovery after a meaningful gap in pressure. Reset on spawn/despawn or
  encounter reset; never inherit another actor's state when IDs are reused.
- A full break requests a supported native reaction. Consume/reset the stance
  state and use a defined recovery/re-entry rule to prevent perpetual breaks.
- Deduplicate contact events by attack instance and target, with explicit
  support for legitimate multihit moves. Cap or constrain ranged/renewable-item
  pressure so safe spam cannot indefinitely immobilize an enemy.
- Preserve native armor, weak points, scripted invulnerability and boss phases.
  An unsupported actor must keep working natively; do not force a generic
  downed state or death flag into every enemy class.
- Communicate breaks through readable animation, sound and restrained effects.
  Debug instrumentation may expose values for QA; normal gameplay does not.

The inspiration is Elden Ring's stance-breaking role for charged/jumping
attacks and guard counters, as described in the
[official combat guide](https://www.bandainamcoent.com/es_mx/news/elden-ring-introduction-part-4-combat-guide).
These TP rules are our proposed adaptation, not a claim of exact Elden Ring
formulas or timings.

## Progression and existing saves

Health, stamina, flask and equipment progression must support the combat rather
than substitute for learning it. Base stamina must allow a useful offensive
sequence and a defensive response; exploration upgrades add options and room
for mistakes. Favor bounded, technique/item-specific refinements over stacking
universal damage or stance multipliers. Do not turn Hidden Skills into random
chest pickups or charge materials for already learned techniques.

The current implementation has shared heart/stamina blessings and later free
redistribution. Separating those rewards was discussed during the abandoned
Sekiro proposal, but was not approved. The owner chose D2 A: keep the current shared pool and redistribution. If changed, specify an idempotent save migration that
preserves earned benefits and cannot grant the same reward twice. Do not
rebalance by confiscating already allocated hearts or stamina.

Existing flask shards, finite relic recipes and rechargeable Magic Armor remain
part of the design. Review their combat interactions, including block versus
actual health damage and ward charges, without replacing their ownership data.
Wolf Link requires appropriate stamina/stance contributions and healing access,
but should not inherit human sword-parry assumptions. Boss changes follow the
ordinary-enemy foundation and retain their original mechanics and spectacle.

## Shared enemy AI architecture

Use the existing native state machines as action executors. Put shared intent
selection above them, rather than replacing their entire actor execution.

| Layer | Responsibility | Boundary |
|---|---|---|
| Encounter director | Assign pressure/approach roles, attack opportunities and fair concurrency within a local encounter | Does not force animations, teleport actors or make every enemy globally aware of Link. |
| Utility decision model | Score available intentions from observed distance, visibility, threats, recent actions and roles | Pure logic over snapshots. Commit decisions long enough to avoid jitter; reevaluate at safe boundaries. |
| Capability/personality profiles | Declare attacks, ranges, defense options, aggression, reaction delays and recovery rules | Unsupported actions are unavailable, not approximated by writing arbitrary native action IDs. |
| Native adapters | Observe native state, report legal actions, start approved intents and map combat outcomes to supported reactions | Small integration per enemy implementation; helpers may be shared across compatible types. Per-enemy work still exists. |

Example intentions: approach, pressure, reposition, flank, attack, defend,
retreat and recover. The utility model scores them; profiles give enemies
different priorities without duplicating the entire decision algorithm.

Suggested flow: native observations -> immutable encounter snapshot -> director
roles/opportunities -> per-enemy utility scores -> intent request -> adapter
accepts at a legal boundary -> native execution -> reported outcome.

### Fairness and execution contracts

- React to visible animations and sensed events after an appropriate delay.
  Never read future input or use hidden flask counts as knowledge of healing.
- Preserve attack commitment. Do not cancel an attack into a perfect counter
  merely because Link acted. Cap tracking during committed attack phases.
- Maintain readable openings and bounded simultaneous pressure. Difficulty
  can change coordination and timing, without unlimited overlapping attacks.
- Separate pursuit, support and actual attacks. Attack reservations expire and
  are released on interruption, death, despawn, lost reachability or scene exit.
- Native execution remains responsible for animation, collision, navigation,
  damage/death and story events. Use its movement constraints; flank intentions
  do not establish that a navigable route exists.
- Adapter requests can fail or defer. Fall back to native behavior safely; do
  not continuously overwrite action/substate fields each frame.
- Do not run native and shared decision selection against each other. An
  adapter defines precisely which native decision boundary it takes over while
  preserving the execution and safety logic around it.
- Suspend shared control during cutscenes, tutorials, unsupported phases,
  scripted encounters and mode transitions. Handle Wolf/human changes and
  mixed supported/unsupported groups explicitly.
- Keep runtime stance and AI state outside native actor layouts, keyed by actor
  identity plus lifecycle/generation. Clear on lifecycle changes and save/load.
  Persist permanent player progression only, not stale encounter reservations.
- Use deterministic time and controllable random seeds in the pure models.
  Update decisions at a bounded cadence, independent of rendering FPS; native
  collision/animation processing continues at the host's required cadence.

Suggested module boundaries: `combat/stance` for rules; `combat/skills` for
learned-technique outcomes; `ai/model` and `ai/director` for pure decisions;
`ai/profiles` for tuning; `ai/adapters` for native integration. These are design
names, not claims that these files currently exist. Integrate with the existing
stamina, manual shield, damage, player hooks and save services. Avoid duplicate
damage application or multiple hooks charging stamina for one action.

Local feasibility evidence: Link already has guard-slip/break and Shield Attack
procedures (`dusklight/src/d/actor/d_a_alink_guard.inc`); enemy dispatchers such
as `d_a_e_dn.cpp` and `d_a_e_dd.cpp` mix actions, timers, animations and damage
responses. Paths are relative to the parent TP workspace. This supports thin
adapters, but does not establish universal safe transitions or hook availability.
Resolve required hooks against the pinned host before choosing the first actor.

## Owner decisions (approved 2026-10-14)

Accepted: **D1 A / D2 A / D3 B / D4 A / D5 A / D6 B**. The table retains the
alternatives and original recommendations for context; the accepted selections
above control implementation. No further approval is needed for routine tuning.

| ID | Question | A | B | Recommendation |
|---|---|---|---|---|
| D1 | Should ordinary sword swings consume stamina? | Small cost; cannot start another ordinary swing when exhausted, but never cancel a committed swing | Free ordinary swings; stamina pays for blocks, dodges and powerful actions | A for stronger Souls-style commitment; tune regeneration/base capacity to avoid idle waiting |
| D2 | How should health/stamina rewards work? | Keep shared blessings and free later redistribution; balance costs/caps around meaningful choices | Separate fixed health and stamina rewards, with a migration preserving all earned benefits | A for the chosen Souls direction; the earlier separation proposal is not adopted automatically |
| D3 | What provides early heavy attacks? | Use existing jump attacks and charged spins first; expand through learned skills | Add a dedicated heavy sword input/move, subject to animation and controller feasibility | A to establish feel without conflicting with TP's existing controls |
| D4 | When are Hidden Skills available? | Preserve native learning gates; integrate every learned skill deeply and tune encounters around available abilities | Plan earlier access to defensive skills through revised Hero's Shade encounters, preserving their story content | A initially; B requires a separate story/access proposal before implementation |
| D5 | Does Great Spin still require full health? | Remove the full-health gate; retain learning requirement and make stamina/commitment its limit | Keep the native full-health requirement alongside the reworked combat role | A so late-game crowd control remains useful after taking damage |
| D6 | Should a guard counter be part of the first combat milestone? | Start with timed Shield Attack and existing follow-ups; consider a separate guard counter after playtesting | Include a distinct ordinary-block counter from the start, with a proposed input/animation to validate | A to avoid duplicating Shield Attack/Helm Splitter before their roles are proven |

Exact stamina costs, stance thresholds, timings and first supported actor are
implementation/playtest choices within the approved rules. They are not reasons
to request repeated design approval. Any later change to reward ownership,
story access or the agreed input model needs an explicit revised proposal.

## Implementation and validation sequence after approval

1. Apply the owner's recorded answers. Finish/validate the separate pending economy
   milestone and preserve its candidate. Audit hooks and choose one compatible
   melee actor; establish baseline recordings and supported reactions.
2. Build pure stamina/stance/contact rules and one native adapter. Deliver a
   complete light/heavy pressure -> break -> supported punish encounter, with
   timed Shield Attack and its learned follow-up where unlocked. Validate all
   seven skills' intended integration in suitable learned-skill fixtures/saves;
   not every move must be unlocked in the first story slice.
3. Add the shared intent model and a small group director for that actor type.
   Compare one-on-one and two-enemy encounters. Prove bounded concurrency,
   reachable repositioning, reaction delays and safe native fallback.
4. Expand adapters and skill interactions across ordinary enemies, armored
   enemies, ranged threats and Wolf encounters. Integrate item roles and check
   exploration upgrades against early-, middle- and late-game loadouts.
5. Rework bosses individually once ordinary combat works. Preserve required
   items, vulnerable phases, ending events and rewards.

Automated tests must cover contact deduplication, genuine multihits, stance
recovery/break reset, block/parry distinctions, insufficient stamina, successful
versus failed action charging, learned flags, guard-break/recovery, unsupported
actors, actor-ID reuse, cancellation, save/load, mode changes, scene transitions,
director reservation release and deterministic reaction delays. Exercise actual
production callbacks and native routines where feasible, not only model copies.

Run the full build/QA gate after every major change and before milestone
commits/pushes. Preserve packages and verify pinned-host hooks. Runtime testing
must establish telegraph readability, control feel, camera behavior, group
fairness, no stun-lock loops, Wolf compatibility and story/tutorial/boss gates.
Fixtures cannot prove whole-game story correctness. Keep routes pending until
actually played and record evidence as described in [QA.md](QA.md).
