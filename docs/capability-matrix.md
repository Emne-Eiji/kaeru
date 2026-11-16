# Kaeru — Capability and Compatibility Matrix (0.2.0)

**Honesty rule:** every row is labeled `implemented+tested`,
`available (untested)`, `partial`, `unavailable`, `deferred`, or
`not evaluated`. No target is claimed supported without real evidence.
Passing unit tests do not imply user acceptance.

## Platform targets

| Target | Status | Evidence / notes |
|---|---|---|
| Pop!_OS 22.04 LTS, GNOME 42, X11, amd64 | **Primary validation environment** | live machine; 0.2.0 GUI/portal/package checks run here |
| Ubuntu 24.04 LTS `amd64` | Package target; Qt 6.2-compatible code; **full integration validation not performed** | no machine available; honest gap |
| Wayland sessions | not evaluated | dev session is X11 |
| KDE Plasma / Debian 13 / arm64 | not evaluated | candidate targets only |

## Capability matrix (0.2.0)

| Capability | Status | Backend | Notes |
|---|---|---|---|
| Prevent automatic system sleep | implemented + integration-tested | Portal flag 4 → logind `sleep:block` fallback | 15m/30m/1h/2h/Indefinite verified via GUI and status; portal confirmation race fixed |
| Keep display awake (session idle inhibition) | implemented + integration-tested | Portal flag 8 (idle) | requested/effective `session-idle` observed; display-awake requires the portal |
| Single replaceable Keep Awake request | implemented + GUI-tested | app/core | choosing a new duration replaces the current request; no stacking |
| Timed expiry | implemented + tested | coordinator wall deadline | 5 s probe auto-cleared; long-duration expiry not separately tested |
| Turn off / suppression | implemented + GUI-tested | coordinator | inactive with documented limitation; a new duration resumes |
| Screen locking detection | implemented (preserved) | GNOME ScreenSaver + logind LockedHint | used by the deferred Work Log; gated off in 0.2.0 |
| Suspend/resume detection | implemented signal; **real suspend not tested** | logind `PrepareForSleep` | timers reevaluate on resume; no forced release |
| Tray icon (SNI) | implemented + property-tested | QSystemTrayIcon → StatusNotifierItem | official frog expressions; PNG pixmaps for all sizes; tooltip verified; panel visuals not re-inspected |
| No-tray fallback | implemented | control window | 5×1 s retry/backoff, then visible fallback controls |
| Notifications | implemented + policy-tested; **audible/visual not observed in 0.2.0** | org.freedesktop.Notifications | Keep Awake expiry notification wired and policy-gated; playback is daemon-side |
| Autostart (opt-in) | implemented + sandbox-tested | XDG user autostart file | `Exec=kaeru --background --awake-on-login`; disabling removes only Kaeru’s entry |
| Single instance + IPC | implemented + live-tested (0.1.1); unchanged | D-Bus session service | second launch forwards and exits; CLI uses the same binary |
| Debian package | implemented + built + lintian-clean | debhelper + CMake install | `kaeru_0.2.0-1_amd64.deb`; optional dbgsym isolated; **root lifecycle tests pending** |
| Work Log data preservation | implemented + sandbox-verified | none (database not opened) | deferred module is strictly read-only; pre-seeded DB hash unchanged |
| Idle detection (input-activity) | **unavailable** | — | not part of 0.2.0; Keep Awake does not require it |
| Battery guard | not evaluated (deferred) | — | out of scope |
| Schedules / command-scoped keep awake | not evaluated (deferred) | — | out of scope |

## Module matrix (0.2.0)

| Module | User-facing? | Notes |
|---|---|---|
| Keep Awake (timed/indefinite) | **yes** | sole release feature; single replaceable request, verified lifecycles |
| Focus (countdown + Pomodoro) | **deferred** | source, page, controller and tests preserved; module forced off; CLI unavailable; owner rejection stands |
| Work Log (manual-first, projects, segments) | **deferred** | source, storage and tests preserved; module forced off; database not opened or written |
| Reports + CSV export | **deferred** | source and tests preserved; UI/CLI unavailable |
| Diagnostics | **yes** | live backend/capability state; reports Work Log storage as deferred |
| Settings | **yes** | startup/autostart, notifications, appearance, data, quit |
| Tray | **yes** | Keep Awake actions/status, Open, Settings, Quit |

## Package scope

- Main package: `kaeru_0.2.0-1_amd64.deb` (328,640 B), SHA-256
  `730bb344451a34d1530aeb7d5669cfa249116813a52269830f369cce61b3c720`.
- Runtime deps: Qt 6 Core/DBus/Gui/SQL/Widgets, `libqt6sql6-sqlite`;
  Recommends `xdg-desktop-portal`; no `libqt6svg6`.
- No maintainer scripts, no conffiles; install/upgrade/remove do not touch
  user configuration or data.
