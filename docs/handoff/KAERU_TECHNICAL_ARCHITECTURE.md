# Kaeru — Technical Architecture and Source Walkthrough

> Handoff package document. All paths are relative to `kaeru/` unless noted.
> This document describes the code **as it is**, not as the docs claim. Where
> they disagree, the disagreement is flagged (see also
   `KAERU_OPEN_ISSUES.md`).
>
> **0.2.0 scope note:** Keep Awake is the only user-facing feature. Focus and
> Work Log code remains in the tree and compiles, but their modules are
> forced off; UI/CLI paths are absent or gated, and the Work Log database is
> not opened or written. Sections 6–7 describe the preserved code for a
> future release, not 0.2.0 behavior.

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

- Parses argv with `CliParser` (0.2.0 grammar in
  `src/app/CliParser.cpp`: `status`, `awake on|off|indefinite`,
  `diagnostics`, `open`, `quit`, plus the startup flags `--background` and
  `--awake-on-login`; deferred `focus …`, `work …`, `export`, `pause-all`
  are recognized but report not-available; durations like
  `90s/30m/1h30m`; exit codes 0/2/3/4/5).
- Modes: GUI (default), `--background` (tray only, window hidden),
  `GuiBackground` vs `Gui`, `Help`, `Version`, `Command`.
- Honors `KAERU_TEST_HOME` **before** any Qt path resolution
  (`Paths::applyTestSandbox`) — the isolation mechanism for all tests.
- GUI path: `QApplication` → `Application` (the wiring root) →
  `SingleInstance::tryAcquire()` (D-Bus name `io.github.kaeru.Kaeru`;
  on failure: forward ShowWindow and exit 0) → `MainWindow` →
  `ControlService` adaptor registered on `/io/github/kaeru/Kaeru` →
  `app.exec()`.
- CLI-command path: creates a **short-lived** `Application` + `ControlService`
  that call the **same** `CommandRouter` handlers as the GUI/DBus paths
  (spec: one command surface; verified by tests and live use).

## 3. Wiring root (`src/app/Application.{h,cpp}`)

`Application` owns and connects everything:

- `Settings` (INI at `$XDG_CONFIG_HOME/kaeru/kaeru.conf`, keys like
  `notifications/quietHoursEnabled`), `AppState` (module switches, Pause All),
  `CapabilityRegistry` (diagnostics), `TimerEngine`, `FocusController`,
  `InhibitionCoordinator`, `ManualAwakeSession`, backends (portal, logind,
  power, lock, notifications, autostart, theme), storage (`Database`,
  `Migrations`, repositories, `WorkSessionManager`).
- 0.2.0 forces `KeepAwake=true`, `Focus=false`, `WorkLog=false`. Storage
  setup is skipped while Work Log is deferred, so the database is not opened
  or migrated.
- 1 Hz `QTimer` → `onTick()`: `timers.tick()`, `inhibition.tick(now)`,
  `work->tick()` only when Work Log is enabled (60 s checkpoints).
- Focus wiring: `intervalStarted` → `applyFocusKeepAwake(...)` per settings;
  `intervalCompleted` → optional `work->recordFocusWork(...)` (focus_sessions
  only; gated by both modules) + `focusIntervalFinished` → notification
  (policy-gated).
- Boundaries: `prepareForSleep` → timers + work suspend boundary (gated);
  `lockStateChanged` → `work->onLockChanged` (gated).
- `prepareQuit()`: `work->stop()` only when Work Log is enabled (graceful
  save) + `focus->stop()` + `inhibition.releaseAll()` + settings sync.
- `statusJson()` / `diagnosticsJson()`: the CLI/DBus payloads (safe fields
  only; no private content).

## 4. Keep Awake (spec §6.1/§6.2, ADR-002)

- **Single manual session (0.2.0):** `ManualAwakeSession`
  (`src/core/ManualAwakeSession.{h,cpp}`) owns exactly one request and
  builds timed/indefinite/until requests. `Application::beginManualAwake`
  adds the replacement request before removing the previous one, so the
  newest duration wins without a release/re-acquire gap. `awakeOff()` stops
  the session and suppresses automatic reactivation until a new explicit
  request.
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
  signal with code 0; release = `Request.Close()`. **0.2.0:** the backend
  predicts the request handle from the bus name + `handle_token` and
  subscribes before calling `Inhibit`, closing the activation race.
  `release()` also closes requests that are still activating.
- **Logind backend** (`src/platform/LogindInhibitBackend.{h,cpp}`):
  `Manager.Inhibit("sleep", "Kaeru", why, "block")` → `QDBusUnixFileDescriptor`;
  the fd is marked `FD_CLOEXEC` and closed on release; honest limitation: it
  cannot provide SessionIdle (never claimed).
- **Capability reporting:** `Application::setupBackends()` publishes honest
  statuses into the CapabilityRegistry (portal/logind availability, idle
  unsupported, suspend/lock detection, notifications, storage-deferred).
- **Expiry:** coordinator registry expiry via `tick(now)` →
  `expireElapsed`; the signal carries the original `InhibitionRequest`
  (reason + policy) so the UI/notification can report what ended.
