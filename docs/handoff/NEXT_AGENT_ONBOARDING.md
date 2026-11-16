# Kaeru — Next-Agent Onboarding (0.2.0)

> Start here. Read this file, then `docs/handoff/README.md`, then the
> documents it points to. The 0.2.0 release candidate is built and verified;
> **user acceptance and root package lifecycle testing are still pending.**

## 0. Honest status

The 0.1.1 owner reports were handled as follows in 0.2.0:

- **Keep Awake 2h/Indefinite:** fixed with a single replaceable manual
  session; verified with real GUI clicks and the live portal.
- **Focused frog icon:** fixed for runtime rendering by embedding the
  supplied PNG variants; verified by tests and live tray pixmaps.
- **Two `.deb` files:** delivery layout now has one main package and an
  isolated optional `debug-symbols/` folder.
- **Focus Timer:** deferred (hidden from GUI/CLI); owner rejection stands.
- **Work Log:** deferred and made strictly read-only; the preserved database
  is not opened or written.

The release is **not user-accepted**, and the package has **not** been
installed/upgraded/removed with root. Do not treat this file or the passing
tests as acceptance. See `docs/DELIVERY_REPORT_0.2.0.md` §8–9.

## 1. Paths and layout

| What | Path |
|---|---|
| Application repo (git) | `/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru` |
| Workspace root | `/home/banana/Constellation/Idea Dev/opencode/Kaeru` |
| Original frog artwork (read-only) | `…/Kaeru/frog_icons/` |
| Documentation package (authoritative spec) | `…/Kaeru/Kaeru_Documentation_Package_v0.1.1/` |
| Build dir | `…/Kaeru/kaeru/build` |
| Release artifacts | `…/Kaeru/kaeru/dist/0.2.0/` (`dist/` is git-ignored) |
| Main package | `dist/0.2.0/kaeru_0.2.0-1_amd64.deb` |
| Optional debug symbols | `dist/0.2.0/debug-symbols/kaeru-dbgsym_0.2.0-1_amd64.ddeb` |

## 2. Environment and dependencies

- Pop!_OS 22.04 / Ubuntu 22.04 base, GNOME 42, X11 (Qt 6.2.4); CMake ≥ 3.16,
  GCC 11, `qt6-base-dev`, `libqt6sql6-sqlite`, `debhelper`, `lintian`,
  `desktop-file-utils`, `appstream`.
- `sudo` requires a password — root package operations are blocked in a
  plain session; ask the owner to run them.
- Use `KAERU_TEST_HOME` sandboxes for every test; never point tests at real
  `$XDG_*` directories.
- GUI acceptance harness: `Xephyr :99 -screen 1200x800`, `DISPLAY=:99`,
  real session bus/portal, `xdotool` clicks, `ffmpeg` screenshots.

## 3. Reading order

1. This file.
2. `docs/handoff/README.md` — handoff index.
3. `docs/DELIVERY_REPORT_0.2.0.md` — release scope, evidence, unverified gaps.
4. `docs/handoff/KAERU_CURRENT_STATE.md` — exact build/git/test/package state.
5. `docs/handoff/KAERU_OPEN_ISSUES.md` — A–E disposition and remaining gaps.
6. `docs/handoff/KAERU_TECHNICAL_ARCHITECTURE.md` — subsystem walkthrough.
7. `docs/capability-matrix.md`, `docs/testing.md`, ADRs as needed.

## 4. Build / test / run

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru"

# Configure + build (Release)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

# Tests (15 suites, offscreen, sandboxed)
cd build && ctest --output-on-failure -j"$(nproc)"

# Run the application
./build/kaeru                # GUI
./build/kaeru --background   # tray/background
./build/kaeru --awake-on-login   # hidden + indefinite (autostart form)
./build/kaeru status [--json]
./build/kaeru awake on --duration 30m --mode both
./build/kaeru awake indefinite
./build/kaeru awake off
./build/kaeru diagnostics [--json]
./build/kaeru open | quit

