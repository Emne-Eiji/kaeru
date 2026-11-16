# Kaeru — Project and Development History

> Part of the Kaeru handoff package. Read `README.md` in this directory first.
> This document reconstructs the actual history from the repository, Git log
> and evidence files; it does not repeat the full specification (see
   `../PROJECT_SPEC.md` and the documentation package for that).

## 1. What Kaeru is

Kaeru (蛙, "frog") is a native Linux desktop utility for **keep-awake
inhibition, focus timing and honest work logging**. The product thesis, in the
owner's words: *"keeping the computer awake is never mistaken for working."*

### 1.1 The problem

Laptops suspend or blank their displays while people watch downloads finish,
run long builds, cook, or step away briefly. Existing workarounds are global
power-settings changes (dangerous, persistent), shell hacks (`caffeinate`-style
wrappers), or full-screen presentation modes that alter the desktop. Kaeru's
premise: a **temporary, revocable, session-scoped inhibition** through the
desktop's own mechanisms (xdg-desktop-portal Inhibit, logind fallback), with a
visible, honest status — plus a focus timer and a work log that record what
actually happened, with suspend time excluded from work.

### 1.2 Intended users and use cases

- Knowledge workers who run long tasks and don't want the machine to sleep.
- Developers waiting on builds/downloads, students, anyone doing focus
  intervals (countdown or Pomodoro) who wants the display kept awake during
  work but not during breaks (configurable, both off by default).
- People who want a lightweight local work log (projects, sessions, daily
  totals, CSV export) without a cloud service.

### 1.3 Product philosophy (from the specification)

- **Native desktop application**, not a web app: a browser tab cannot take
  logind inhibitors, cannot survive browser throttling, and cannot honestly
  report system capabilities; a native Qt app integrates with the portal,
  logind, the notification daemon, the theme and the tray.
- **Local-first / privacy-first**: no accounts, no network code at all, no
  telemetry; private fields (task titles, tags, notes) are never logged or
  exported; data lives under `$XDG_DATA_HOME/kaeru`.
- **Honesty over optimism** (spec §14): a submitted inhibition request is
  never reported as active; activation is only claimed after backend
  confirmation; unsupported capabilities are reported as unsupported; tests
  are recorded with exact commands and real outputs; mocked results are never
  labeled native evidence.
- **Non-interference**: Kaeru never changes global power settings, never
  touches other apps' autostart entries, never synthesizes input activity,
  and never modifies system Do Not Disturb (quiet hours are enforced by *not
  sending* notifications).
- **Keep Awake ≠ Work Log**: system-sleep inhibition is *not* recorded as
  work; work accrual is suspend-exclusive (CLOCK_MONOTONIC semantics), while
  countdowns are suspend-inclusive (CLOCK_BOOTTIME) — see ADR-003.
- **Platform targets**: primary target Ubuntu 24.04 LTS and newer compatible
  releases; actual development/validation on Pop!_OS 22.04 (Ubuntu 22.04
  base). Qt 6.2 API floor. The capability matrix never claims untested
  compatibility.

### 1.4 Module relationship

Keep Awake, Focus and Work Log are **independent modules** with independent
switches (Settings). Focus can optionally request keep-awake during work or
break intervals; focus intervals can optionally be recorded into
`focus_sessions` (never double-counted as work-log segments); the Work Log is
manual-first. Reports aggregate the work log with date/project filters and CSV
export.

## 2. Development history — work orders WP-00 … WP-07

The project was executed as eight work orders under one autonomous
authorization (2026-11-15). Git history: `git log --oneline` in `kaeru/`.

### WP-00 — Read-only environment investigation (commit: pre-git, report only)

- **Objective:** establish machine facts without modifying anything.
- **Result:** `docs/wp00-environment-report.md` — Pop!_OS 22.04, GNOME 42,
  X11 `DISPLAY=:1`, Qt 6.2.4, CMake 3.22.1, systemd 249, portal Inhibit v3
  active, appindicator tray extension enabled, idle-delay 300 s,
  suspend-after-1800 s idle on AC, TZ America/Toronto.
- **Status:** complete; no modifications were made.

### WP-01 — Design package (commit `bd3b4c6`)

