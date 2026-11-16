# WP-03 — Tray, Single Instance, Autostart, Frog Identity and Debian Package (COMPLETE)

Environment: Pop!_OS 22.04 (Ubuntu 22.04 base), GNOME 42, X11 `DISPLAY=:1`,
`XDG_RUNTIME_DIR=/run/user/1000`, user `banana` (uid 1000). Qt 6.2.4 with
qtsvg (`libqt6svg6`), debhelper 13.6, lintian 2.114, dpkg-buildpackage.
Binary: `kaeru 0.1.1-1` (amd64). All runs sandboxed via `KAERU_TEST_HOME`.

## 1. Tray — dynamic official frog expressions (PASS)

Live X11 session, `kaeru --background`; the SNI item registers on the session
watcher and the ToolTip property was captured at each state:

```
== 1. inactive          → (sa(iiay)ss) "" 0 "Kaeru — inactive" ""
== 2. keep-awake active → "Kaeru — keep awake active · Keep awake (timed)"
== 3. focus + keep-awake→ "Kaeru — keep awake active · Keep awake (timed) · Countdown (59s left)"
== 4. focus only        → "Kaeru — focus running · Countdown (57s left)"
== 5. everything off    → "Kaeru — inactive"
```

- Icon semantics (ADR-008): Awake = inhibition actively held; Focused = focus
  interval running; Sleep = Kaeru inactive (never "computer sleeping").
- Precedence verified in state 3: **awake wins the icon** (safety-first) and
  the tooltip still states the running focus interval — nothing hidden.
- The tray pixmap itself carries the official colors: D-Bus `IconPixmap`
  (22×22 and 64×64) contains ARGB `(255,76,175,80)` = **#4CAF50** and
  `(255,27,94,32)` = **#1B5E20** — the exact supplied palette.
- Panel presence proven by a with/without-Kaeru screenshot diff: the icon
  occupies panel x 1670–1741 and disappears when Kaeru quits
  (`docs/evidence/kaeru-tray-panel-diff.png`, before/after zooms committed).
- The dev panel renders it desaturated because the user's desktop runs the
  `desaturated-tray-icons` GNOME extension (their configuration; shape stays
  legible — readability proof `../frog_icons/proof-16px.png`).
- All five states exercised through the real application, not mock images.

## 2. Single instance (PASS)

`kaeru --background` (primary) + second `kaeru --background` → second launch
forwards ShowWindow and exits 0; one process remains; first keeps serving.
(Fixed in this milestone: `AllowReplacement` takeover + treating the
non-error "name taken" reply as success — both documented in CHANGELOG.)

## 3. Autostart backend (PASS)

Sandbox harness (15/15 checks): enable writes ONLY
`io.github.kaeru.KaerU.desktop` under `$XDG_CONFIG_HOME/autostart` with
`Exec=kaeru --background`; idempotent; disable removes only that file; the
directory returns to its original contents. No other app's entries touched.

## 4. SVG + PNG rendering (PASS)

- Qt6 qsvg present: `libqt6svg6 6.2.4`, icon engine `libqsvgicon.so`.
- `svg_probe` (Qt 6.2, offscreen): `QIcon(awake.svg).pixmap()` non-null at
  16/24/32/64/128 px (was null before qtsvg was installed).
- Packaged icon resolution (`pkg_icon_probe` against the extracted .deb):
  8/8 PASS — scalable SVGs (awake/focused/sleep) + PNG fallbacks at 16/24/32/
  48/256, all non-null pixmaps.
- Resource alias fix: `qt_add_resources(... PREFIX "/" BASE resources ...)` —
  the WP-02 resource aliases never actually resolved (`:/icons/kaeru.svg`
  was not the real path); Icons.cpp now loads `:/icons/frog/*.svg` which is
  verified by the live runs (no `qt.svg` errors, tray renders).

## 5. Debian package (PASS)

Built with the standard workflow (no source signing, binary-only):

```
dpkg-buildpackage -us -uc -b   → ../kaeru_0.1.1-1_amd64.deb (361,074 bytes)
lintian kaeru_0.1.1-1_amd64.deb
  W: initial-upload-closes-no-bugs        (informational: no bug numbers
                                           exist for a first upload — accepted,
                                           documented; not suppressed)
  (no other warnings/errors; no-manual-page fixed by shipping kaeru.1)
```

Package facts (dpkg-deb -I / -c):

- Package: kaeru 0.1.1-1, Architecture: amd64, Section: utils, Priority: optional
- Depends: libc6, libgcc-s1, libqt6core6/gui6/widgets6/dbus6/sql6, libstdc++6
  (all ≥ 6.2 via shlibs) + **libqt6sql6-sqlite** (QSQLITE driver plugin) +
  **libqt6svg6** (SVG icon engine); Recommends: xdg-desktop-portal
- Contents: /usr/bin/kaeru; desktop entry; metainfo; manpage kaeru.1;
  hicolor scalable SVGs (3 frog expressions) + PNG fallbacks at
  16/24/32/48/64/128/256 (awake/focused/sleep); docs + copyright
- md5sums: 32/32 files OK; desktop-file-validate: clean (single main
  category after fix)
- No maintainer scripts at all → user data/config/cache are never touched
  on install, upgrade or removal (by construction; no conffiles)

## 6. Package lifecycle tests (safe, isolated)

`sudo` is password-unavailable in this session, so dpkg-database lifecycle
tests were done via **extraction** (`dpkg-deb -x`) + run, plus content
audits. This is documented as the honest limitation; the exact commands
below are reproducible.

- Extraction run of the packaged binary (sandbox home):
  - `kaeru --version` → `kaeru 0.1.1 (app id io.github.kaeru.Kaeru)`
  - `--background` + `status` → JSON with keepAwake/workLog sections
  - `awake on --duration 60s --mode system` → **active via portal**
    (`"backend":"portal","effective":["system-sleep"],"status":"active"`)
  - `work start --project "Pkg Test"` / `work stop` → recorded + saved
  - `export --format csv …` → 1 data row written, header correct
  - `quit` → process exits cleanly
- True `dpkg -i` / `-P` / upgrade tests require root and are **not executed**
  here (no passwordless sudo; broad system modification is out of scope per
  the task boundaries). They remain the final step once a root shell is
  available; the package has no maintainer scripts and no conffiles, so the
  data-preservation risk on upgrade/remove is structural (nothing to run).

## 7. Regression (PASS)

- Clean Release build: exit 0, zero warnings (-Wall -Wextra -Wpedantic).
- `ctest -j9`: **13/13 suites pass** (12 existing + new `test_traystate`).
- CLI verified natively (status/awake/focus/work/export/quit).
- Native inhibition re-verified after icon changes: portal active, release
  on quit (see §6 extraction run and evidence-wp02 procedure).

## 8. Honest gaps

- dpkg-database lifecycle tests (install/upgrade/remove/reinstall) — blocked
  on root; extraction-based verification done; no maintainer scripts exist
  so data preservation is structural.
- GUI screenshots of the full window — the tray/panel evidence above is the
  screenshot-based verification; full-window capture is a WP-07 manual pass.
- Ubuntu 24.04 native validation — declared environment gap (dev machine is
  Pop!_OS 22.04).