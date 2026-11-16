# Kaeru — Testing Strategy and Evidence Plan

**Rule:** every recorded test lists exact command, environment, expected,
actual, pass/fail/not-run, evidence path, limitations. Mocked success is
never labeled as native validation. Passing tests are not user acceptance.

This document reflects **0.2.0** (Keep Awake release). Sections covering
Focus/Work Log remain for the preserved code and future re-enablement.

## 1. Unit tests (Qt Test, offscreen, injected clocks — deterministic)

| Suite | Covers |
|---|---|
| test_clocks | monotonic vs boot vs wall semantics; fake clocks; suspend simulation by advancing fake clocks; duration/date parsing (accept + reject) |
| test_registry | add/remove/expire (duration + until), earliestRemaining, priority reasons, clear + changed signal |
| test_coordinator | capability union/diff, acquire/release against fake backend, backend loss/denial/failure, suppression, module disable, Pause All (no auto-restart), requested≠effective, expired signal carries the original request payload |
| test_manualawake | single manual session: timed/indefinite/until factories, mode→capabilities, replacement, stop, remaining time, invalid requests (8 cases) |
| test_timerengine | countdown expiry (boot), stopwatch suspend pause (monotonic), wall-deadline, fires-once, cancel, invalid inputs |
| test_focus | Pomodoro transitions, pause freeze/resume, skip/extend, upcoming-work override, stop cancels, quiet-hours windows — preserved for the deferred module |
| test_notify | notification policy decisions (notify option, quiet hours, keep-awake expiry gate) |
| test_worklog | open-segment policy, paused exclusion, no-overlap, single active session, checkpoints never counted, focus sessions separate, daily totals offset — preserved code |
| test_workmanager | work lifecycle, idempotent start, project switch, suspend/lock boundaries, conservative recovery, focus→focus_sessions — preserved code |
| test_storage | migrations versioned/idempotent, project CRUD + guards, session round-trip, FK integrity, backup-before-migration, WAL + integrity — preserved code |
| test_csv | quoting, formula-injection neutralization, header + open-segment rows, write/truncate + row count, filename convention — preserved code |
| test_reports | per-project totals, focus totals, open segments never count — preserved code |
| test_traystate | tray presentation mapping: all state combinations, precedence, honesty rules (ADR-008) |
| test_cliparser | grammar, malformed values rejected, defaults, missing operands, combined startup flags, flags-vs-command rejection |
| test_icons | all 21 embedded PNG variants exist at the right size; state icons expose raster sizes; expressions are distinct; focused artwork retains green/light pixels |

