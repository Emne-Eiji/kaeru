# Kaeru Icon Assets — Frog Identity

**Official visual identity:** the frog icon set supplied and approved by Emne
(2026-11). Original artwork is preserved byte-identical; Kaeru never redraws,
recolors or regenerates it.

## Locations

| What | Where | Notes |
|---|---|---|
| Original source package (untouched) | `../frog_icons/` (outside the repo) | `icons/awake|focused|sleep.svg`, `icons/*.png` (16–512), `frog-tray-icons.svg/png` (preview composition), `frog-tray-lab.html`, `generate_icons.py`, proof images |
| Authoritative app assets | `kaeru/resources/icons/frog/` | the three SVGs + `frog-tray-icons.svg` (documentation) + `generate_icons.py` (provenance) |
| Raster variants | `kaeru/resources/icons/frog/png/` | supplied PNGs 16, 24, 32, 48, 64, 128, 256, 512 per expression |
| Embedded (app UI) | Qt resources `:/icons/frog/{awake,focused,sleep}.svg` | via `qt_add_resources(BASE resources PREFIX /)` — see Icons.cpp |
| Installed (launcher) | `share/icons/hicolor/scalable/apps/io.github.kaeru.Kaeru.svg` | awake expression |
| Installed (states) | `share/icons/hicolor/scalable/apps/io.github.kaeru.Kaeru.{focused,sleep}.svg` | focused/sleep expressions |
| Installed (raster fallback) | `share/icons/hicolor/{16..256}x{16..256}/apps/io.github.kaeru.KaerU-{awake,focused,sleep}.png` | supplied PNGs, per-size dirs |

## Expressions

| File | Meaning |
|---|---|
| `awake.svg` | Keep-awake inhibition actively held |
| `focused.svg` | A focus interval is actively running |
| `sleep.svg` | Kaeru inactive (paused/off/failed) — never "the computer is sleeping" |

Tray mapping + precedence: `docs/decisions/ADR-008-frog-icon-identity.md`.
Palette: face `#4CAF50`, stroke/features `#1B5E20`, eye white `#FFFFFF`
(all expressions share geometry, stroke weight and framing).

## Verification performed (2026-11-16)

- `svg_probe` (Qt6 qsvg, offscreen): QIcon loads `awake.svg` at 16/24/32/64/128
  px, all pixmaps non-null (previously null before `libqt6svg6` was installed).
- Live tray: SNI item `:1.359/StatusNotifierItem` (example) registered on the
  session watcher; ToolTip verified in all five states
  (inactive → keep awake active → both (awake+focus text) → focus running →
  inactive); panel diff with/without Kaeru isolates the icon region
  (x 1670–1741); the D-Bus `IconPixmap` payload contains the exact official
  colors ARGB `(255,76,175,80)` = #4CAF50 and `(255,27,94,32)` = #1B5E20.
- The dev panel desaturates tray icons (user's `desaturated-tray-icons`
  extension) — the frog renders greyscale there by desktop configuration, not
  by Kaeru; shape remains legible (readability proof: `../frog_icons/proof-16px.png`).

## Licensing

Frog artwork © 2026 Emne, licensed GPL-3.0-or-later as part of the Kaeru
project (see `LICENSE`, `debian/copyright`). The original source package
remains the upstream reference; no third-party assets are incorporated.