# Kaeru — Technical Architecture and Source Walkthrough

> Handoff package document. All paths are relative to `kaeru/` unless noted.
> This document describes the code **as it is**, not as the docs claim. Where
> they disagree, the disagreement is flagged (see also
   `KAERU_OPEN_ISSUES.md`).

## 1. Layering

```mermaid
flowchart TD
    subgraph core["kaeru_core (Qt Core only)"]
        types[Types: EffectiveAwakeState, InhibitionRequest…]
        clocks[Clock: Monotonic/Boot/Wall + FakeTimeSource]
        registry[InhibitionRegistry]
        coordinator[InhibitionCoordinator]
        fakes[FakeInhibitionBackend (tests)]
        timers[TimerEngine]
        focus[FocusController]
        state[AppState]
        caps[CapabilityRegistry]
        notify[NotifyPolicy (TrayState.h too)]
    end
    subgraph storage["kaeru_storage (Qt Sql)"]
        db[Database (WAL, 0600)]
        migrations[Migrations v1]
        projects[ProjectRepository]
        sessions[SessionRepository]
        csv[CsvExport]
    end
    subgraph platform["kaeru_platform (Qt DBus)"]
        portal[PortalInhibitBackend]
        logind[LogindInhibitBackend]
        power[LogindPowerMonitor]
        lock[LockMonitor]
        notif[Notifications]
        autostart[AutostartBackend]
        single[SingleInstance]
        theme[ThemeWatcher]
    end
    subgraph app["kaeru_app"]
        application[Application (wiring)]
        router[CommandRouter]
        control[ControlService (DBus adaptor)]
        cli[CliParser]
        paths[Paths]
        settings[Settings (QSettings INI)]
        work[WorkSessionManager]
        recovery[RecoveryManager (stub)]
    end
    subgraph ui["kaeru_ui (Qt Widgets)"]
        mainwin[MainWindow]
        tray[TrayController]
        icons[Icons]
        themeui[Theme]
        pages[Overview/Focus/WorkLog/Reports/Settings/Diagnostics pages]
    end
    app --> core
    app --> storage
    app --> platform
    ui --> app
    platform --> core
    storage --> core
```

Rules enforced by the build: `kaeru_core` has no GUI/DBus/SQL; `kaeru_platform`
sits behind interfaces; all user commands flow through `CommandRouter`
(AGENTS.md).

## 2. Entry point and lifecycle (`src/app/main.cpp`)

- Parses argv with `CliParser` (grammar in `src/app/CliParser.cpp`;
  `status`, `awake on|off`, `focus …`, `work …`, `export`, `pause-all`,
  `diagnostics`, `open`, `quit`; durations like `90s/30m/1h30m`; exit codes
  0/2/3/4/5).
- Modes: GUI (default), `--background` (tray only, window hidden),
  `GuiBackground` vs `Gui`, `Help`, `Version`, `Command`.
- Honors `KAERU_TEST_HOME` **before** any Qt path resolution
  (`Paths::applyTestSandbox`) — the isolation mechanism for all tests.
- GUI path: `QApplication` → `Application` (the wiring root) →
  `SingleInstance::tryAcquire()` (D-Bus name `io.github.kaeru.KaerU`;
  on failure: forward ShowWindow and exit 0) → `MainWindow` →
  `ControlService` adaptor registered on `/io/github/kaeru/KaerU` →
  `app.exec()`.
- CLI-command path: creates a **short-lived** `Application` + `ControlService`
  that call the **same** `CommandRouter` handlers as the GUI/DBus paths
  (spec: one command surface; verified by tests and live use).

## 3. Wiring root (`src/app/Application.{h,cpp}`)

`Application` owns and connects everything:

- `Settings` (INI at `$XDG_CONFIG_HOME/kaeru/kaeru.conf`, keys like
  `notifications/quietHoursEnabled`), `AppState` (module switches, Pause All),
  `CapabilityRegistry` (diagnostics), `TimerEngine`, `FocusController`,
  `InhibitionCoordinator`, backends (portal, logind, power, lock,
  notifications, autostart, theme), storage (`Database`, `Migrations`,
  repositories, `WorkSessionManager`).
- 1 Hz `QTimer` → `onTick()`: `timers.tick()`, `inhibition.tick(now)`,
  `work->tick()` (60 s checkpoints).
- Focus wiring: `intervalStarted` → `applyFocusKeepAwake(...)` per settings;
  `intervalCompleted` → optional `work->recordFocusWork(...)` (focus_sessions
  only) + `focusIntervalFinished` → notification (policy-gated).
- Boundaries: `prepareForSleep` → timers + work suspend boundary;
  `lockStateChanged` → `work->onLockChanged`.
