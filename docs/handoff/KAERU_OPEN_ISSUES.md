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
| F | Tray always shows `Keep Awake: inactive`; “Turn off” permanently disabled | **Fixed, packaged in 0.2.0-2, and verified in the installed app**: live coordinator state refreshes menu + action; tray labels shortened to `Kaeru` | §F below |

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

## Issue F — Tray status stale, turn-off action disabled **FIXED**

**Report:** the tray menu always showed `Keep Awake: inactive` (grey), and
`Turn off Keep Awake now` was permanently unavailable even while Keep Awake
was confirmed active.

**Root causes (confirmed):**

1. The menu was rebuilt only on `AppState::changed` and when the tray was
   first created. `InhibitionCoordinator::stateChanged` — the signal emitted
   after activation, release, expiry, failure and denial — was wired only to
   `updateIcon()`. The icon/tooltip therefore changed while the already-built
   menu text and action enablement stayed frozen at the initial OFF state.
2. The off action was created once with
   `state.status != EffectiveStatus::Inactive`. From the initial OFF state it
   was permanently disabled. The condition was also too broad: a fresh build
   would have enabled it during `Activating`, `Unsupported`, `Failed` and
   `PermissionDenied` instead of only when inhibition was confirmed.
3. The action itself was not broken: it already called
   `Application::awakeOff()` → `ManualAwakeSession::stop()` +
   `InhibitionCoordinator::suppressAll()`. The defect was entirely the stale
   menu/enablement path.

**Fix:**

- Connect both `InhibitionCoordinator::stateChanged` and `AppState::changed`
  to a single refresh that rebuilds/refreshes the menu and icon together.
- Build the menu once, then update the status action and off action from
  `Application::awakeState()` on every relevant state change.
- Map the compact menu status exactly: `Kaeru: OFF` (Inactive),
  `Kaeru: ON` (Active/PartiallyActive), `Kaeru: ACTIVATING…` (Activating),
  `Kaeru: ERROR` (Failed/Denied/Unsupported). The status action is enabled
  (readable, not painted unavailable) and carries remaining/limitation/error
  detail in its tooltip.
- Enable `Turn off Kaeru` only for confirmed `Active`/`PartiallyActive`.
- Rename the tray submenu to `Kaeru` and the action to `Turn off Kaeru`; the
  main GUI keeps the descriptive `Keep Awake` name.
- Add `TrayMenuState`/`decideTrayMenuState()` and unit tests covering all
  seven `EffectiveStatus` values and action enablement.

**Verified with real menu interaction:**

- Initial OFF; 15 minutes from the tray; `ACTIVATING…` (delayed fake portal);
  confirmed ON with off enabled; clicking `Turn off Kaeru` released the real
  portal inhibition and returned to OFF with off disabled.
- Main GUI 15 minutes → tray menu changed to ON without reopening or
  restarting; tray 2 hours → main GUI showed `1h 59m`.
- 5-second expiry → OFF; indefinite → ON → manual off → OFF.
- Fake portal denial (isolated bus, not native portal evidence) → `ERROR`,
  never ON, off disabled.
- `diagnostics` showed two capability bindings for one manual request
  (system sleep + idle), not duplicate/stacked requests.

Procedure and exact results: `docs/testing.md` §2a.

**Deployment gap found after the first report (2026-11-17):** the source fix
and the first verification used `build/kaeru`, but the installed application
was still `/usr/bin/kaeru` 0.2.0-1 (md5 `474d240a…`, built 2026-11-16
11:51) from `dist/0.2.0/kaeru_0.2.0-1_amd64.deb`. The desktop entry and
autostart both call `kaeru`, so GNOME kept launching the old installed
binary and the user saw the old labels and frozen menu. This was a
packaging/deployment gap, not a source regression.

Resolution: `debian/changelog` now has 0.2.0-2; the package was rebuilt and
installed. The actual running process is `/usr/bin/kaeru` launched through
`/usr/share/applications/io.github.kaeru.Kaeru.desktop`
(`GIO_LAUNCHED_DESKTOP_FILE`), binary md5
`53eb7ec1eeb60c48b078b4e9a0a2e9a8`, and the real GNOME menu was verified to
show `Kaeru: OFF/ON`, the shortened `Kaeru` submenu, and the correctly gated
`Turn off Kaeru`. The `.ddeb` is optional debug symbols, not the
application; the installable app is `kaeru_0.2.0-2_amd64.deb`.

**Still open:** a specific desktop panel's theme, icon scaling and placement
remain host-side; native portal denial and a visible `ACTIVATING…` state were
not forced against the production portal (the production portal confirms too
quickly). Failure/denial and pending mapping use the fake-backend harness
plus unit tests.

## Remaining gaps / not verified

| # | Item | Status |
|---|---|---|
| 1 | Real `dpkg`/`apt` install, upgrade, remove, reinstall | **partially tested** — user uninstalled 0.2.0-1 and installed 0.2.0-2, verified in the real session; full automated lifecycle matrix remains blocked by root password |
| 2 | Ubuntu 24.04 native validation | **not tested** — no machine; Pop!_OS 22.04 / Ubuntu 22.04 base only |
| 3 | Wayland session | **not tested** — X11 only |
| 4 | Real suspend/resume cycle | **not tested** — requires a real suspend; unit clock simulation only |
| 5 | Audible notification sound / visual expiry notification | **not observed** — wiring and policy tested; playback is daemon-side |
| 6 | Live portal denial / fallback scenario in 0.2.0 | **not tested live** — backend failure paths are unit-tested with fakes |
| 7 | Long-duration expiry (hours) | **not tested** — short expiry probe only |
| 8 | Tray menu and visual in the real panel | **menu verified** — exported DBusMenu labels/enabled flags and actions, XEmbed-rendered menu clicks, and GNOME panel screenshots; a specific panel's theme/scaling remains desktop-side |
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
