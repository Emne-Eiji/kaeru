# Kaeru Icon Assets — Frog Identity

**Official visual identity:** the frog icon set supplied and approved by Emne
(2026-11). Original artwork is preserved byte-identical; Kaeru never redraws,
recolors or regenerates it.

## Locations

| What | Where | Notes |
|---|---|---|
| Original source package (untouched) | `../frog_icons/` (outside the repo) | `icons/awake|focused|sleep.svg`, `icons/*.png` (16–512), `frog-tray-icons.svg/png`, `frog-tray-lab.html`, `generate_icons.py`, proof images |
| Authoritative app assets | `kaeru/resources/icons/frog/` | the three SVGs + `frog-tray-icons.svg` (documentation) + `generate_icons.py` (provenance) |
| Raster variants | `kaeru/resources/icons/frog/png/` | supplied PNGs 16, 24, 32, 48, 64, 128, 256, 512 per expression |
| **Embedded (app UI)** | Qt resources `:/icons/frog/png/{awake,focused,sleep}-{16…256}.png` | **runtime icons are PNG-only**; see `Icons.cpp` |
| Installed (launcher) | `share/icons/hicolor/scalable/apps/io.github.kaeru.Kaeru.svg` | awake expression |
| Installed (states) | `share/icons/hicolor/scalable/apps/io.github.kaeru.Kaeru.{focused,sleep}.svg` | focused/sleep expressions |
| Installed (raster) | `share/icons/hicolor/{16..256}x{16..256}/apps/io.github.kaeru.Kaeru-{awake,focused,sleep}.png` | supplied PNGs, per-size dirs |

## Expressions

| File | Meaning |
|---|---|
| `awake.svg` / `awake-*.png` | Keep-awake inhibition actively held |
| `focused.svg` / `focused-*.png` | A focus interval is actively running (preserved for the deferred Focus module) |
| `sleep.svg` / `sleep-*.png` | Kaeru inactive (paused/off/failed) — never "the computer is sleeping" |

Tray mapping + precedence: `docs/decisions/ADR-008-frog-icon-identity.md`.
Palette: face `#4CAF50`, stroke/features `#1B5E20`, eye white `#FFFFFF`
(all expressions share geometry, stroke weight and framing).

## Why runtime uses PNGs (0.2.0)

Qt 6.2's SVG renderer mis-paints `focused.svg`: the clip-path construct on
the eye lids causes the face to render black. The supplied PNG variants
rasterize correctly. Kaeru therefore embeds the PNGs for every runtime
`QIcon` (window, tray, About) while still installing the SVGs for desktop
menus/launchers that prefer scalable icons. This is an asset-selection fix;
no artwork was edited.

## Verification performed

- **0.1.1 probe:** SVG rendering confirmed the black-face defect in Qt 6.2;
  the supplied PNGs rendered the correct palette.
- **0.2.0:** `test_icons` verifies all 21 embedded PNG variants exist at the
  expected size (3 expressions × 7 sizes), `QIcon` exposes raster sizes
  16/64/256, the three expressions are visually distinct, and the focused
  render contains green body pixels and light eye/highlight pixels.
- **Live tray (0.2.0):** SNI item registered; ToolTip reports the Keep Awake
  state and remaining time; `IconPixmap` exposes 5 embedded pixmaps built
  from the PNG icon. The user's panel theme/desaturation is desktop-side.
- **Packaging:** the `.deb` contains 21 PNGs under hicolor size directories
  and the 3 SVGs under `scalable/apps`; md5sums verified.

## Licensing

Frog artwork © 2026 Emne, licensed GPL-3.0-or-later as part of the Kaeru
project (see `LICENSE`, `debian/copyright`). The original source package
remains the upstream reference; no third-party assets are incorporated.
