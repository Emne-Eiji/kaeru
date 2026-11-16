# Kaeru 0.1.1 — Final Delivery Report

**Date:** 2026-11-16 · **Work orders executed:** WP-00 … WP-07 (autonomous
execution authorized 2026-11-15)
**Repository:** `kaeru/` (this directory) · **License:** GPL-3.0-or-later

## 1. What was delivered

Kaeru is a Qt 6 (Widgets) keep-awake / focus-timer / work-log desktop utility
for Linux (GNOME-first, X11 validated), delivered as a single binary with
GUI, background (tray) and CLI/D-Bus modes.

| Work order | Status | Delivered |
|---|---|---|
| WP-00 environment report | done | `docs/wp00-environment-report.md` |
| WP-01 design package | done | architecture, ADR-001…007, uiux-design, capability matrix, privacy/testing/traceability docs, execution plan, PROJECT_SPEC |
| WP-02 keep-awake core | done | inhibition registry/coordinator, portal + logind backends, clocks, storage layer, six-page UI, tray, packaging metadata |
| WP-03 tray / single instance / autostart | partial | tray live-verified (SNI), single-instance forwarding verified, autostart sandbox-verified; **`.deb` packaging blocked** (missing toolchain — see §6) |
| WP-04 focus + notifications | done | full focus state machine, quiet-hours policy, sound hint; native notification captured |
| WP-05 work log | done | sessions/segments/projects, SQLite + migrations, recovery checkpoints, lock/suspend pause, conservative crash recovery; native lifecycle verified |
| WP-06 reports + CSV | done | reports page (filters, daily/project totals, focus time), CLI/D-Bus CSV export with formula-injection guard; native export verified |
| WP-07 hardening + release | done | zero-warning Release build, 12/12 test suites, hardening audit (no network/telemetry/process-spawn), resource measurement, staged tarball + checksums; `.deb` pending toolchain |

## 2. Evidence (real, not mocked — per spec §14)

Every claim below has exact commands and expected/actual in its evidence file:

- **Native inhibition (portal + logind):** `docs/evidence-wp02.md` —
  portal `Inhibit(flags=8)` → `Response(0)` → active → `Request.Close`;
  logind fallback verified with a real `sleep:block` inhibitor in
  `systemd-inhibit --list` (appears while held, gone after release);
  expiry → inactive; quit → all inhibitors released.
- **Tray / single instance / autostart:** `docs/evidence-wp03.md` — SNI item
  `:1.149/StatusNotifierItem` on the live GNOME 42 session, state-reflecting
  tooltip, DBusMenu; second launch forwards + exits 0; autostart backend
  15/15 sandbox checks.
- **Focus notifications + quiet hours:** `docs/evidence-wp04.md` — 1
  notification on interval completion (dbus-monitor capture); 0 during
  all-day quiet hours with config preserved (caught + fixed a settings-clobber
  bug in the process).
- **Work log lifecycle + crash recovery:** `docs/evidence-wp05.md` —
  start/pause/resume/stop via CLI; DB inspection shows exactly 2
  non-overlapping segments, closed session; `kill -9` recovery closes the
  session conservatively with an honest note (no invented work, checkpoints
  never duplicate report entries).
- **Reports + CSV export:** `docs/evidence-wp06.md` — real
  `kaeru export --format csv` with row-count summary; formula-injection
  guard verified natively; `duration_minutes` rounded down, never up.

## 3. Test results (final regression, clean Release build)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j12
  → exit 0, ZERO warnings (-Wall -Wextra -Wpedantic)
ctest -j9 → 12/12 suites passed:
  test_clocks · test_registry · test_coordinator · test_timerengine ·
  test_focus · test_worklog · test_storage · test_csv · test_cliparser ·
  test_notify · test_workmanager · test_reports
