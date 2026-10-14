# Combat implementation and validation

The approved direction and decisions live in [COMBAT_AND_ENEMY_AI.md](COMBAT_AND_ENEMY_AI.md).
This page records implemented behavior separately from the wider design.

## Link techniques (0.13.0)

- A dedicated heavy sword attack uses the native overhead finisher animation,
  played at 75% speed with a later cancel boundary. Its default cost is 28.
- After an ordinary successful shield block, a one-second opportunity allows
  the same input to produce a distinct right-swing guard counter (90% animation
  speed, cost 25). It starts at a native action decision boundary after recovery;
  it does not cancel an incoming hit or guarantee safety against a combo. Taking
  damage cancels the opportunity and any buffered heavy input.
- Xbox LB is the default physical heavy/counter input. Existing native action
  mappings remain unchanged. F1 allows L3/R3/LB/RB selection; a button already
  mapped to a native button, axis or separate host action is ignored to prevent triggering two actions.
  The port-one controller is read through resolved SDL/host functions; no host
  controller layouts or launcher presets are modified. A fresh release/press is
  required after focus, device, binding, menu or scene changes. Held input cannot repeat.
- Ordinary sword swings keep their existing small cost (default 12), with an
  additional decision-level exhaustion check for buffered combos. Existing
  held-sword spin input remains available.
- Ordinary shield impacts cost 18 stamina. Exhaustion causes native guard break,
  replacing the native every-fourth-guard-slip break for supported human shielding.
  Native special unblockable/break attacks and mounted/item defenses remain native.
- Learned Great Spin works below full health and costs 40. Its learning gate is
  owned in gameplay, independent of launcher presets. Teaching/events stay native.
- Jump Strike now charges its launch cost; Mortal Draw uses the technique cost
  rather than an ordinary-swing cost. Back Slice, Helm Splitter and Ending Blow
  retain their existing native gates and action costs.

These are reused native animations with distinct action/input semantics, not
newly authored animation assets. Visual feel and controller responsiveness must
still be tested in the release host. The complete seven-skill stance integration,
shared AI director, other enemy adapters and boss redesign are separate stages.

Automated coverage includes actual production input/technique callbacks for
normal/heavy/counter selection, charging exactly once, failed actions, exhaustion,
Great Spin learning, block costs, focus/menu/device transitions and native mapping
conflicts. Full runtime routes remain pending; no fixtures establish game feel.

## First hidden-stance adapter (0.14.0)

Ordinary human combat against Bokoblins now uses hidden stance: default 33 for
the native 40-HP variant and 55 for the sturdy 220-HP variant. Native health is
unchanged. This lets a committed heavy break the weaker enemy before a second
health hit would kill it.
Accepted sword/shield contacts contribute once per attack identity, including
contacts spanning both hurt spheres. Light hits add 10, the dedicated heavy 35,
guard counter 40, Back Slice 28, Helm Splitter 40, Mortal Draw 35, ordinary jump
25, Jump Strike's main/landing phases 40, spin 18 and Great Spin 25. Ending Blow
continues through native execution eligibility/death handling. Native lethal
hits still kill; no HP inflation or forced resurrection is introduced.

After three seconds without pressure, stance recovers at 15 points per second.
A break enters native big damage/fall/down handling and resets pressure, with a
two-second period before further stance accumulation. Ordinary nonlethal sword
finishers do not automatically bypass stance into knockdown. Native item hits,
Wolf attacks, death, scripted states and unsupported enemy types stay native.
Donor finisher-count and short-get-up changes are disabled for this adapter so
there is one owner of its sword stance/knockdown rules.

Shield Attack's first six simulation ticks can parry an actual incoming guarded
Bokoblin melee attack. A successful parry adds 40 stance pressure and interrupts
that attack into a native shield-reaction opening or full stance break. Native
Helm Splitter targeting and learned flags remain responsible for the follow-up.
There is one accepted incoming parry per Shield Attack, with a shield-impact
sound cue. Failed timing remains ordinary shielding. A plain bash causes brief
recoil but does not grant the timed-parry Helm Splitter opening. A bash/parry
from the same attack contributes at most the higher impact, never both summed. Unsupported enemies do not receive a fabricated response.

The stance adapter has production-callback tests for accepted versus ignored
contacts, deduplication, recovery, lethal/Ending Blow preservation, scripted and
Wolf isolation, skill impact categories, parry eligibility, actor reuse/deletion,
menu freezing and mode reset. Whole-game integration and visible feedback still
require runtime playtesting. The shared enemy AI director is documented but is
not part of this first combat prototype; other adapters and bosses remain pending.


## Review status

The requested architecture and gameplay review agents were both launched after
initial QA, but both failed at the service usage limit before returning findings.
No independent grade or sign-off is available. They remain pending.

The primary-agent review added regression-covered fixes for host action-binding
conflicts, binding changes while held, damage cancelling a counter opportunity,
timed-parry-only Helm Splitter openings, bash/parry contact deduplication, bounded
recovery counters, independent stance toggling, and weak-enemy stance resistance
that does not require inflating native HP. This is not an independent review.
