# Definitive Edition — Graphics Mod Set

This is the curated set of third-party mods that runs alongside the overhaul. It covers DESIGN_VISION.md §10:

- high-resolution textures and improved models
- better lighting
- an improved HUD

The original art direction stays intact.

**Status:**

- Assembled 2026-10-13 for Dusklight v2.0.3.
- Every mod's imported game symbols are checked against the release binary (`tools/check_symbols.sh`).
- Smoke test (title screen, 45 s):
  - 9/10 mods active; Twilight HD is disabled by config, as intended
  - VBAO, SMAA and Deferred Fog report ready
  - Dusklight HD loads 4,574 textures; 77 are rejected by Aurora because they are not 4×4-block aligned, and those
    show the vanilla texture instead
  - the title scene renders correctly
- Gameplay visuals and the in-game pack switcher are not yet verified.

## Layout on disk

```
~/Games/Dusklight/definitive/mods/        ← curated source packages, read by tools/play.sh
tp-remaster/build/runtime-mods/          ← generated --mods folder; excludes integrated standalone gameplay copies
~/Games/Dusklight/texture-packs/<name>/   ← extracted HD texture packs (switch with tools/texture_pack.sh)
~/Games/Dusklight/downloads/              ← original archives
~/.local/share/TwilitRealm/Dusklight/texture_replacements/active-pack → active texture pack
```

The vanilla **Dusklight** launcher uses the normal user mods folder. It does not load this set, except for the
texture pack, which is global.

## Mods

| Mod | Version | Source | Why | Default |
|---|---|---|---|---|
| TP Definitive Overhaul | build | this repo (symlink to `build/mods`) | gameplay | on |
| VBAO (ambient occlusion) | 1.1.0 | [automata-rtx/dusklight-mods](https://github.com/automata-rtx/dusklight-mods) `df57a94`, **built here against v2.0.3** | grounding, contact shadows; avoids over-darkening grass | on |
| Deferred Fog | 2.0.0 | same repo, built against v2.0.3 | re-applies fog after AO so AO doesn't darken fog. Hooks game code, so it must match the game build | on |
| SMAA (anti-aliasing) | 1.0.0 | same repo, built against v2.0.3 | edge AA using brightness + normals/depth | on |
| Cinematic Depth of Field | 1.1.0 | catalog `com.giorgiomendieta.dof_hq`, sha256 `8e35b012…a7ae8b` | better cutscene DoF; optional slight distance blur | on |
| Enhanced Hero's Tunic | 1.1.0 | catalog `com.owlero.link_green_tunic`, sha256 `393294d5…51e50f` | HD textures for Link's face, eyes, hair, tunic, swords, shields, bow, lantern, boomerang, boots, slingshot (inspired by the Wii U tech demo) | on |
| Twilight HD (HUD) | 2.7.4 | [OTPR26/twilight-hd-hud](https://github.com/OTPR26/twilight-hd-hud/releases/tag/v2.7.4) | TP HD–style HUD, menus and maps | **off** (see below) |

**Twilight HD is off by default** (`mod.org_twilight_hd__hud.enabled = false` in `config.json`). It hooks every
`J2DScreen::draw`, and the overhaul's stamina bar draws through `mpKanteraScreen->draw()`. It also remaps
controls (a third item slot on R; Start opens Items). Enable it in the Mods window to try it, and check that the
stamina bar still shows and that shield (Q = R) still works.

**Considered and not included:**

- Hylian Shield (ALTTP-inspired redesign; changes the design, and the tunic mod already retextures the shield)
- Fighter's Sword, Lokomo or ALTTP swords, character swaps (Linkle, Dark Link, Ganondorf…), Nightwalker Epona
  (they change identity, against vision §10 "preserve the original art direction")
- No Underwater Filter, Interlacing (they change the atmosphere)
- automata's `realtime_sun_shadows` and `ssilvb` (still use a retired renderer service; not buildable without
  porting work, which is a future task)

## HD texture packs (one active at a time)

| Pack | Version | Source | Size | Character |
|---|---|---|---|---|
| `dusklight-hd` | 1.06 | [GameBanana 722358](https://gamebanana.com/mods/722358) by TIREDnONFIRE, md5 `c76fdc6606e2ad33bac363ce5016e1f5` | 1.0 GB archive (≈6 GB of textures) | hand-reworked from the original artwork; restores original colours, bloom, rough sand and wall/floor blends; avoids TP HD's oversaturation. **Default; it matches the vision best** |
| `henriko-4k` | 4.0d (4K, PC) | [Henriko Magnifico](https://www.henrikomagnifico.com/zelda-twilight-princess-4k) via MediaFire, sha256 `f6fd6f86…ec3eb1a` | 4.5 GB archive | 1,800+ 4K textures, reworked UI, character pass. Title screen shows "4K" when loaded |

**In game (live, no restart):**

1. Open the Mods window and select *TP Definitive Overhaul*.
2. Under **Graphics**, pick from the **Texture pack** dropdown: Original / Dusklight HD / Henriko 4K.

The dropdown lists every folder in `~/Games/Dusklight/texture-packs/`. Its choice is the `active-pack` symlink,
shared with the script below. It calls the host's `dusk::texture_replacements::reload()`, the same function the
built-in *Use Texture Pack* setting calls. That function is not part of the mod SDK, so re-check it on Dusklight
updates (`tools/check_symbols.sh` covers it).

**From the shell (applies on the next start):**

```sh
tools/texture_pack.sh status
tools/texture_pack.sh dusklight-hd
tools/texture_pack.sh henriko-4k
tools/texture_pack.sh off
```

Mod textures always override `texture_replacements`, so Enhanced Hero's Tunic wins for Link with either pack.

## Updating

- **automata mods:**

  ```sh
  cmake -B build-v203 -DDUSKLIGHT_VERSION=<new> -DDUSKLIGHT_DIR=…
  ```

  Rebuild in `reference-mods/dusklight-mods`, copy the `.dusk` files, then run `tools/check_symbols.sh`.
- **Catalog mods:** download `https://twilitrealm.dev/download/dusklight/<id>/<version>` and compare with
  `package_sha256` from `https://twilitrealm.dev/api/v1/games/dusklight/mods/<id>`.
- **Dusklight version bump:** re-check every native mod with `tools/check_symbols.sh` before playing. Deferred Fog
  and Twilight HD hook game code.