# Rebuild the Debian package
dpkg-buildpackage -us -uc -b
lintian ../kaeru_0.2.0-1_amd64.deb
```

Focus, Work Log, Export and Pause-all are **not** available in this release;
the CLI returns a not-available message when a running instance is present.

## 5. Debugging

- Capture the process output: `./build/kaeru --background > kaeru.log 2>&1`.
- `KAERU_TEST_HOME=/tmp/opencode/sandbox` isolates config/data/cache/runtime.
- `KAERU_DEBUG_SETTINGS=1` prints settings decisions to stderr.
- `KAERU_DEBUG_PORTAL=1` prints portal handle subscription and response
  lines (gated; safe to leave off).
- D-Bus: `busctl --user list | grep kaeru`, introspect
  `io.github.kaeru.Kaeru`; the tray item is registered under the app’s unique
  bus name (`:1.x/StatusNotifierItem`), not a fixed well-known name.
- Portal inhibitors do **not** appear in `systemd-inhibit --list` on this
  system; use `kaeru status --json`, DBus signals and absence after release.
- SQLite: do not open the real database while Work Log is deferred. Use a
  sandbox copy if inspecting historical data.

## 6. Source map (entry points)

| Concern | Files |
|---|---|
| Entry / modes / single instance | `src/app/main.cpp`, `src/app/CliParser.{h,cpp}`, `src/platform/SingleInstance.{h,cpp}` |
| Wiring root | `src/app/Application.{h,cpp}` |
| Single manual Keep Awake session | `src/core/ManualAwakeSession.{h,cpp}` |
| Keep Awake registry/coordinator | `src/core/InhibitionRegistry`, `InhibitionCoordinator`, `InhibitionBackend.h`, `FakeInhibitionBackend` |
| Keep Awake backends | `src/platform/PortalInhibitBackend`, `LogindInhibitBackend` |
| Commands (UI/CLI/D-Bus share) | `src/app/CommandRouter.{h,cpp}`, `ControlService.{h,cpp}` |
| Settings / paths / autostart | `src/app/Settings`, `Paths`, `platform/AutostartBackend` |
| Clocks/timers | `src/core/Clock`, `TimerEngine` |
| Deferred Focus | `src/core/FocusController`, `src/ui/pages/FocusPage` (compiled, not shown) |
| Deferred Work Log | `src/app/WorkSessionManager`, `src/storage/SessionRepository` (gated/read-only) |
| Storage/migrations/CSV | `src/storage/*` (not opened while Work Log is deferred) |
| UI shell/pages | `src/ui/MainWindow`, `src/ui/TrayController`, `src/ui/pages/{Overview,Settings,Diagnostics}Page` |
| Icons (frog) | `src/ui/Icons.{h,cpp}`, `resources/icons/frog/` (embedded PNGs; installed SVGs) |
| Build/install | `CMakeLists.txt`, `debian/*`, `kaeru.1` |
| Tests | `tests/unit/test_*.cpp` (15 suites) |

## 7. Evidence locations

- `docs/DELIVERY_REPORT_0.2.0.md` — primary 0.2.0 evidence and limitations.
- `docs/handoff/KAERU_OPEN_ISSUES.md` — per-issue disposition.
- `docs/evidence-wp02..06.md` — earlier native evidence (0.1.1 scope).
- `dist/0.2.0/README.md`, `SHA256SUMS` — release artifacts.
- `/tmp/opencode/` — throwaway probes/screenshots from this session (not in
  the repo).

## 8. Change procedure and approval boundary

- Follow `AGENTS.md`: core has no GUI/DBus/SQL; platform behind interfaces;
  all user commands through `CommandRouter`; tests in `KAERU_TEST_HOME`;
  build green; packaging via `debian/` only; never synthesize input; never
  log private content.
- **Do not re-enable Focus or Work Log in the GUI/CLI** without an explicit
  product decision, a new design for Focus, and agreed recording semantics
  for Work Log. Their source/tests/data must stay preserved until then.
- Do not modify `../frog_icons/` or the approved Master Specification.
- Do not rewrite Git history or push.
- Do not weaken tests; add coverage for new behavior.
- Product-owner approval is required for Keep Awake semantics changes, Focus
  redesign, Work Log semantics, and packaging behavior changes.

## 9. What to do first

1. Run the root-assisted package lifecycle test in
   `docs/DELIVERY_REPORT_0.2.0.md` §9 (install, upgrade, remove, reinstall;
   verify data preservation).
2. Ask the owner to use the 0.2.0 GUI and confirm the Keep Awake flows and
   the simplified three-page layout.
3. If accepted, decide the next feature: Focus redesign or Work Log
   semantics — one at a time, with a written acceptance plan.
4. Only then re-enable a deferred module, restoring its UI/CLI paths and
   tests behind an explicit opt-in.
