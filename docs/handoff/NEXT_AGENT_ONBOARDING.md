# Kaeru — Next-Agent Onboarding

> Start here. Read this file, then `README.md` (handoff index), then the
> documents it points to in order.

## 0. First, the honest status warning

**The previous agent's declaration that WP-00 through WP-07 are "complete"
does not mean Kaeru has passed user acceptance testing.** The owner has
tested the application in the real world and rejected several behaviors
(two `.deb` confusion, Keep Awake 2h/indefinite, Focus Timer UX,
Work Log premature recording, broken focused tray icon). Those issues are
documented in `KAERU_OPEN_ISSUES.md` and remain **unresolved**. Do not
blindly trust prior completion claims, and never weaken tests to get green.

## 1. Paths and layout

| What | Path |
|---|---|
| Application repo (git) | `/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru` |
| Workspace root | `/home/banana/Constellation/Idea Dev/opencode/Kaeru` |
| Original frog artwork (read-only) | `…/KaerU/frog_icons/` |
| Documentation package (authoritative spec) | `…/KaerU/KaerU_Documentation_Package_v0.1.1/` |
| Build dir | `…/KaerU/kaeru/build` |
| Debian artifacts | `…/KaerU/kaeru_0.1.1-1_amd64.deb` (+ `kaeru-dbgsym_0.1.1-1_amd64.ddeb`) and `…/KaerU/kaeru/dist/` |

## 2. Environment and dependencies

- Ubuntu-based Linux (dev machine: Pop!_OS 22.04, GNOME 42, X11).
- Qt 6.2 development packages (`qt6-base-dev`), CMake ≥ 3.16, C++17
  compiler (GCC 11 works), `libqt6svg6` (SVG icon engine), `debhelper`,
  `lintian`, `devscripts` (packaging), `desktop-file-utils` (validation).
- `sudo` currently requires a password — plan around it (no root-level
  package operations in a plain session).
- All tests and test scripts must use `KAERU_TEST_HOME` sandboxes; never
  point tests at the real `$XDG_*` directories.

## 3. Reading order (authoritative documents)

1. This file (onboarding).
2. `docs/handoff/README.md` — index of the handoff package.
3. `docs/handoff/KAERU_PROJECT_HISTORY.md` — what Kaeru is, WP-00…07.
4. `docs/handoff/KAERU_TECHNICAL_ARCHITECTURE.md` — how the code works.
5. `docs/handoff/KAERU_CURRENT_STATE.md` — exact build/git/test/package state.
6. `docs/handoff/KAERU_OPEN_ISSUES.md` — the four owner-reported issues + status register.
7. `docs/capability-matrix.md` — per-capability honesty matrix.
8. `docs/testing.md`, `docs/privacy.md`, ADRs as referenced.

## 4. Build / test / run (exact commands)

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru"

# Configure + build (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

# Run the tests (13 suites, offscreen, sandboxed)
cd build && ctest --output-on-failure -j9

# Run the application
./build/kaeru                # GUI
./build/kaeru --background   # tray/background
./build/kaeru status         # CLI (needs a running instance via D-Bus)
./build/kaeru awake on --duration 30m --mode system
./build/kaeru focus start --duration 25m
./build/kaeru work start --project "Example"
./build/kaeru export --format csv --from 2026-11-01 --to 2026-11-30
./build/kaeru quit

# Rebuild the Debian package
dpkg-buildpackage -us -uc -b
lintian ../kaeru_0.1.1-1_amd64.deb

