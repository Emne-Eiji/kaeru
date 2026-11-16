# Kaeru 0.2.0 — Delivery Report

**Date:** 2026-11-16
**Version:** 0.2.0-1 (amd64)
**Status:** release candidate delivered; **user acceptance is not declared**
**Baseline:** `d8ce6ae` (0.1.1 handoff) · **Release commits:** `472323a`,
`07b81f3`, `743ec78`, `cfebbbe`, `b67fc21`, plus the documentation commit
containing this report.

This report describes what was changed, what was verified directly, and what
was **not** verified. It does not treat passing tests as user acceptance.

## 1. Release scope

Kaeru 0.2.0 is a focused Keep Awake release:

- The GUI has three pages: **Keep Awake**, **Settings**, **Diagnostics**.
- Keep Awake offers 15 minutes, 30 minutes, 1 hour, 2 hours and
  **Indefinitely**. Choosing a new duration replaces the current request;
  requests never stack.
- The tray offers the same Keep Awake actions, status, Open, Settings and
  Quit.
- The CLI offers `status`, `awake on`, `awake indefinite`, `awake off`,
  `diagnostics`, `open` and `quit`.
- **Focus Timer, Work Log and Reports are deferred.** They are absent from
  the GUI and normal CLI; their source, tests and stored data are preserved.

## 2. Owner-reported issues — disposition

| Issue | Owner report | 0.2.0 disposition | Evidence |
|---|---|---|---|
| A | Two `.deb` files confuse delivery | **Addressed in delivery layout**: main `.deb` at `dist/0.2.0/`, optional dbgsym isolated under `debug-symbols/`; README names the one file to install | `dist/0.2.0/README.md`, package audit |
| B | 2 hours / Indefinite do not work while 15m/30m/1h do | **Fixed**: one replaceable manual session; GUI clicks verified with live portal | §3, §4 |
| C | Focus Timer design rejected | **Deferred**: hidden from GUI/CLI; source and tests retained; no redesign attempted | §5 |
| D | Work Log records work prematurely | **Deferred**: hidden from GUI/CLI; database not opened or modified; no session is created, checkpointed or closed | §5, §6 |
| E | Focused icon is broken | **Fixed** for runtime rendering: embedded UI icons come from the supplied PNG variants; SVG stays installed for desktop integration | §7 |

## 3. Keep Awake model and fixes

- `ManualAwakeSession` (`src/core/ManualAwakeSession.{h,cpp}`) owns exactly
  one manual request. `Application::beginManualAwake` adds the replacement
  request before removing the previous one, so the newest duration wins
  without a release/re-acquire gap.
- `awakeOff()` stops the session and suppresses automatic reactivation with
  the limitation text “turned off manually; choose a duration to start again”.
  A new explicit duration clears the suppression.
- The coordinator expiry signal now carries the original
  `InhibitionRequest` payload (reason and expiration policy), so the
  notification and UI no longer receive an empty placeholder.
- The portal backend subscribes to the **predicted request handle before**
  calling `Inhibit`. This closes the observed race where `Response(0)`
  arrived milliseconds after the method reply and was lost, leaving the app
  stuck “activating” while the portal held an orphan inhibitor.
- `release()` now also closes a request that is still activating, instead of
  only handling already-active handles.

## 4. Keep Awake verification (direct execution)

Environment: Pop!_OS 22.04, GNOME 42, Qt 6.2.4, X11. GUI clicks were driven
with `xdotool` against a real Kaeru window on `Xephyr :99`, using the real
session bus and `xdg-desktop-portal`; data was isolated with
`KAERU_TEST_HOME`. Status was read through the normal CLI/D-Bus path.

All checks below **passed** unless marked otherwise.

