# Kaeru — Current State (0.2.0, 2026-11-16)

> Every fact below was verified by direct execution or source inspection
> during the 0.2.0 release work. Unverified items are labeled.

## 1. Repository and Git

| Item | Value |
|---|---|
| Repository | `/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru` |
| Workspace root | `/home/banana/Constellation/Idea Dev/opencode/Kaeru` (contains `frog_icons/`, `kaeru/`, documentation package) |
| Branch | `main` |
| Baseline | `d8ce6ae` — 0.1.1 handoff package |
| Release commits | `472323a` core Keep Awake · `07b81f3` UI/icons · `743ec78` CLI/autostart · `cfebbbe` packaging · `b67fc21` deferred Work Log read-only · final docs commit (`git log -1`) |
| Remote | none configured; nothing pushed |
| Working tree | clean after the documentation commit (verify with `git status`) |

Commit history (newest first):

```
<docs commit> docs: 0.2.0 delivery report and handoff update
b67fc21 work log: keep the deferred module strictly read-only
cfebbbe release: Kaeru 0.2.0 packaging and user-facing documentation
743ec78 cli: add awake indefinite and autostart startup flags
07b81f3 ui: simplify to Keep Awake and render frog icons from PNGs
472323a keep awake: single replaceable session and reliable portal activation
d8ce6ae docs: comprehensive handoff package for the next agent
```

## 2. Environment (verified)

| Component | Version |
|---|---|
| OS | Pop!_OS 22.04 LTS (Ubuntu 22.04 base), GNOME 42, X11 |
| Qt | 6.2.4 |
| CMake | 3.22.1 |
| GCC | 11.4.0 |
| debhelper | 13.6 |
| lintian | 2.114.0 |
| Session | user `banana` (uid 1000), `XDG_RUNTIME_DIR=/run/user/1000` |
| sudo | password required (no root package operations in-session) |

Primary declared target: Ubuntu 24.04 amd64 — **not tested**. Directly
tested: Pop!_OS 22.04 / X11 only.

## 3. Build state

- Build dir: `kaeru/build`, `CMAKE_BUILD_TYPE=Release`.
- Clean Release build: **succeeds with zero warnings**
  (`-Wall -Wextra -Wpedantic`).
- Binary version: `kaeru 0.2.0 (app id io.github.kaeru.Kaeru)`.
- Last verified build after the Work Log read-only change: exit 0, 0
  warnings.

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

## 4. Test state

- `ctest` in `build`: **15/15 suites pass**, ~0.2–0.4 s.
- Suites: clocks, registry, coordinator, timerengine, focus, worklog,
  storage, csv, cliparser, notify, workmanager, reports, traystate,
  **manualawake**, **icons**.
- Tests run with `QT_QPA_PLATFORM=offscreen` and `KAERU_TEST_HOME`
  sandboxes; real user data is never touched.

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru/build"
ctest --output-on-failure -j"$(nproc)"
```

## 5. Release artifacts

`dist/` is git-ignored (regenerated artifacts, not committed).

| File | Location | Size | SHA-256 / notes |
|---|---|---|---|
| `kaeru_0.2.0-1_amd64.deb` | `dist/0.2.0/` and repo parent | 328,640 B | `730bb344451a34d1530aeb7d5669cfa249116813a52269830f369cce61b3c720` — **the file normal users install** |
| `kaeru-dbgsym_0.2.0-1_amd64.ddeb` | `dist/0.2.0/debug-symbols/` and repo parent | 5,551,640 B | optional debug symbols |
| `.buildinfo` / `.changes` | `dist/0.2.0/` | — | build metadata |
| `SHA256SUMS` | `dist/0.2.0/` | — | verifies the four release files |
| `README.md` | `dist/0.2.0/` | — | install instructions and checksums |
| 0.1.1 artifacts | `dist/0.1.1/` | — | historical only |

Package audit:

- `lintian kaeru_0.2.0-1_amd64.deb` → only
  `W: initial-upload-closes-no-bugs` (informational).
- `appstreamcli validate` → successful (one pedantic note).
- `desktop-file-validate` → clean.
- `md5sum -c` → 32/32 files OK.
- Depends: Qt 6 Core/DBus/Gui/SQL/Widgets, `libqt6sql6-sqlite`; Recommends
  `xdg-desktop-portal`; no `libqt6svg6`.
- No maintainer scripts, no conffiles; user data is not touched.
- **Not tested (root required):** real `dpkg -i`/`apt install`, upgrade,
  remove, reinstall. The root-assisted procedure is in
  `docs/DELIVERY_REPORT_0.2.0.md` §9.

## 6. Runtime facts (verified live)

- GUI launched under `Xephyr :99` with the real session bus/portal and
  `KAERU_TEST_HOME` isolation; clicks driven with `xdotool`.
- 15m → 2h replacement: portal active, remaining ≈ 7,196,863 ms; no stacked
  shorter request masks it.
- Indefinitely: active, `remainingMs=-1`, reason “Keep awake (indefinite)”.
- 30m from turned-off state and 1h replacement both active with correct
  remaining time.
- Turn off: inactive with the documented limitation text.
- 5 s expiry: auto-cleared, no Kaeru inhibitor.
- Quit while active: process exits, inhibitor released.
- `kill -9` + restart: no leaked inhibitor; app reports inactive.
- Autostart: `--background --awake-on-login` starts hidden with indefinite
  Keep Awake; disabling removes only Kaeru’s own entry.
- Tray: SNI item registered; tooltip “Kaeru — keep awake active · 59m 59s”;
  5 embedded pixmaps. Panel visual under the user’s theme not re-inspected.
- Deferred Work Log: a pre-seeded SQLite database stayed byte-identical
  (same SHA-256, marker row intact) across startup, diagnostics and quit.
- Focus/Work Log CLI commands return a clear not-available message when a
  running instance is present; without one, the normal “Kaeru is not
  running” message applies.

## 7. Documentation map

- Release report: `docs/DELIVERY_REPORT_0.2.0.md` (verified vs unverified,
  root test request).
- Issue disposition: `docs/handoff/KAERU_OPEN_ISSUES.md`.
- Product/architecture: `docs/architecture.md`, `docs/uiux-design.md`,
  ADR-001…008, `PROJECT_SPEC.md`.
- Status/capabilities: `docs/capability-matrix.md`, `docs/testing.md`,
  `docs/traceability.md`, `docs/privacy.md`.
- Icons: `docs/icon-assets.md`, ADR-008.
- Historical 0.1.1 report: `docs/delivery-report.md` (superseded, retained
  for history).
- Agent rules: `AGENTS.md`.

## 8. Known contradictions resolved

1. The 0.1.1 docs described tray icons as SVG-rendered. 0.2.0 embeds PNGs;
   the docs now describe the PNG runtime path and the installed SVG.
2. The stale Overview text about a future Work Log milestone is gone; the
   page is Keep Awake only.
3. `RecoveryManager` remains a stub; the real recovery logic is in
   `WorkSessionManager::recoverInterrupted`, which is gated off while Work
   Log is deferred.
4. The 0.1.1 delivery report/quick start lists Focus/Work commands; it is
   explicitly marked historical and superseded by the 0.2.0 report.