# Rebuild the staged tarball
cmake --install build --prefix /tmp/kaeru-stage
```

## 5. Debugging

- Process logs: the app prints nothing by default; capture stdout/stderr
  of the process you launch (e.g. `./build/kaeru --background > kaeru.log 2>&1`).
- `KAERU_DEBUG_SETTINGS=1` — prints the settings file path and key
  decisions (set quiet hours etc.) to stderr.
- `KAERU_TEST_HOME=/tmp/somewhere ./build/kaeru …` — isolate all XDG
  paths (config/data/cache/runtime) for experiments; this is how every
  test and integration script works.
- D-Bus inspection: `busctl --user list | grep kaeru`, `busctl --user
  introspect io.github.kaeru.KaReru /io/github/kaeru/KaerU`, `dbus-monitor
  --session "interface='org.freedesktop.Notifications'"`.
- Inhibition checks: `systemd-inhibit --list | grep -i kaeru`;
  portal traffic via `dbus-monitor --session "interface='org.freedesktop.portal.Inhibit'"`.
- SQLite: `sqlite3 "$XDG_DATA_HOME/kaeru/kaeru.db" "SELECT …"` (only in
  sandboxes or with explicit user approval for real data).

## 6. Source map (entry points)

| Concern | Files |
|---|---|
| Entry / modes / single instance | `src/app/main.cpp`, `src/app/CliParser.{h,cpp}`, `src/platform/SingleInstance.{h,cpp}` |
| Wiring root | `src/app/Application.{h,cpp}` |
| Commands (UI/CLI/D-Bus share) | `src/app/CommandRouter.{h,cpp}`, `src/app/ControlService.{h,cpp}` |
| Settings / paths | `src/app/Settings.{h,cpp}`, `src/app/Paths.{h,cpp}` |
| Keep Awake core | `src/core/InhibitionRegistry`, `InhibitionCoordinator`, `InhibitionBackend.h`, `FakeInhibitionBackend` |
| Keep Awake backends | `src/platform/PortalInhibitBackend`, `LogindInhibitBackend` |
| Clocks/timers/focus | `src/core/Clock`, `TimerEngine`, `FocusController` |
| Tray state mapping | `src/core/TrayState.h` (ADR-008) |
| Work Log | `src/app/WorkSessionManager.{h,cpp}`, `src/storage/SessionRepository.{h,cpp}` |
| Storage/migrations/CSV | `src/storage/Database`, `Migrations`, `ProjectRepository`, `CsvExport` |
| UI shell/pages | `src/ui/MainWindow`, `src/ui/TrayController`, `src/ui/pages/*` |
| Icons (frog) | `src/ui/Icons.{h,cpp}`, `resources/icons/frog/` |
| Build/install | `CMakeLists.txt`, `debian/*`, `kaeru.1` |
| Tests | `tests/unit/test_*.cpp` (13 suites) |

## 7. Evidence locations

- `docs/evidence-wp02..06.md` — native evidence with exact commands.
- `docs/evidence/` — tray icon/panel-diff crops (safe, committed).
- `docs/delivery-report.md` — release candidate report.
- `/tmp/opencode/` on this machine holds throwaway probes/scripts from the
  project sessions (not part of the repo).

## 8. Change procedure and approval boundary

- Follow `AGENTS.md`: core has no GUI/DBus/SQL; platform behind
  interfaces; all user commands through `CommandRouter`; tests in
  `KAERU_TEST_HOME` sandboxes; build green at each commit; packaging via
  `debian/` only; never synthesize input; never log private content.
- **Product-owner approval is required** before: redesigning the Focus
  module (Issue C), changing work-log recording semantics (Issue D),
  changing keep-awake stacking/duration semantics (Issue B), replacing or
  regenerating the frog artwork (Issue E fix direction), or any
  packaging-behavior change (Issue A).
- Bug fixes that do not change product behavior (e.g. the refresh-hides-
  warning defect, resource loading) may proceed with tests, but the
  owner's four issues should be discussed first — several need decisions,
  not just code.
- Do not weaken or delete tests; add tests for new behavior.
- Commit style: small, logical, message describing the change; the
  history is linear on `main` (no branches, no remotes).

## 9. What to do first (suggested order, subject to owner direction)

1. Reproduce Issue E (focused icon) with a fresh build and confirm the
   black-face rendering; propose a fix (PNG fallback or SVG restructure)
   to the owner.
2. Reproduce Issue B with the GUI (stacked durations + Indefinitely) and
   capture evidence; propose semantics (replace vs stack).
3. Discuss Issues C and D with the owner before touching code.
4. Clarify Issue A delivery instructions (single-file install).
5. Only then: fix, test, document, commit — one coherent change at a time.