- `prepareQuit()`: `work->stop()` (graceful save) + `focus->stop()` +
  `inhibition.releaseAll()` + settings sync.
- `statusJson()` / `diagnosticsJson()`: the CLI/DBus payloads (safe fields
  only; no private content).

## 4. Keep Awake (spec §6.1/§6.2, ADR-002)

- **Request model:** `InhibitionRequest` (source, capabilities, reason,
  expiration policy) in `InhibitionRegistry`; the coordinator resolves
  requests into backend bindings with explicit precedence (1: quit/disable/
  pause, 2: suppression, 3: eligible requests). A binding is NOT active
  until the backend confirms (`requested ≠ effective`; status values
  Inactive/Activating/Active/PartiallyActive/Unsupported/PermissionDenied/
  Failed).
- **Portal backend** (`src/platform/PortalInhibitBackend.{h,cpp}`):
  `org.freedesktop.portal.Inhibit` v3 `Inhibit(hwnd, flags, options)` →
  handle; flags 4 = suspend (system sleep), 8 = idle; activation = `Response`
  signal with code 0; release = `Request.Close()`. Tracks pending handles by
  request path; handles service restarts by re-resolving.
- **Logind backend** (`src/platform/LogindInhibitBackend.{h,cpp}`):
  `Manager.Inhibit("sleep", "Kaeru", why, "block")` → `QDBusUnixFileDescriptor`;
  the fd is marked `FD_CLOEXEC` and closed on release; honest limitation: it
  cannot provide SessionIdle (never claimed).
- **Capability reporting:** `Application::setupBackends()` publishes honest
  statuses into the CapabilityRegistry (portal/logind availability, idle
  unsupported, suspend/lock detection, notifications, storage).
- **Expiry:** all countdowns are `ClockSet::boot` (suspend-inclusive) in
  `TimerEngine`; coordinator expiry via `tick(now)` → `expireElapsed`.
- **CLI surface:** `kaeru awake on --duration 30m --mode system|display|both`,
  `awake off` (suppress until explicit resume), `resume` via router.

## 5. Clocks (`src/core/Clock.{h,cpp}`, ADR-003)

`MonotonicTimeSource` (QElapsedTimer; suspend-exclusive), `BootTimeSource`
(CLOCK_BOOTTIME with honest fallback), `WallTimeSource` (UTC + tz),
`FakeTimeSource` (tests). All engines take a `ClockSet`; tests inject fakes.

## 6. Focus (spec §6.3) — `src/core/FocusController.{h,cpp}` + `src/ui/pages/FocusPage.{h,cpp}`

State machine: Idle → Work → (autoNext) ShortBreak/LongBreak → …; pause
freezes remaining (countdown cancelled and recreated on resume); skip
advances (pomodoro) or stops (custom); extend adds; `setUpcomingWorkMs`
overrides the next work interval. Options: work/short/long/every, autoNext
(off), keepAwakeDuringWork/Breaks (off), recordInWorkLog (off), notify,
sound, quiet hours. **Design caveats (owner rejected the UX — see Open
Issues C):** with autoNext off, each finished interval returns to Idle and
the next interval only starts on an explicit Start press; `startPomodoro`
resets the completed counter; the Focus page is a dense settings-style form
rather than a glanceable timer. Native behavior is not yet meaningfully
tested beyond unit level.

## 7. Work Log (spec §6.4, §10) — `src/app/WorkSessionManager.{h,cpp}`, `src/storage/SessionRepository.{h,cpp}`

- **Model:** `sessions` (at most one open; `started_at`, `ended_at`,
  `state`) and `session_segments` (`kind` ∈ work/break/paused/idle,
  `source`, `started_at`, `ended_at`); `focus_sessions` separate;
  `recovery_checkpoints` resume evidence only; `session_revisions` for
  provenance (table exists; no UI writes yet).
- **Lifecycle:** `start` (creates session + first work segment, resumes if
  one is open), `pause` (closes the open segment; session stays open),
  `resume` (new work segment), `stop` (closes segment + session),
  `switchProject` (transactional close+reopen), `recordFocusWork` →
  `focus_sessions` (never work segments; no double counting).
- **Boundaries:** suspend (`PrepareForSleep`) closes the open segment;
  lock (opt-in `pauseWorkWhenLocked`) closes it; on graceful quit the
  session is stopped and saved. There is **no idle detection** (out of 1.0)
  — a running segment records continuously until stopped/paused/suspended
  (the owner's "records work prematurely" complaint targets this — see
  Open Issues D).
- **Recovery:** `recoverInterrupted()` at startup closes any open session at
  the last checkpoint (or segment start if none), deletes checkpoints
  (never duplicate report entries), and returns honest notes.