Run:

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru/build"
ctest --output-on-failure -j"$(nproc)"
```

Result at release: **15/15 suites pass** on the clean Release build, ~0.2–0.4 s.
The suites set `QT_QPA_PLATFORM=offscreen` and isolate data with
`KAERU_TEST_HOME`; the real user home is never touched.

## 2. GUI acceptance (real workflows, 0.2.0)

Harness used:

```bash
Xephyr :99 -screen 1200x800 -no-host-grab &
export DISPLAY=:99
export KAERU_TEST_HOME=/tmp/opencode/kaeru-accept
export XDG_RUNTIME_DIR=/run/user/1000
./build/kaeru &
# clicks via xdotool; screenshots via ffmpeg -f x11grab
```

This is a real Qt window with the real session bus and portal; only the
display is nested. Clicks used the stable layout (chips at y≈314; Turn off
at ≈(313,378) in the 1200×800 harness).

| Workflow | Expected | Result |
|---|---|---|
| Click 15 minutes | active, portal backend, both capabilities, ~15 min remaining | pass |
| Click 2 hours while 15m active | replaces 15m; remaining ~1 h 59 m 57 s | pass |
| Click Indefinitely while timed active | `remainingMs=-1`, reason indefinite, active | pass |
| Turn off | inactive, no backend, documented limitation text | pass |
| Click 30m after turn-off | suppression cleared, active ~30 m | pass |
| Click 1h after 30m | replaces 30m, active ~1 h | pass |
| 5 s timer | auto-clears at expiry, inactive, no inhibitor | pass |
| Quit while active | process exits, inhibitor released | pass |
| `kill -9` then restart | no leaked inhibitor; app reports inactive | pass |
| Autostart flags | hidden start, indefinite active; `kaeru open` shows window | pass |
| Settings → Startup | writes/removes only Kaeru's own autostart entry | pass |
| Tray (SNI properties) | item registered, tooltip with state/remaining, 5 pixmaps | pass (properties; panel visuals not re-inspected) |

Screenshots from the session are in `/tmp/opencode/` (throwaway, not
committed). The delivery report records the exact observed status values.

## 3. Deferred-module read-only verification (0.2.0)

- Pre-seed a sandbox database with a marker row, record its SHA-256.
- Start Kaeru with `--background`, call `diagnostics --json`, then quit.
- Observed: hash unchanged, marker row intact, diagnostics
  `work-log-storage.status = deferred`.
- This proves the deferred Work Log does not open, migrate, checkpoint or
  close the preserved database.

## 4. Integration tests (real desktop)

| Test | Method | Evidence |
|---|---|---|
| Portal inhibit acquire/release | GUI durations + `kaeru status --json` | active with `portal` backend; inactive and no Kaeru inhibitor after release/turn-off/quit |
| Logind sleep inhibit | 0.1.1 evidence; portal is the primary path in 0.2.0 | `docs/evidence-wp02.md`; not re-run as a standalone test in 0.2.0 |
| Denial/failure path | unit-tested with fake backend | `test_coordinator`; live portal denial not tested in 0.2.0 |
| Timed expiry | 5 s request | auto-cleared; no inhibitor |
| Suppression | turn off, then choose a duration | new duration clears suppression and activates |
| Quit cleanup | quit while active | inhibitor released |
| Single instance | second launch | forwards to the running instance (0.1.1 evidence; unchanged path) |
| Autostart toggle | Settings → Startup | file created/removed; exact Exec verified |
| Tray presence | SNI property query | registered item, tooltip, pixmaps |
| CLI/IPC | all 0.2.0 verbs | meaningful output/exit codes; deferred commands report not available |
| Real suspend/resume | **manual only** | not run; user session protection |
| Audible notification | **manual only** | not observed |

Note: portal inhibitors do not appear in `systemd-inhibit --list` on this
system; app status/DBus evidence is authoritative, and the command is used
only to confirm no Kaeru logind inhibitor remains.

## 5. Packaging tests

- `dpkg-buildpackage -us -uc -b` clean build.
- `lintian kaeru_0.2.0-1_amd64.deb` → only informational
  `initial-upload-closes-no-bugs`.
- `appstreamcli validate --no-net` → successful (one pedantic note).
- `desktop-file-validate` → clean.
- Package md5sums → 32/32 OK; SHA-256 recorded in `dist/0.2.0/SHA256SUMS`.
- Extract-run of `/usr/bin/kaeru` from the `.deb` → reports 0.2.0.
- **Not run (root required):** `dpkg -i`/`apt install`, upgrade, remove,
  reinstall, and user-data preservation across those operations. Procedure
  and request are in `docs/DELIVERY_REPORT_0.2.0.md` §9.

## 6. Resource measurement (0.1.1, not repeated in 0.2.0)

- Idle CPU target < 0.5 % of one core; RSS target < 100 MB background.
- 0.1.1: RSS 55.6 MB, idle CPU 0.26 % over 300 s.
- 0.2.0 changes are not expected to regress this materially, but no fresh
  measurement was taken; do not cite the old numbers as 0.2.0 evidence.

## 7. Known test limitations (declared)

1. Real suspend/resume not auto-triggered — manual procedure only.
2. Ubuntu 24.04 native validation not performed (no machine).
3. Wayland/KDE not evaluated.
4. Tray menu cannot be fully script-driven; evidence = SNI properties,
   tooltips, pixmaps and screenshots.
5. DST machine-clock change not performed; validated via TZ injection.
6. Root package lifecycle tests blocked by sudo password.
7. Audible notification playback is daemon-side and was not observed.