| Check | Observed result |
|---|---|
| 15 minutes | `status=active`, backend `portal`, requested/effective `system-sleep` + `session-idle`, remaining ≈ 896,000 ms |
| 2 hours after 15 minutes (replacement) | `remainingMs=7,196,863` (≈1 h 59 m 57 s), still `active`; the 15-minute request did not mask it |
| Indefinitely | `reason="Keep awake (indefinite)"`, `remainingMs=-1`, `status=active` |
| 30 minutes from the turned-off state | suppression cleared, `active`, remaining ≈ 1,798,000 ms |
| 1 hour after 30 minutes (replacement) | `active`, remaining ≈ 3,597,000 ms |
| Turn off now | `status=inactive`, no backend, limitation “turned off manually; choose a duration to start again” |
| 5-second timer expiry | request cleared automatically, `status=inactive`, no Kaeru inhibitor left |
| Quit while active | process exited; no Kaeru inhibitor left |
| `kill -9` while active then restart | no leaked inhibitor; restarted app reports `inactive` (no stale request is restored) |
| Autostart flags `--background --awake-on-login` | starts hidden (`background=true`), indefinite Keep Awake active, reason “Keep awake (started at login)”; `kaeru open` shows the window |
| Autostart settings toggle | writes `Exec=kaeru --background --awake-on-login`; disabling removes only Kaeru’s own entry |
| Tray via SNI properties | item registered; tooltip “Kaeru — keep awake active · 59m 59s”; 5 embedded icon pixmaps; tooltip returns to inactive after turn-off |

Unit coverage added/updated:

- `test_manualawake` — request factories, mode→capabilities, replacement,
  stop, remaining time, indefinite, until-time, invalid requests (8 cases).
- `test_coordinator` — expiry signal carries the original request id,
  reason and expiration policy, and the backend binding is released.
- `test_icons` — every embedded PNG size/state exists, icons expose raster
  sizes, expressions are visually distinct, focused artwork keeps green and
  light pixels.
- `test_cliparser` — combined startup flags, order independence, and
  rejection of flags mixed with a command.

**Build:** Release, zero compiler warnings. **Tests:** 15/15 suites pass.

Note: `systemd-inhibit --list` does not show portal inhibitors on this
system (the portal owns the logind inhibitor). Status/DBus evidence and the
absence of a Kaeru inhibitor after release were used instead; the command
alone is not proof.

## 5. Deferred features and data preservation

- `Module::Focus` and `Module::WorkLog` are forced off for this release.
  The GUI no longer constructs their pages; the CLI returns a clear
  not-available message; the D-Bus/CommandRouter commands remain gated and
  return a failure rather than acting.
- Focus and Work Log source, pages, controllers, repositories and unit tests
  remain in the tree and still compile/pass.
- Work Log storage setup is skipped entirely while the module is deferred:
  the database is **not opened, migrated or written**. This was verified in
  a sandbox by pre-seeding a SQLite database with a marker row, starting
  Kaeru, reading diagnostics and quitting: the database SHA-256 was
  identical before and after and the marker row remained.
- Diagnostics reports `Work Log storage: deferred` with the text “Work Log
  is deferred in this release; existing data is not opened or modified”.
- Interrupted-session recovery, 60-second checkpoints, suspend/lock
  boundaries, focus-to-work-log recording and quit-time session stop are all
  gated off while the module is disabled.

## 6. Packaging

| Item | Value |
|---|---|
| Main package | `kaeru_0.2.0-1_amd64.deb` — 328,640 bytes |
| Main package SHA-256 | `730bb344451a34d1530aeb7d5669cfa249116813a52269830f369cce61b3c720` |
| Debug symbols | `kaeru-dbgsym_0.2.0-1_amd64.ddeb` — 5,551,640 bytes, optional |
| Installed size | 872 KiB |
| Depends | Qt 6 Core/DBus/Gui/SQL/Widgets, `libqt6sql6-sqlite`, libc/libstdc++ |
| Recommends | `xdg-desktop-portal` |
| Lintian | only `W: initial-upload-closes-no-bugs` (informational) |
| AppStream | validation successful (one pedantic note) |
| md5sums | 32/32 files OK |
| desktop entry | `desktop-file-validate` clean |
| Maintainer scripts / conffiles | none; user data is not touched by install, upgrade or removal |

