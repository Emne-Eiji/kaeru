# Kaeru — Testing Strategy and Evidence Plan

**Rule:** every recorded test lists exact command, environment, expected, actual, pass/fail/not-run, evidence path, limitations. Mocked success is never labeled as native validation.

## 1. Unit tests (Qt Test, offscreen, injected clocks — deterministic)

| Suite | Covers |
|---|---|
| test_clocks | monotonic vs boot vs wall semantics; fake clocks; suspend simulation by advancing fake clocks |
| test_registry | overlap/expiry, suppression, module disable cleanup, precedence order, priorities |
| test_coordinator | capability union/diff, acquire/release wiring against fake backend, backend loss/denial/failure → requested vs effective state |
| test_timerengine | countdown expiry (boot), stopwatch suspend pause (monotonic), wall-deadline recompute under TZ/DST/clock changes, fires-once guarantee |
| test_focus | Pomodoro transitions, extend/skip, auto-next off by default, keep-awake linkage, focus→worklog accrual excluding suspend |
| test_worklog | one active session, project switch transaction, suspend/lock pause, segment math, revisions/provenance, crash recovery from checkpoints (no invented time) |
| test_storage | migrations v1 (+v2 forward), constraints (unique active session, non-negative durations, FK), WAL, backup-before-migration |
| test_csv | escaping (quotes/commas/newlines), formula injection neutralization (=,+,−,@,tab,CR prefixes) |
| test_cliparser | grammar, malformed values rejected, defaults, exit-code mapping |

Run: `ctest` / each test binary with `QT_QPA_PLATFORM=offscreen` and `KAERU_TEST_HOME` sandbox.

## 2. Integration tests (real desktop, primary environment)

| Test | Method | Native evidence |
|---|---|---|
| Logind sleep inhibit acquire/release | `kaeru awake on --duration 10s --mode system` | `systemd-inhibit --list` shows kaeru sleep:block while active and not after; `busctl get-property …CanSuspend` = inhibited during, yes after |
| Portal suspend/idle inhibit | same via portal path (backend=portal) | Response signal logged; gnome-session inhibitor delta in `systemd-inhibit --list`; release removes it |
| Denial/failure path | deny portal (env or stop service in test scope) → logind fallback or honest "denied" | diagnostics JSON + logs |
| Backend restart resilience | restart `xdg-desktop-portal` (user service, test-scoped) | coordinator re-acquires; state transitions logged |
| Timed expiry | 10 s timer | request gone after expiry; other requests survive |
| Suppression | `awake on` ×2, `awake off` | both released; new automatic requests stay suppressed until explicit resume |
| Quit cleanup | GUI quit while active | all inhibitors gone (list), recording closed |
| Single instance | second launch | exits 0, window activated |
| Autostart toggle | settings toggle | file created/removed under ~/.config/autostart (Kaeru's own only) |
| Tray presence | GUI launch | SNI registered on bus; screenshot evidence; fallback when unavailable (simulated) |
| CLI/IPC | all CLI verbs against running instance | meaningful outputs/exit codes |
| Real suspend/resume | **manual procedure only** (user-run) | documented; not auto-triggered to protect the live session |

## 3. GUI tests (real workflows)

Launch real app on the live X11 session; exercise: start/stop keep awake with durations; focus pomodoro run with skip/extend; work log start/pause/stop/edit; reports totals + CSV export; settings toggles; theme switch; keyboard navigation. Evidence: screenshots via `ffmpeg -f x11grab` + state dumps (`kaeru status --json`), DB inspection of created records.

## 4. Packaging tests (WP-03/07)

- `dpkg-buildpackage -us -uc` clean build; `lintian` review (no errors; warnings documented).
- Install → launch from desktop entry → binary present, icon resolves, service runs.
- Upgrade: install v0.1.0 → v0.1.1 with pre-existing data → data intact.
- Remove: `dpkg -r kaeru` → binaries gone, autostart untouched if user-enabled (documented), user data preserved.
- Reinstall → data still intact; app resumes.
- No network access, no home traversal, no system changes in maintainer scripts.

## 5. Resource measurement (WP-07)

- Idle CPU: `/proc/<pid>/stat` utime+stime delta over 5 min idle (target < 0.5% of one core).
- Memory: RSS via `ps -o rss` (target < 100 MB background).
- Poll frequency: audited (1 s max tick for visible timers; none when idle-hidden).

## 6. Known test limitations (declared up front)

1. Real suspend/resume not auto-triggered (user session protection) — manual procedure provided.
2. Ubuntu 24.04 native validation not performed (no such machine).
3. Wayland/KDE not evaluated.
4. Tray menu cannot be fully script-driven; evidence = SNI registration + screenshots + programmatic action verification.
5. DST machine-clock change not performed; validated via TZ injection instead.