- **CLI surface (0.2.0):** `kaeru awake on --duration 30m --mode
  system|display|both`, `awake indefinite`, `awake off`.

## 5. Clocks (`src/core/Clock.{h,cpp}`, ADR-003)

`MonotonicTimeSource` (QElapsedTimer; suspend-exclusive), `BootTimeSource`
(CLOCK_BOOTTIME with honest fallback), `WallTimeSource` (UTC + tz),
`FakeTimeSource` (tests). All engines take a `ClockSet`; tests inject fakes.

## 6. Focus (spec §6.3) — **deferred in 0.2.0**

`src/core/FocusController.{h,cpp}` + `src/ui/pages/FocusPage.{h,cpp}` are
preserved and compiled, but the module is forced off and the page/CLI are
not reachable in 0.2.0.

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

## 7. Work Log (spec §6.4, §10) — **deferred and read-only in 0.2.0**

`src/app/WorkSessionManager.{h,cpp}` and `src/storage/SessionRepository.{h,cpp}`
are preserved and compiled, but the module is forced off. Storage setup is
skipped, so the existing database is not opened, migrated, checkpointed or
closed; tick, suspend/lock, focus recording, recovery and quit-stop are
gated. The repository/model description below applies to the preserved code,
not to 0.2.0 runtime behavior.

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

`MainWindow` (sidebar nav with **three pages: Keep Awake, Settings,
Diagnostics**; Ctrl+1..3; File > Settings/Quit; close hides when tray
available), `TrayController` (SNI icon + tooltip + one persistently built menu
refreshed from `InhibitionCoordinator::stateChanged`/`AppState::changed`;
exact `Kaeru: OFF/ON/ACTIVATING…/ERROR` status line, `Kaeru` duration submenu,
`Turn off Kaeru` gated on confirmed activation, Open, Settings, Quit; fallback
window after 5×1 s retries), `Icons`
(loads `:/icons/frog/png/*.png`), `Theme` (light/dark QSS), `StatusBadge`
(icon+word+shape, never color-only). Pages: Overview (stable-layout Keep
Awake card, duration chips 15m/30m/1h/2h/Indefinitely, one turn-off action),
Settings (startup/autostart, notifications, appearance, data, quit),
Diagnostics (capability table + live bindings). Focus/WorkLog/Reports pages
remain in the source tree but are not constructed.

## 11. Frog identity (`docs/icon-assets.md`, ADR-008)

Official artwork supplied by Emne, preserved byte-identical in
`resources/icons/frog/` (SVG + PNG 16–512 + preview + generator). **0.2.0
runtime resources are PNG-only:** `:/icons/frog/png/{awake,focused,sleep}-{16…256}.png`
(via `qt_add_resources(PREFIX "/" BASE resources)`). The SVGs remain
installed in hicolor `scalable/apps` for desktop integration. Launcher icon =
awake frog; tray mapping with safety-first precedence (awake > focused >
sleep; tooltip states all facts). **Fixed defect:** Qt 6.2's QSvgRenderer
renders `focused.svg` with a black face; the supplied PNGs render correctly,
so no SVG rendering is used at runtime. `test_icons` guards the palette.

## 12. Build and packaging

- CMake ≥ 3.16, Qt 6.2 (Widgets/Gui/DBus/Sql/Test), C++17,
  `-Wall -Wextra -Wpedantic`. Targets: `kaeru_core`, `kaeru_storage`,
  `kaeru_platform`, `kaeru_app`, `kaeru_ui` (+resources), `kaeru`,
  `test_*` (**15 suites**; `enable_testing()`; tests run with
  `QT_QPA_PLATFORM=offscreen` and sandboxed via `KAERU_TEST_HOME`).
- Install rules: binary → `bin/`, desktop entry, metainfo, manpage,
  hicolor SVG + PNG icons, README/LICENSE.
- Debian: `debian/` via debhelper 13 + cmake buildsystem,
  `hardening=+all`, binary-only target; produces
  `kaeru_0.2.0-1_amd64.deb` (+ auto `kaeru-dbgsym` .ddeb, optional and
  isolated in the release folder). No maintainer scripts, no conffiles.

## 13. Known architectural debt / limitations (code-verified)

- `TimerEngine` default-constructs real `TimeSource` objects that are
  leaked when tests inject fakes (minor; one instance in production).
- `RecoveryManager` is a stub (`emptyReport()`); recovery logic actually
  lives in `WorkSessionManager::recoverInterrupted` (deferred in 0.2.0).
- `session_revisions` table exists but nothing writes revisions yet.
- Focus/WorkLog/Reports pages remain compiled but are not constructed;
  their CommandRouter paths return failures while the modules are disabled.
- `ControlService` still exposes legacy Focus/Work methods; they are gated
  by the module switches and are not part of the normal 0.2.0 CLI.
- The tray “Settings” action now switches to the Settings page.
- `QSettings` INI keys are not documented anywhere user-facing.
- Tray menu content/actions are verified through DBusMenu and an XEmbed-hosted
  Qt menu; a specific panel's theme, icon scaling and placement remain
  desktop-side and are not asserted (`docs/testing.md` §2a).