Release layout (`dist/0.2.0/`, not committed because `dist/` is ignored):

```
kaeru_0.2.0-1_amd64.deb          ← install this
kaeru_0.2.0-1_amd64.buildinfo
kaeru_0.2.0-1_amd64.changes
SHA256SUMS
README.md
debug-symbols/kaeru-dbgsym_0.2.0-1_amd64.ddeb   ← optional
```

`dist/0.1.1/` retains the previous artifacts for history. The main package
contains the binary, desktop entry, AppStream metadata, manpage, 21 PNG
icons (7 sizes × 3 expressions), 3 installed SVGs, README, license and
changelog.

## 7. Icon fix

The original artwork in `../frog_icons/` was **not modified**. The runtime
fix is an asset-selection change:

- The embedded Qt resource set now contains the supplied PNG variants at
  16, 24, 32, 48, 64, 128 and 256 px for `awake`, `focused` and `sleep`.
- `Icons.cpp` builds every `QIcon` from those PNGs.
- The installed hicolor SVGs remain for desktop menus/launchers that prefer
  scalable icons; Qt 6.2’s SVG mis-rendering of `focused.svg` is therefore
  off the application rendering path.
- `test_icons` asserts that the focused render contains green body pixels
  and light eye/highlight pixels, directly guarding the black-face
  regression.

## 8. Verified vs not verified

**Verified directly (this release work):**

- Clean Release build, zero warnings; 15/15 unit suites.
- GUI Keep Awake: every duration, replacement, indefinite, turn-off,
  expiry, quit cleanup, crash/restart, autostart startup flags.
- Portal activation/release through the live session portal.
- Autostart entry contents and removal.
- Tray SNI registration, tooltip and embedded pixmaps.
- Package build, lintian, AppStream, desktop validation, 32/32 md5sums.
- Work Log database untouched while deferred.

**Not verified — do not claim otherwise:**

- Real `dpkg -i` / `apt install` / upgrade / remove / reinstall lifecycle
  (requires root; no password available in this session).
- Ubuntu 24.04 (no such machine); only Pop!_OS 22.04 / Ubuntu 22.04 base.
- Wayland sessions.
- Real suspend/resume cycle.
- Audible notification sound and the visual desktop notification on expiry
  (wiring is present and policy is unit-tested; playback/display is
  daemon-side and was not observed).
- Live portal denial/fallback scenario in this release (backend failure
  paths are unit-tested with fakes).
- Long-duration wall-clock behaviour beyond the short expiry probe.
- User acceptance of the 0.2.0 GUI.

## 9. Requested root-assisted package testing

Please run the following on a disposable VM or a machine where the package
can be installed/removed. Record the exact commands and output.

```bash
# 1. Install
sudo apt install ./kaeru_0.2.0-1_amd64.deb
kaeru --version
kaeru status --json

# 2. Launch from the desktop menu (or `kaeru`), verify:
#    - Keep Awake page, all five duration buttons, Turn off
#    - Settings -> Startup toggle creates the autostart file
#    - Diagnostics reports a backend

# 3. Upgrade/remove/reinstall while preserving data
sudo dpkg -r kaeru
# confirm: binary gone, $XDG_DATA_HOME/kaeru/kaeru.db (if pre-existing) still present
sudo apt install ./kaeru_0.2.0-1_amd64.deb

# 4. Optional: same test on Ubuntu 24.04 and a Wayland session
```

Report the distribution, session type (X11/Wayland), install/upgrade/remove
results, and any difference from the verified behaviour above. A real
suspend/resume test and an audible notification check are also welcome but
remain separate from package acceptance.

## 10. Bottom line

The release candidate fixes the two functional blockers that could be
reproduced and verified here (Keep Awake duration replacement/portal
activation and the broken focused icon), makes autostart match the intended
behaviour, and removes the rejected/deferred features from the normal user
surface without deleting their source or data. The remaining acceptance
step is a real package lifecycle test with root and, ultimately, the user’s
own acceptance of the simplified GUI.
