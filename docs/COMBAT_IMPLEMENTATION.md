# Combat implementation and validation

The approved direction and decisions live in [COMBAT_AND_ENEMY_AI.md](COMBAT_AND_ENEMY_AI.md).
This page records implemented behavior separately from the wider design.

## Link techniques (0.13.0)

- A dedicated heavy sword attack uses the native overhead finisher animation,
  played at 75% speed with a later cancel boundary. Its default cost is 28.
- After an ordinary successful shield block, a one-second opportunity allows
  the same input to produce a distinct right-swing guard counter (90% animation
  speed, cost 25). It starts at a native action decision boundary after recovery;
  it does not cancel an incoming hit or guarantee safety against a combo.
- Xbox LB is the default physical heavy/counter input. Existing native action
  mappings remain unchanged. F1 allows L3/R3/LB/RB selection; a button already
  mapped to a native button or axis is ignored to prevent triggering two actions.
  The port-one controller is read through resolved SDL/host functions; no host
  controller layouts or launcher presets are modified. A fresh release/press is
  required after focus, device, menu or scene changes. Held input cannot repeat.
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