- **Objective:** architecture, ADRs, UI/UX spec, capability matrix, privacy,
  testing strategy, traceability, execution plan, project spec, README,
  CHANGELOG, AGENTS.md, GPL-3.0 LICENSE.
- **Key decisions:** app ID `io.github.kaeru.KaerU` (ADR-001); D-Bus service
  `io.github.kaeru.KaerU` + interface `….Control1` at
  `/io/github/kaeru/KaerU`; portal-first inhibition with logind fallback
  (ADR-002, logind cannot honestly claim SessionIdle); clock model
  BOOTTIME/MONOTONIC/wall (ADR-003); SQLite + WAL + migrations (ADR-004);
  single binary GUI/background/CLI via D-Bus with exit codes 0/2/3/4/5
  (ADR-005); QSystemTrayIcon + no-tray fallback (ADR-006); Qt Widgets + QSS
  themes (ADR-007).
- **Status:** complete (design documents only).

### WP-02 — Core, storage, UI shell, first native evidence (commits `53ad601`, `21cc621`)

- **Objective:** keep-awake core (registry, coordinator, portal + logind
  backends), clocks, timer engine, focus controller, app state, storage
  layer, six-page UI, tray, packaging metadata, and *real* native evidence.
- **Implemented:** `kaeru_core` (Types, Clock, Duration, InhibitionRegistry,
  InhibitionCoordinator, FakeInhibitionBackend, TimerEngine, FocusController,
  AppState, CapabilityRegistry); `kaeru_platform` (PortalInhibitBackend,
  LogindInhibitBackend, LogindPowerMonitor, LockMonitor, Notifications,
  AutostartBackend, SingleInstance, ThemeWatcher); `kaeru_app` (Application,
  CommandRouter, ControlService, CliParser, Paths, Settings, RecoveryManager);
  `kaeru_storage` (Database, Migrations, ProjectRepository, SessionRepository,
  CsvExport); `kaeru_ui` (MainWindow, Theme, Icons, TrayController,
  StatusBadge, six pages).
- **Native evidence (docs/evidence-wp02.md):** portal `Inhibit(flags=8)` →
  `Response(0)` → active → `Request.Close` captured on the live D-Bus; logind
  fallback verified with a real `sleep:block` inhibitor in
  `systemd-inhibit --list`; quit releases all inhibitors.
- **Bugs found and fixed during the work:** settings-view load clobbered quiet
  hours (signal storms), FocusController::pause froze 0 instead of the
  remainder, appendSegment rejected the first segment of every session, CLI
  quit reply mismatch, julianday totals rounding, single-instance takeover
  bugs, resource-alias breakage (icons never loaded from resources).
- **Status:** complete for its scope; the UI pages Work Log/Reports were
  placeholders at this point.

### WP-03 — Tray, single instance, autostart, packaging (commits `b898cb9`, `26b5a6d`, `82e2df6`, `299f008`)

- **Objective:** complete the lifecycle features and deliver the Debian
  package; integrate the official frog icon identity.
- **Completed in two waves:**
  - First wave (`b898cb9`): tray SNI item live-verified, single-instance
    forwarding verified, autostart backend sandbox-verified 15/15;
    packaging blocked at the time (debhelper missing, no sudo password).
  - Second wave (after the owner installed the toolchain and supplied the
    official frog set):
    - **Frog identity (`26b5a6d`, ADR-008):** awake/focused/sleep expressions
      replace the WP-02 placeholder icons; tray uses the three expressions
      with deterministic, safety-first precedence; launcher/window/About use
      the awake frog; hicolor SVGs + PNG fallbacks installed; artwork
      preserved byte-identical in `resources/icons/frog/`.
    - **Packaging (`82e2df6`):** `debian/` (control, rules, changelog,
    copyright, source/format 3.0 native) + `kaeru.1` manpage;
    `dpkg-buildpackage -us -uc -b` → `kaeru_0.1.1-1_amd64.deb`; lintian
    clean except the informational `initial-upload-closes-no-bugs`;
    extraction-run verified; icon resolution 8/8.
    - **Evidence/docs (`299f008`):** `docs/evidence-wp03.md` rewritten with
    all results; tray panel-diff evidence committed.
