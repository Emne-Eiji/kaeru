# Kaeru — Open Issues and Unresolved Validation (0.2.0)

> Updated 2026-11-16 for the 0.2.0 release. This file supersedes the 0.1.1
> handoff status. Each item states what was changed, what was verified, and
> what remains open. Passing automated tests are **not** user acceptance.

## Resolution summary

| Issue | Owner report | 0.2.0 disposition | Where |
|---|---|---|---|
| A | Two `.deb` files confuse delivery | Addressed in the release layout: one main `.deb`; dbgsym isolated and labeled optional | `dist/0.2.0/README.md` |
| B | 2 hours / Indefinite “fail” while shorter durations work | **Fixed and GUI-verified**: one replaceable Keep Awake session; no stacked requests | `docs/DELIVERY_REPORT_0.2.0.md` §3–4 |
| C | Focus Timer design rejected | **Deferred**: removed from GUI/CLI; source, tests and data retained; no redesign attempted | §C below |
| D | Work Log records work prematurely | **Deferred and made read-only**: removed from GUI/CLI; the preserved database is not opened or written | §D below |
| E | Focused icon is broken | **Fixed for runtime rendering**: embedded PNG variants replace SVG rendering | §E below |

## Issue A — Two Debian packages (debug symbols)

**Disposition:** addressed as a delivery/instruction problem, not a
packaging bug.

- 0.2.0 produces `kaeru_0.2.0-1_amd64.deb` (328,640 B) and the automatically
  generated `kaeru-dbgsym_0.2.0-1_amd64.ddeb` (5,551,640 B).
- The release directory `dist/0.2.0/` contains the main package at its root
  and the dbgsym under `debug-symbols/`; `README.md` states that normal users
  install only the main `.deb`.
- The main package remains the complete user package: binary, desktop entry,
  AppStream metadata, manpage, PNG/SVG icons, README, license, changelog.
- `libqt6svg6` was removed from `Depends` because runtime icon rendering no
  longer uses SVG; the SVGs are installed for desktop integration.
- The dbgsym is standard debhelper 13 behavior and remains optional.

**Still open:** real package lifecycle testing requires root; see the request
in `docs/DELIVERY_REPORT_0.2.0.md` §9.

## Issue B — Keep Awake 2h / Indefinite **FIXED**

**Root cause (confirmed):** every chip added another `InhibitionRequest` to
the registry. The UI displayed the **earliest** expiry among stacked
requests, so a later 2h or Indefinite request was masked by an older 15m
request. The Indefinitely warning was also hidden by the 1-second refresh.

**Fix:**

- `ManualAwakeSession` owns exactly one manual request.
- A new duration adds its request first, then removes the previous request,
  so the newest duration wins without a release/re-acquire gap.
- The timer line is always present and the action chips do not move between
  states; one turn-off action appears only while a request is active.
- Turn-off stops the session and suppresses automatic reactivation until the
  user chooses a new duration.
- Expiry emits the original request payload (reason + policy) and clears the
  session.
- The portal backend subscribes to the predicted request handle **before**
  `Inhibit`, eliminating the activation race that could leave the UI stuck
  “activating”.

**Verified with real GUI clicks and the live portal:**

- 15m → active, remaining ≈ 896,000 ms.
- 2h after 15m → active, remaining ≈ 7,196,863 ms (1 h 59 m 57 s).
- Indefinitely → active, `remainingMs=-1`, reason “Keep awake (indefinite)”.
- 30m from the turned-off state, then 1h → both active with the correct
  remaining time.
- Turn off → inactive with the documented limitation text.
- 5-second timer → auto-cleared, no inhibitor left.
- Quit while active → process exits, no inhibitor left.
- `kill -9` → no leaked inhibitor; restart reports inactive.
- Autostart flags → hidden startup, indefinite active, window opens on
  demand.

**Unit coverage:** `test_manualawake` (8 cases) and the updated
`test_coordinator` expiry-payload assertion.

**Still open:** long-duration wall-clock behaviour beyond the short probe;
real suspend/resume (not tested).

## Issue C — Focus Timer **DEFERRED**

The owner’s rejection stands. No redesign was attempted in this release.

- `Module::Focus` is forced off; the Focus page is not constructed in the
  main window and the CLI reports the feature as unavailable.
- `FocusController`, `FocusPage`, timer engine support, notifications and
  their tests remain in the source tree and still compile/pass.