- **Reports:** `dailyTotals` (local days via UTC offset), `totalsByProject`,
  `focusTotals`; open segments never count.

## 8. Storage (`src/storage/`)

`Database` (QSQLITE, WAL, FK ON, busy_timeout 5 s, 0600 perms),
`Migrations` (schema_migrations; v1: projects/sessions/session_segments/
session_revisions/focus_sessions/recovery_checkpoints; backup-before-risky-
migrations helper), `ProjectRepository`, `SessionRepository` (all
constraints enforced in SQL + transactions), `CsvExport` (RFC-4180-ish
escaping + OWASP formula-injection guard: leading `= + - @ \t \r` get an
apostrophe).

## 9. Platform adapters (`src/platform/`)

Portal/Logind inhibition (above), `LogindPowerMonitor` (PrepareForSleep),
`LockMonitor` (org.gnome.ScreenSaver ActiveChanged + logind LockedHint),
`Notifications` (org.freedesktop.Notifications; optional `sound-name` hint;
quiet hours enforced by callers), `AutostartBackend` (own XDG entry only),
`SingleInstance` (D-Bus name; DontQueueService, no replacement — name clash
is a valid reply with ServiceNotRegistered, checked by value),
`ThemeWatcher` (portal appearance color-scheme).

## 10. UI (`src/ui/`)

`MainWindow` (sidebar nav, six pages, Ctrl+1..6, close hides when tray
available), `TrayController` (SNI icon + tooltip + DBusMenu; fallback window
after 5×1 s retries; menu includes Keep Awake submenu, Focus, Work Log with
today total, Pause All, Settings, Quit), `Icons` (loads `:/icons/frog/*.svg`),
`Theme` (light/dark QSS), `StatusBadge` (icon+word+shape, never color-only),
pages: Overview (state-first cards, duration chips 15m/30m/1h/2h/Indefinitely),
Focus, WorkLog, Reports, Settings (modules/startup/notifications/appearance/
data/about with frog), Diagnostics (capability table + live bindings).

## 11. Frog identity (`docs/icon-assets.md`, ADR-008)

Official artwork supplied by Emne, preserved byte-identical in
`resources/icons/frog/` (SVG + PNG 16–512 + preview + generator). Qt
resources: `:/icons/frog/{awake,focused,sleep}.svg` (via
`qt_add_resources(PREFIX "/" BASE resources)`). Launcher icon =
awake frog (`io.github.kaeru.KAeru.svg` hicolor); tray mapping with
safety-first precedence (awake > focused > sleep; tooltip states all facts).
**Known defect:** Qt 6.2's QSvgRenderer renders `focused.svg` with a BLACK
face (verified: face pixels (0,0,0,255) vs (76,175,80,255) for awake/sleep;
supplied PNGs render correctly via rsvg-convert) — see Open Issues E.

## 12. Build and packaging

- CMake ≥ 3.16, Qt 6.2 (Widgets/Gui/DBus/Sql/Test), C++17,
  `-Wall -Wextra -Wpedantic`. Targets: `kaeru_core`, `kaeru_storage`,
  `kaeru_platform`, `kaeru_app`, `kaeru_ui` (+resources), `kaeru`,
  `test_*` (13 suites; `enable_testing()`; tests run with
  `QT_QPA_PLATFORM=offscreen` and sandboxed via `KAERU_TEST_HOME`).
- Install rules: binary → `bin/`, desktop entry, metainfo, manpage,
  hicolor SVG + PNG icons, README/LICENSE.
- Debian: `debian/` via debhelper 13 + cmake buildsystem,
  `hardening=+all`, binary-only target; produces
  `kaeru_0.1.1-1_amd64.deb` (+ auto `kaeru-dbgsym` .ddeb, see Open
  Issues A). No maintainer scripts, no conffiles.

## 13. Known architectural debt / limitations (code-verified)

- `TimerEngine` default-constructs real `TimeSource` objects that are
  leaked when tests inject fakes (minor; one instance in production).
- `RecoveryManager` is a stub (`emptyReport()`); recovery logic actually
  lives in `WorkSessionManager::recoverInterrupted`.
- `session_revisions` table exists but nothing writes revisions yet.
- OverviewPage Work Log card still shows stale text ("Manual work tracking
  arrives with the Work Log milestone") although WP-05 delivered it.
- `ControlService::WorkResume` exists; `ExportCsv` is implemented; the
  tray "Settings" action only opens the main window (no page switch).
- `FocusController::intervalCompleted` reports planned/achieved (now
  populated) — earlier reports of zeros are outdated.
- `QSettings` INI keys are not documented anywhere user-facing.