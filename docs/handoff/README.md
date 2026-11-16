# Kaeru Handoff Package — README (entry point)

This directory is the primary handoff for the next coding agent taking over
Kaeru. The **0.2.0 release candidate** focuses on dependable Keep Awake and
simplifies the GUI. The owner's earlier reports were handled as follows:

- Keep Awake 2h/Indefinite: **fixed** (single replaceable request; GUI and
  portal verified).
- Focused frog icon: **fixed** for runtime rendering (supplied PNG assets).
- Two `.deb` files: **delivery clarified** (one main package; optional
  dbgsym isolated).
- Focus Timer and Work Log: **deferred** (hidden from GUI/CLI; source, tests
  and data preserved). The Work Log database is not opened or written.

**Still pending:** user acceptance, root package lifecycle testing, Ubuntu
24.04/Wayland, real suspend/resume, audible sound. See
`docs/DELIVERY_REPORT_0.2.0.md`.

## Reading order

1. **`NEXT_AGENT_ONBOARDING.md`** — start here: paths, environment, build/
   test/run commands, source map, approval boundary, honest status.
2. **`../../docs/DELIVERY_REPORT_0.2.0.md`** — what was fixed/verified/
   deferred and the root-assisted test request.
3. **`KAERU_PROJECT_HISTORY.md`** — product philosophy and the WP-00…WP-07
   development history, plus the 0.2.0 addendum.
4. **`KAERU_TECHNICAL_ARCHITECTURE.md`** — layering and subsystem walkthrough
   (keep awake, clocks, focus, work log, storage, platform, UI, icons,
   packaging). Focus/Work Log sections describe preserved but deferred code.
5. **`KAERU_CURRENT_STATE.md`** — exact Git/build/test/package state, release
   artifacts and checksums.
6. **`KAERU_OPEN_ISSUES.md`** — per-issue disposition, remaining gaps and
   do-not-touch list.

## One-paragraph summary

Kaeru 0.2.0 is a Qt 6 (Widgets) Linux Keep Awake utility with a clean
Release build (zero warnings), 15/15 sandboxed test suites, verified portal
activation/release and every duration/replacement flow, a simplified
three-page GUI (Keep Awake, Settings, Diagnostics), corrected frog icon
rendering from the supplied PNGs, and a lintian-clean Debian package
(`kaeru_0.2.0-1_amd64.deb`). Focus Timer, Work Log and Reports are deferred
without deleting source, tests or user data. The product is **not yet
accepted** and the package has **not** been through a root install/upgrade/
remove/reinstall cycle.

## Ground rules (restated)

- Do not modify the original artwork (`../frog_icons/`) or the approved
  Master Specification.
- Do not rewrite Git history; do not push to any remote.
- Do not trust completion claims; re-verify behavior; never weaken tests.
- Do not re-enable Focus or Work Log without an explicit product decision.
- Keep all testing in `KAERU_TEST_HOME` sandboxes.
