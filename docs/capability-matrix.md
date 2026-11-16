# Kaeru — Capability and Compatibility Matrix

**Honesty rule:** every row is labeled `implemented+tested`, `available (untested)`, `partial`, `unavailable`, or `not evaluated`. No target is claimed supported without real evidence.

## Platform targets

| Target | Status | Evidence / notes |
|---|---|---|
| Pop!_OS 22.04 LTS, GNOME 42, X11, amd64 | **Primary validation environment** | live machine (WP-00); all integration tests run here |
| Ubuntu 24.04 LTS `amd64` | Package target; code uses Qt 6.2-compatible APIs; **full integration validation not performed** (no machine available) | honest gap; see docs/testing.md §limitations |
| Wayland sessions | not evaluated | dev session is X11 |
| KDE Plasma / Debian 13 / arm64 | not evaluated | candidate targets only (spec §5.2) |

## Capability matrix (primary environment)

| Capability | Status | Backend | Notes |
|---|---|---|---|
| Prevent automatic system sleep | implemented + integration-tested in WP-02 | Portal flag 4 → logind `sleep:block` fallback | evidence: `systemd-inhibit --list`, `CanSuspend` |
| Keep display awake (session idle inhibition) | implemented + integration-tested in WP-02 | Portal flag 8 (idle) | GNOME portal idle inhibition affects blank/idle actions |
| Screen locking detection | implemented | org.gnome.ScreenSaver + logind LockedHint | used to pause Work Log (opt-out) |
| Suspend/resume detection | implemented (signal) | logind `PrepareForSleep` | real-suspend full test = manual procedure (not auto-triggered) |
| Tray icon (SNI) | implemented + live-tested WP-03 | QSystemTrayIcon → StatusNotifierItem | official frog expressions (awake/focused/sleep) with safety-first precedence (ADR-008); tooltip + pixmap + panel-diff verified in all states; dev panel desaturates via user's extension (rendering itself verified) |
| No-tray fallback | implemented | control window | always available; tray-unavailable retry/backoff (5×1 s) |
| Notifications | implemented + native-tested WP-04 | org.freedesktop.Notifications | focus-interval + keep-awake-expiry notifications; quiet-hours suppression verified natively (0 notifications all-day-quiet; config preserved); sound via XDG `sound-name` hint (audible check manual); policy unit-tested (test_notify) |
| Autostart (opt-in) | implemented + sandbox-tested WP-03 | XDG user autostart file | off by default; only Kaeru's own entry (15/15 harness checks); GUI toggle wired to same backend |
| Single instance + IPC | implemented + live-tested WP-03 | D-Bus session service | second launch forwards ShowWindow + exits 0 (two takeover bugs found & fixed, CHANGELOG); CLI via same binary |
| Debian package | implemented + built + lintian-clean WP-03 | debhelper + CMake install rules | kaeru_0.1.1-1_amd64.deb; no maintainer scripts, no conffiles (data preservation structural); dpkg lifecycle tests pending root |
| Idle detection (input-activity) | **unavailable (out of 1.0 scope)** | — | manual tracking unaffected; reported honestly in Diagnostics |
| Battery guard | not evaluated (deferred) | — | hardware exists; out of 1.0 |
| Schedules / command-scoped keep awake | not evaluated (deferred) | — | requires separate approval |

## Module matrix

| Module | 1.0 | Notes |
|---|---|---|
| Keep Awake (manual, timed, indefinite w/ warning) | yes | request registry, verified lifecycles |
| Focus (countdown + Pomodoro) | yes | suspend-inclusive countdown, suspend-exclusive work accrual |
| Work Log (manual-first, projects, segments) | yes | implemented + native-tested WP-05; recovery conservative (never invents work) |
| Reports + CSV export | yes | implemented + native-tested WP-06; formula-injection guard verified |
| Diagnostics | yes | live backend state |
