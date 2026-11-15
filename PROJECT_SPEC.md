# Kaeru — Project Specification Digest

**Status:** USER APPROVED — 2026-11-15 (see authoritative package)
**Authoritative specification:** `../Kaeru_Documentation_Package_v0.1.1/MASTER_SPECIFICATION.md` (KAE-SPEC-001 v0.1.1)
**Operating protocol:** `../Kaeru_Documentation_Package_v0.1.1/AGENT_OPERATING_PROTOCOL.md`
**Execution authorization:** user instruction dated 2026-11-15 — WP-00…WP-07 autonomous execution within safety boundaries.

## Product mission (binding)

Lightweight, Linux-native desktop utility that helps its owner prevent unwanted automatic system sleep, optionally keep the display awake, run timed keep-awake sessions, organize focused work and breaks, record and review work hours honestly, start after graphical login only when explicitly enabled, and be controlled from a tray icon where supported — with independently enable/disableable modules.

**Central principle: keeping the computer awake must never be mistaken for the user working.**

## Hard prohibitions (binding)

No mouse jigglers, fake input, presence falsification, hidden monitoring, screenshots, keyloggers, browser history collection, payroll/invoicing, cloud requirement, employer dashboard, telemetry by default, account requirement, root/privileged daemon for normal operation, or machine-wide power-setting changes.

## Version 1.0 required scope

Packaging (maintainable `.deb`, launcher, icons), Keep Awake (manual toggle, timed sessions, verified native inhibition), tray + visible fallback, user-controlled autostart, Focus timer (countdown + Pomodoro), Work Log (manual start/pause/stop, projects, local history), Reports (daily/weekly totals, filters, CSV), safety (release inhibitors on disable/exit, conservative crash recovery), independent module switches, diagnostics, tests with real evidence.

## Platform baseline

- Primary declared target: Ubuntu 24.04 LTS `amd64` (Debian `.deb`).
- Actual development/validation machine (WP-00): Pop!_OS 22.04 LTS (Ubuntu 22.04 base), GNOME 42, X11, Qt 6.2.4, systemd 249, xdg-desktop-portal with GNOME backend. See `docs/wp00-environment-report.md` and `docs/capability-matrix.md`.

## Default stack (approved)

C++, Qt 6 Widgets, CMake, QtDBus, SQLite (via Qt SQL), Debian debhelper, Qt Test.

## Requirement traceability IDs

KAE-PROD-001, KAE-SAFE-001, KAE-AWAKE-001/002, KAE-LIFE-001, KAE-UI-001, KAE-FOCUS-001, KAE-WORK-001, KAE-DATA-001, KAE-REPORT-001, KAE-PRIV-001, KAE-PKG-001, KAE-TEST-001, KAE-REL-001 — tracked in `docs/traceability.md`.
