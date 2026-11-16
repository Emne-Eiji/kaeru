# Kaeru 0.1.1 — Final Delivery Report

> **Historical document.** This report describes 0.1.1. It is superseded by
> `docs/DELIVERY_REPORT_0.2.0.md`, which records the release scope changes,
> issue dispositions and 0.2.0 evidence. The 0.1.1 commands below for Focus
> and Work Log are **not available** in 0.2.0.

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
| WP-03 tray / single instance / autostart | **done** | tray live-verified (SNI, frog states), single-instance forwarding verified, autostart sandbox-verified, **`.deb` built and lintian-clean**, packaged binary verified via extraction — `docs/evidence-wp03.md` |
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
  on the live GNOME 42 session with the official frog expressions
  (tooltip + pixmap + panel-diff verified in all five states), second launch
  forwards + exits 0, autostart backend 15/15 sandbox checks.
- **Debian package (WP-03 complete):** `kaeru_0.1.1-1_amd64.deb` (361 KB,
  amd64) built with debhelper via `dpkg-buildpackage -us -uc -b`;
  `lintian`: only the informational `initial-upload-closes-no-bugs`
  (accepted, documented); md5sums 32/32 OK; `desktop-file-validate` clean;
  packaged binary verified via extraction (version/awake-via-portal/work
  log/CSV export/clean quit); icon resolution 8/8 from the installed
  hicolor tree.
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

- **Source:** this repository (see §8 for the release commits).
- **Debian package:** `dist/kaeru_0.1.1-1_amd64.deb` (361,074 bytes,
  amd64) — built with `dpkg-buildpackage -us -uc -b`; `lintian` clean except
  the informational first-upload note; md5sums 32/32 OK; install with
  `sudo apt install ./kaeru_0.1.1-1_amd64.deb` (see evidence-wp03 §5).
- **Staged Linux tarball:** `dist/kaeru-0.1.1-Linux-amd64.tar.gz`
  (bin/kaeru + .desktop + hicolor icons + metainfo + README/LICENSE;
  installs under a `kaeru-0.1.1/` prefix — copy `bin/kaeru` into PATH and
  `share/*` into `$XDG_DATA_DIRS` or `$HOME/.local/share`).
- **Checksums:** `dist/kaeru-0.1.1-Linux-amd64.tar.gz.sha256`
  (`sha256sum -c`).
- **Build from source:**
  `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
  sudo cmake --install build --prefix /usr` (desktop deps: Qt 6 Widgets,
  qtsvg; runtime: libqt6sql6-sqlite, notification daemon, logind, portal).

## 6. Honest gaps (no unsupported claims)

1. **dpkg-database lifecycle tests NOT run** — `dpkg -i` / upgrade / remove /
   reinstall require root and this session has no sudo password. The `.deb`
   itself is built and verified (lintian, md5sums, desktop-validate,
   extraction-run, icon resolution); the package has **no maintainer scripts
   and no conffiles**, so data preservation across upgrade/remove is
   structural. Run the cycle once a root shell is available:
   `dpkg -i ../kaeru_0.1.1-1_amd64.deb` → launch → `dpkg -r kaeru` →
   verify `~/.config/kaeru` untouched.
2. **Icons render desaturated on THIS dev box** — the user's
   `desaturated-tray-icons` GNOME extension greyscales all tray icons; the
   frog pixmap data itself carries the official colors (verified via D-Bus
   IconPixmap). The package installs both SVG and PNG variants; icon
   rendering itself is verified (8/8 resolution + live tray).
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
5ebecee WP-07 hardening, resource measurement, delivery report
26b5a6d Official frog icon identity (ADR-008): tray states, launcher, resources
82e2df6 Debian packaging (WP-03 completion): debhelper + lintian-clean .deb
dce5ae7 WP-03 completion evidence and documentation (this commit)
```

## 9. How to use (quick start)

> Superseded by 0.2.0. Use the commands in `README.md` or
> `docs/DELIVERY_REPORT_0.2.0.md`; Focus, Work Log and Export are deferred.

```
kaeru                          # GUI
kaeru --background             # tray/background
kaeru awake on --duration 30m  # keep awake 30 min (system+display)
kaeru awake indefinite         # keep awake until turned off
kaeru awake off                # turn off now
kaeru status | diagnostics | open | quit
```