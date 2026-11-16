# Kaeru — Architecture

**Status:** Approved design (WP-01), implementing per user authorization of 2026-11-15.

> **0.2.0 scope:** the shipped release implements the Keep Awake subset of
> this design (plus Settings/Diagnostics). Focus, Work Log and Reports are
> deferred: their design, source and tests remain, but they are not exposed
> in the 0.2.0 GUI/CLI and the Work Log database is not opened. The sections
> below describe the full target architecture, not the 0.2.0 user surface.

## 1. Overview

Kaeru is a single-process Qt 6 Widgets desktop application with a second control surface:

```
                    ┌──────────────────────────── kaeru process ────────────────────────────┐
                    │                                                                      │
 argv ──► CliParser │   Application                                                        │
        (CLI mode)  │     ├── AppState (module switches, pause-all, suppression)            │
                    │     ├── CapabilityRegistry                                           │
                    │     ├── InhibitionRegistry / InhibitionCoordinator                   │
                    │     │        └── InhibitionBackend (interface)                       │
                    │     │             ├── PortalInhibitBackend (org.freedesktop.portal)  │
                    │     │             └── LogindInhibitBackend (org.freedesktop.login1)  │
                    │     ├── TimerEngine (countdown/stopwatch/deadline, injected clocks)  │
                    │     ├── FocusController (state machine)                              │
                    │     ├── WorkSessionManager ── Storage (SQLite: Database/Migrations/  │
                    │     │        │                          Repositories)                │
                    │     │        └── RecoveryManager (checkpoints, crash recovery)       │
                    │     ├── CommandRouter (shared by UI, tray, CLI, DBus service)        │
                    │     ├── TrayController (SNI via QSystemTrayIcon + fallback window)   │
                    │     ├── Notifications (org.freedesktop.Notifications)                │
                    │     ├── AutostartBackend (XDG user autostart file, opt-in)           │
                    │     ├── LockMonitor + PowerMonitor (logind/gnome signals)            │
                    │     ├── Settings (QSettings, $XDG_CONFIG_HOME/kaeru/)                │
                    │     └── UI: MainWindow { Overview, Focus, Work Log, Reports,         │
                    │                             Settings, Diagnostics }                 │
                    │   ControlService (DBus io.github.kaeru.Kaeru.Control1; single        │
                    │   instance + IPC for CLI)                                            │
                    └──────────────────────────────────────────────────────────────────────┘
```

## 2. Layer rules

- **core/** — pure state machines and value types. Depends on Qt Core (types, signals) and injected `IClock`/backend interfaces only. No widgets, no DBus, no SQL.
- **platform/** — QtDBus/system adapters implementing core interfaces: inhibition backends, power/lock monitors, notifications, autostart, single-instance, theme watcher.
- **storage/** — SQLite via `QSqlDatabase`; migrations; repositories; CSV writer. Parameterized SQL only.
- **app/** — wiring: `Application` owns core services and platform adapters; `CommandRouter` implements all user commands once (UI, tray, CLI, DBus all call it); `main.cpp` selects GUI vs CLI mode by argv.
- **ui/** — widgets only; reads state via signals; never calls platform backends directly.

## 3. Key data flows

### 3.1 Keep Awake request lifecycle

```
UI/CLI/Timer → InhibitionRegistry.add(InhibitionRequest)
             → InhibitionCoordinator.resolve()
                 1. module enabled? pause-all? suppressed?
                 2. safety constraints (never shutdown/logout/lid)
                 3. drop expired
                 4. suppression check
                 5. union of capabilities from eligible requests
                 6. diff vs acquired bindings → acquire/release per capability
                 7. publish effective state (requested ≠ effective)
```

Each binding tracks: capability, backend, handle (portal request path / logind fd), state (Activating → Active/Failed/Denied), acquire timestamps. Backend loss (service restart, denial, failure) demotes the binding and triggers re-resolution.

### 3.2 Clocks

| Kind | Source | Suspend | Used for |
|---|---|---|---|
| Monotonic | `CLOCK_MONOTONIC` | stops | work/focus stopwatch accrual (suspend-exclusive) |
| Boot | `CLOCK_BOOTTIME` (Linux) | runs | keep-awake countdowns and Pomodoro interval progression (suspend-inclusive) |
| Wall | `QDateTime` UTC | — | timestamps, wall-clock deadlines (TZ-aware) |

`TimerEngine` guarantees "fires once" completion after resume and re-derivation of wall deadlines on clock change.

### 3.3 Work Log persistence

Sessions (project, task, tags, note, source, revision history) → segments (recorded intervals, kind work/uncertain). One active session (partial unique index). Suspend/lock closes the open segment and pauses the session. Checkpoints (`recovery_checkpoints`) every 60 s while recording; after a crash the recovered duration is capped at the last checkpoint, the remainder is a marked-uncertain gap for user review. No invented work.

### 3.4 Command routing

`CommandRouter` exposes: status, awake on/off, focus start/pause/resume/skip/stop, work start/pause/stop, pause-all, export, diagnostics, show-window, quit. UI actions, tray menu actions, CLI and the DBus control service all funnel through it → one implementation, consistent behavior (KAE-LIFE-001, spec §12).

## 4. Processes, instances, IPC

- One instance per graphical session. Enforced by owning the D-Bus name `io.github.kaeru.Kaeru` (service). A second launch: CLI args → remote call via the service; plain launch → `ShowWindow()` on the running instance, exit 0.
- IPC interface: `io.github.kaeru.Kaeru.Control1` at `/io/github/kaeru/Kaeru`. Session-bus scoped; user-session ownership restricts access to the user's own session.
- CLI exit codes: 0 ok · 2 usage error · 3 not running · 4 operation failed · 5 denied/unsupported.

## 5. Storage layout

```
$XDG_CONFIG_HOME/kaeru/kaeru.conf        Settings (QSettings ini)
$XDG_DATA_HOME/kaeru/kaeru.db            SQLite (WAL), + -wal/-shm, backups/
$XDG_CACHE_HOME/kaeru/                   cache (unused in 1.0)
$XDG_RUNTIME_DIR/kaeru/                  runtime scratch if needed
~/.config/autostart/io.github.kaeru.Kaeru.desktop   only when user enables autostart
```

## 6. Schema (v1)

`schema_migrations`, `projects`, `sessions`, `session_segments`, `session_revisions`, `focus_sessions`, `recovery_checkpoints`. Details in `storage/Migrations.cpp` (single source of truth) — constraints: one active session (partial unique index), non-negative durations (CHECK), FK integrity (`ON`), transactional project switch, WAL journal.

Deferred (not created until approved): `schedules`, `presets`.
