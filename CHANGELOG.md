# Changelog

All notable changes to Kaeru are documented here. Format: Keep a Changelog 1.1; versioning: SemVer.

## [0.1.1] — 2026-11-15

### Added
- WP-01 design package: architecture, ADR-001..007, UI/UX specification, capability matrix, privacy/testing/traceability documents, research register.
- WP-02 implementation: core library (types, clocks, inhibition registry/coordinator, timer engine, focus controller, app state, capability registry), platform adapters (portal/logind inhibition, logind power monitor, lock monitor, notifications, autostart, single instance, theme watcher), app layer (Application, CommandRouter, ControlService over D-Bus, CLI parser, paths, settings), storage layer (SQLite WAL database, versioned migrations, project/session repositories, CSV export with formula-injection protection), UI shell (main window, six pages, tray controller with fallback, themes, original SVG icon set), desktop entry + AppStream metainfo.
- WP-03 (partial): tray verified live (SNI item, tooltip, DBusMenu); single-instance forwarding verified live; autostart backend sandbox-verified; `docs/evidence-wp03.md`. Packaging blocked on `sudo apt install debhelper lintian devscripts` (password unavailable this session).
- WP-04: focus notifications (interval completion + keep-awake expiry) with policy-gated quiet hours (pure `core/NotifyPolicy.h`, unit-tested via test_notify) and optional XDG `sound-name` hint; quiet-hours window (from/to) configurable in Settings; focus widgets seeded from persisted settings; real notification captured via dbus-monitor (docs/evidence-wp04.md).
- Repository bootstrap (kaeru/), build system skeleton.

### Fixed
- SettingsPage: loading the settings view no longer persists not-yet-loaded widget state (checkbox `toggled` handlers clobbered quiet hours to false/00:00 at every launch — caught by the quiet-hours native test). All seeding is signal-blocked now. Same fix applied to FocusPage seeding (spinbox minimums were overwriting user settings on launch).
- Single-instance: `tryAcquire` used `AllowReplacement` (second launch could take over the bus name instead of forwarding) and treated the non-error "name taken" reply as success (`registerService` returns `ServiceNotRegistered` as a *valid* reply). Both fixed; second launch now forwards and exits 0 (verified live; `docs/evidence-wp03.md` §2).
- `InhibitionCoordinator::effectiveState` now reports module/pause-gated requested capabilities (suppression/module-disable/Pause All report Inactive, not Unsupported); Pause All clears interrupted requests so they are never auto-restarted (spec §7).
- `FocusController::pause()` computed remaining time after switching to Paused (froze 0 instead of the real remainder).
- `SessionRepository::appendSegment` treated "no open segment" as an error (first segment of every session was rejected).
- CLI `kaeru quit` expected a QString reply from the void `Quit` method (now uses a raw D-Bus message call).
- Timer totals use ROUND() on julianday deltas (floating-point truncation lost 1 ms per segment).

### Notes
- Specification 0.1.1 approved by user (2026-11-15); WP-00…WP-07 execution authorized the same date.

## [0.1.0] — 2026-11-15 (pre-release milestones)

### Added
- WP-00 environment investigation report (read-only).