```

All tests run with `QT_QPA_PLATFORM=offscreen` and an isolated
`KAERU_TEST_HOME` sandbox — the user's real data is never touched.

## 4. Resource usage (idle, background mode)

- RSS: **55.7 MB** measured at startup (target < 100 MB) — within target.
- Idle CPU: 1 Hz tick only (countdown displays); measured 300 s delta:
  **790 ms total ≈ 0.26% of one core** (target < 0.5%) — within target.
- Poll audit: single 1 s `QTimer`; no busy loops; notifications/icons load
  lazily; `tick()` skips work when idle (no timers, no session, no
  inhibition requests).

## 5. Release candidate artifacts

- **Source:** this repository (commit tagged in §8).
- **Staged Linux tarball:** `dist/kaeru-0.1.1-Linux-amd64.tar.gz`
  (bin/kaeru + .desktop + hicolor icon + metainfo + README/LICENSE;
  installs under a `kaeru-0.1.1/` prefix — copy `bin/kaeru` into PATH and
  `share/*` into `$XDG_DATA_DIRS` or `$HOME/.local/share`).
- **Checksums:** `dist/kaeru-0.1.1-Linux-amd64.tar.gz.sha256`
  (`sha256sum -c`).
- **Build from source:**
  `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
  sudo cmake --install build --prefix /usr` (desktop deps: Qt 6 Widgets,
  qtsvg; runtime: libqt6sql6-sqlite, notification daemon, logind, portal).

## 6. Honest gaps (no unsupported claims)

1. **`.deb` package NOT built** — `debhelper`/`lintian`/`devscripts` are not
   installed and `sudo apt install` needs a password this session could not
   supply. The `debian/` packaging will be added and the
   install/upgrade/remove/reinstall cycle executed once the user runs:
   `sudo apt install -y debhelper lintian devscripts`
   (the exact packages authorized on 2026-11-15). Everything else (install
   rules, desktop file, icons, metainfo, README/LICENSE) is already in
   CMakeLists and staged in the tarball.
2. **Icons render blank on THIS dev box** — Qt6 qsvg (`libqt6svg6`) is not
   installed locally. The icon engine is declared as a runtime dependency for
   packaging, so normal desktops render the SVG icons; local re-test needs
   `sudo apt install -y libqt6svg6`.
3. **Real suspend/resume not auto-triggered** (user session protection).
   Boot-clock suspend-inclusive countdowns are verified with injected clocks
   and the logind inhibitor lifecycle; the manual real-suspend procedure is
   in `docs/testing.md` §6.1.
4. **Ubuntu 24.04 not validated** — dev machine is Pop!_OS 22.04 (Ubuntu
   22.04 base). No unsupported compatibility claim is made; Qt 6.2 floor APIs
   only.
5. **Audible notification sound** — delegated to the daemon via the XDG
   `sound-name` hint; audible verification is manual.
6. **GUI automation** — Focus/Work Log/Reports/Settings pages are wired and
   compile; widget-level behavior is covered by the underlying logic tests;
   screenshot-driven GUI regression is a WP-07 manual pass (tray screenshot
   captured at /tmp/opencode/tray-full.png during WP-03).

## 7. Security / privacy posture (audit results)

- No network code (no sockets/HTTP anywhere in `src/`).
- No `system()`/`popen()`/`QProcess` process spawning.
- No telemetry or analytics; private fields (task/tag/note text) are never
  logged or exported (CSV contains only session/project/timing data).
- All SQL parameterized (`prepare`+`bindValue`); static queries carry no
  user input.
- Private data files get owner-only permissions (`0600`) on write
  (database + settings).
- Tests run only in `KAERU_TEST_HOME` sandboxes.

## 8. Commits

```
bd3b4c6 WP-01 design package
53ad601 WP-02 core
21cc621 WP-02 storage/UI/tests/evidence
b898cb9 WP-03 (partial) tray/single-instance/autostart
0c46ad0 test_clocks flake fix
a593f05 WP-04 focus notifications + settings-view load fix
400291a WP-05 work log end-to-end
7f181e2 WP-06 reports + CSV export
bc40365 WP-07 hardening, resource measurement, delivery report (this commit)
```

## 9. How to use (quick start)

```
kaeru                          # GUI
kaeru --background             # tray/background
kaeru awake on --duration 30m  # keep awake 30 min (system+display)
kaeru awake off                # turn off now
kaeru focus start [--duration 25m] | pause | resume | skip | stop
kaeru work start --project X | pause | resume | stop
kaeru export --format csv --from 2026-11-01 --to 2026-11-30
kaeru status | diagnostics | pause-all | quit
```