- Existing focus-session rows in the preserved database are not read or
  written by the user interface in this release.
- Re-enabling Focus requires a product decision about the interaction model
  (“horrible” was not decomposed into specific requirements) and a fresh
  design pass.

## Issue D — Work Log **DEFERRED AND READ-ONLY**

The owner’s “records work prematurely” report was not re-litigated. The
feature is removed from the user surface for this release.

- `Module::WorkLog` is forced off; the page and CLI commands are
  unavailable.
- Storage setup is skipped entirely: the existing database is **not opened,
  migrated, checkpointed or closed**.
- Tick, suspend/lock boundaries, focus-to-work-log recording, interrupted
  session recovery and quit-time stop are all gated off.
- Sandbox proof: a pre-seeded SQLite database with a marker row had the same
  SHA-256 before and after a background start, diagnostics read and quit;
  the marker row was unchanged.
- Diagnostics reports `Work Log storage: deferred` with the reason.

**Still open:** the product decision about recording semantics (idle
detection, quit behavior, expected start/stop) before the feature can return.

## Issue E — Focused frog icon **FIXED (runtime rendering)**

**Root cause (reproduced earlier):** Qt 6.2’s SVG renderer mis-paints
`focused.svg` (clip-path construct), producing a black face. The supplied
PNG variants rasterize correctly.

**Fix (asset selection, not redrawing):**

- The embedded Qt resource set now uses supplied PNGs at 16, 24, 32, 48, 64,
  128 and 256 px for all three expressions.
- `Icons.cpp` builds every runtime `QIcon` from those PNGs.
- The installed hicolor SVGs remain for desktop integration only.
- The original `../frog_icons/` artwork was not modified.
- `test_icons` verifies all embedded sizes exist, the three expressions are
  distinct, and the focused render retains green body and light eye pixels.

**Still open:** visual inspection in the user’s real top panel on their
desktop configuration (SNI properties and embedded pixmaps were verified;
the panel’s own desaturation/theme behavior is desktop-side).

## Remaining gaps / not verified

| # | Item | Status |
|---|---|---|
| 1 | Real `dpkg`/`apt` install, upgrade, remove, reinstall | **not tested** — root password unavailable; root-assisted procedure requested in the delivery report |
| 2 | Ubuntu 24.04 native validation | **not tested** — no machine; Pop!_OS 22.04 / Ubuntu 22.04 base only |
| 3 | Wayland session | **not tested** — X11 only |
| 4 | Real suspend/resume cycle | **not tested** — requires a real suspend; unit clock simulation only |
| 5 | Audible notification sound / visual expiry notification | **not observed** — wiring and policy tested; playback is daemon-side |
| 6 | Live portal denial / fallback scenario in 0.2.0 | **not tested live** — backend failure paths are unit-tested with fakes |
| 7 | Long-duration expiry (hours) | **not tested** — short expiry probe only |
| 8 | Tray visual in the real panel | **partial** — SNI item, tooltip and 5 pixmaps verified; panel theme/desaturation is desktop-side |
| 9 | User acceptance of the simplified GUI | **pending** — this report is not acceptance |

## Status register

- **Verified by direct execution (0.2.0):** Release build zero warnings;
  15/15 suites; every Keep Awake duration and replacement; turn-off;
  5-second expiry; quit cleanup; kill/restart; autostart flags; SNI tooltip
  and pixmaps; package build/lintian/AppStream/md5sums; preserved database
  byte-identical.
- **Verified by source inspection:** deferred-module gating; PNG-only runtime
  icon path; portal handle subscription ordering.
- **Not independently verified:** Ubuntu 24.04, Wayland, real suspend/resume,
  audible sound, real panel visuals, dpkg lifecycle.
- **Requires product-owner decision:** Focus redesign (C), Work Log recording
  semantics (D), acceptance of the 0.2.0 GUI.
- **Blocked:** root package lifecycle testing (password).

## Do-not-touch list

- `../frog_icons/` — original artwork; must remain byte-identical.
- `Kaeru_Documentation_Package_v0.1.1/MASTER_SPECIFICATION.md` — do not
  rewrite the approved spec.
- Git history — do not rewrite/reset; do not push anywhere.
- User data (real `$XDG_*` dirs) — all testing must stay in
  `KAERU_TEST_HOME` sandboxes.
- Do not re-enable Focus or Work Log in the user surface without an explicit
  product decision and acceptance plan.