- **Status:** complete except true `dpkg -i/-r` lifecycle tests (root
  required; extraction-verified instead — documented, see Open Issues).

### WP-04 — Focus polish + notifications (commit `a593f05`)

- **Objective:** focus state machine tests, interval notifications with quiet
  hours, optional keep-awake during work/breaks, settings seeding.
- **Implemented:** `NotifyPolicy` (pure core decisions), interval-completion
  and keep-awake-expiry notifications via org.freedesktop.Notifications with
  optional XDG `sound-name` hint, quiet-hours window in Settings, focus
  widgets seeded from persisted settings.
- **Native evidence (docs/evidence-wp04.md):** exactly 1 notification on
  focus completion (dbus-monitor capture); 0 notifications during all-day
  quiet hours with config preserved.
- **Status:** complete for its scope; **the owner has since rejected the
  Focus module's UX outright — see Open Issues C.**

### WP-05 — Work Log end-to-end (commit `400291a`)

- **Objective:** sessions/segments/projects, SQLite persistence, recovery
  checkpoints, lock/suspend pause, work log page + CLI.
- **Implemented:** WorkSessionManager (start/pause/resume/stop, transactional
  project switch, single active session, suspend/lock boundary close, 60 s
  checkpoints), Application storage wiring + migrations at startup, real
  WorkLogPage, `kaeru work start|pause|resume|stop` over D-Bus, conservative
  crash recovery (recover only to last checkpoint; never invent work;
  checkpoints deleted after recovery so they never duplicate report entries),
  focus intervals → `focus_sessions` (never work-log segments).
- **Native evidence (docs/evidence-wp05.md):** CLI lifecycle with DB
  inspection (2 non-overlapping segments, session closed); `kill -9` recovery
  with honest note.
- **Status:** complete for its scope; **the owner has since reported that it
  records work prematurely — see Open Issues D.**

### WP-06 — Reports + CSV (commit `7f181e2`)

- **Objective:** reports page (filters, daily/project totals), real CSV
  export with formula-injection protection.
- **Implemented:** ReportsPage (date range + project filters, daily totals,
  project breakdown, focus achieved), SessionRepository::totalsByProject /
  focusTotals, CommandRouter::exportCsv, CSV with apostrophe-prefix guard.
- **Native evidence (docs/evidence-wp06.md):** real `kaeru export --format
  csv` with row-count summary; injection guard verified.
- **Status:** complete for its scope; not yet accepted by the owner.

### WP-07 — Hardening + release candidate (commit `5ebecee`)

- **Objective:** full regression, resource measurement, hardening audit,
  release artifacts, delivery report.
- **Result:** clean Release build zero warnings; 12/12 suites at the time;
  audit (no network/telemetry/process-spawn; 0600 private files); RSS
  55.6 MB, idle CPU 0.26 %/300 s; tarball + checksums; delivery report.
- **Status:** complete at the time; superseded by the WP-03 second wave and
  the current open issues.

## 3. How the implementation evolved

- The **core/platform/app/storage/UI layering** has been stable since WP-02:
  core has no GUI/DBus/SQL; platform adapts to the desktop; app wires
  everything through CommandRouter; UI only calls app-level APIs.
- The **resource-path bug** (icons never actually loaded from Qt resources)
  survived from WP-02 to WP-03 unnoticed — a reminder that compiled-and-tested
  does not mean exercised end-to-end; it was caught by live tray testing.
- The **tray icon set** changed twice: placeholder geometric icons (WP-02,
  never rendered) → official frog set (WP-03).
- The **packaging** went from blocked (no toolchain) to a lintian-clean .deb
  once the owner installed debhelper/lintian/libqt6svg6.
- Test count grew 9 → 12 → 13 suites; every suite runs sandboxed with
  `KAERU_TEST_HOME` and `QT_QPA_PLATFORM=offscreen`.

## 4. Where the project stands today

Functionally complete for the approved 1.0 scope **as designed**, but the
owner's acceptance testing has rejected several behaviors (see
`KAERU_OPEN_ISSUES.md`). The handoff package exists precisely because
"all work orders complete" is not the same as "user accepted".