# Kaeru — Execution Plan and Risk Register

**Date:** 2026-11-15 · **Authorization:** user instruction 2026-11-15 (WP-00…WP-07 autonomous)

## Execution plan

| Phase | Deliverable | Gate |
|---|---|---|
| WP-00 | Read-only environment report (`docs/wp00-environment-report.md`) | complete — no modifications |
| WP-01 | Architecture, ADRs, UI/UX design, capability matrix, test strategy, traceability | design package committed |
| WP-02 | Native Keep Awake: registry, coordinator, portal + logind backends, clocks, minimal UI slice, unit + real integration evidence | build green, inhibition demonstrated |
| WP-03 | Single instance, autostart (opt-in), tray + fallback, icons, `.deb` (debhelper), packaging tests | package installs/launches/removed cleanly |
| WP-04 | Focus timer state machine, Pomodoro, notifications, optional links to Keep Awake/Work Log | state machine tests pass |
| WP-05 | Work Log: sessions/segments/projects, SQLite persistence/migrations, recovery checkpoints, lock/suspend pause | invariants verified, recovery honest |
| WP-06 | Reports, filters, CSV export with formula-injection protection, data deletion | totals reconcile, CSV safe |
| WP-07 | Hardening: full regression, packaging tests, resource measurement, docs, release candidate artifacts + checksums | release gate evidence complete |

## Risk register

| ID | Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|---|
| R-01 | Dev machine is Pop!_OS 22.04 (Ubuntu base), not Ubuntu 24.04 | Package target mismatch claims | certain | Honest capability matrix; use Qt 6.2-compatible APIs; `.deb` built for amd64 with runtime deps resolved by `dh_shlibdeps`; document gap |
| R-02 | Real suspend test disrupts user's session | environment damage | low | No auto suspend trigger; verify inhibition via logind `CanSuspend` + `systemd-inhibit --list` + portal response; manual procedure documented |
| R-03 | Qt 6.2 lacks some newer APIs | build failures | medium | Compile against 6.2 (floor); avoid 6.3+ APIs |
| R-04 | Portal inhibit behavior differs from docs on GNOME 42 | wrong backend logic | medium | Live tests in WP-02; logind fallback backend |
| R-05 | Tray icon not shown by GNOME 42 despite SNI | UX failure | low | Fallback visible control window + evidence tests; SNI verified present |
| R-06 | debhelper install needed (sudo) | authorization scope | low | Named packages only (`debhelper lintian devscripts`) |
| R-07 | AppIndicator tray menu not scriptable | test evidence gap | medium | Screenshot evidence + SNI registration evidence; document limitation |
| R-08 | Clock-change/DST edge cases | data honesty | medium | Boot-clock for countdowns; wall-deadline recompute; TZ-injection tests |
| R-09 | Crash mid-transaction corrupts DB | data loss | low | WAL + transactions + backup-before-migration; recovery checkpoints |
| R-10 | User data contamination during tests | privacy | low | `KAERU_TEST_HOME` env override for XDG paths in all tests |

## Engineering conventions

- C++17, Qt 6.2 floor, CMake ≥3.16.
- Core logic has no GUI dependencies; platform adapters behind interfaces; clocks/storage injectable.
- Test data under a dedicated test home directory; never touch real user data.
- Commits: small, reviewable, one logical change each.
