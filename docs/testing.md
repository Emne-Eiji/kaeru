# Kaeru — Testing Strategy and Evidence Plan

**Rule:** every recorded test lists exact command, environment, expected,
actual, pass/fail/not-run, evidence path, limitations. Mocked success is
never labeled as native validation. Passing tests are not user acceptance.

This document reflects **0.2.0** (Keep Awake release). Sections covering
Focus/Work Log remain for the preserved code and future re-enablement.

## 1. Unit tests (Qt Test, offscreen, injected clocks — deterministic)

| Suite | Covers |
|---|---|
| test_clocks | monotonic vs boot vs wall semantics; fake clocks; suspend simulation by advancing fake clocks; duration/date parsing (accept + reject) |
| test_registry | add/remove/expire (duration + until), earliestRemaining, priority reasons, clear + changed signal |
| test_coordinator | capability union/diff, acquire/release against fake backend, backend loss/denial/failure, suppression, module disable, Pause All (no auto-restart), requested≠effective, expired signal carries the original request payload |
| test_manualawake | single manual session: timed/indefinite/until factories, mode→capabilities, replacement, stop, remaining time, invalid requests (8 cases) |
| test_timerengine | countdown expiry (boot), stopwatch suspend pause (monotonic), wall-deadline, fires-once, cancel, invalid inputs |
| test_focus | Pomodoro transitions, pause freeze/resume, skip/extend, upcoming-work override, stop cancels, quiet-hours windows — preserved for the deferred module |
| test_notify | notification policy decisions (notify option, quiet hours, keep-awake expiry gate) |
| test_worklog | open-segment policy, paused exclusion, no-overlap, single active session, checkpoints never counted, focus sessions separate, daily totals offset — preserved code |
| test_workmanager | work lifecycle, idempotent start, project switch, suspend/lock boundaries, conservative recovery, focus→focus_sessions — preserved code |
| test_storage | migrations versioned/idempotent, project CRUD + guards, session round-trip, FK integrity, backup-before-migration, WAL + integrity — preserved code |
| test_csv | quoting, formula-injection neutralization, header + open-segment rows, write/truncate + row count, filename convention — preserved code |
| test_reports | per-project totals, focus totals, open segments never count — preserved code |
| test_traystate | tray presentation mapping: all state combinations, precedence, honesty rules (ADR-008) |
| test_cliparser | grammar, malformed values rejected, defaults, missing operands, combined startup flags, flags-vs-command rejection |
| test_icons | all 21 embedded PNG variants exist at the right size; state icons expose raster sizes; expressions are distinct; focused artwork retains green/light pixels |

Run:

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru/build"
ctest --output-on-failure -j"$(nproc)"
```

Result at release: **15/15 suites pass** on the clean Release build, ~0.2–0.4 s.
The suites set `QT_QPA_PLATFORM=offscreen` and isolate data with
`KAERU_TEST_HOME`; the real user home is never touched.

## 2. GUI acceptance (real workflows, 0.2.0)

Harness used:

```bash
Xephyr :99 -screen 1200x800 -no-host-grab &
export DISPLAY=:99
export KAERU_TEST_HOME=/tmp/opencode/kaeru-accept
export XDG_RUNTIME_DIR=/run/user/1000
./build/kaeru &
# clicks via xdotool; screenshots via ffmpeg -f x11grab
```

This is a real Qt window with the real session bus and portal; only the
display is nested. Clicks used the stable layout (chips at y≈314; Turn off
at ≈(313,378) in the 1200×800 harness).

| Workflow | Expected | Result |
|---|---|---|
| Click 15 minutes | active, portal backend, both capabilities, ~15 min remaining | pass |
| Click 2 hours while 15m active | replaces 15m; remaining ~1 h 59 m 57 s | pass |
| Click Indefinitely while timed active | `remainingMs=-1`, reason indefinite, active | pass |
| Turn off | inactive, no backend, documented limitation text | pass |
| Click 30m after turn-off | suppression cleared, active ~30 m | pass |
| Click 1h after 30m | replaces 30m, active ~1 h | pass |
| 5 s timer | auto-clears at expiry, inactive, no inhibitor | pass |
| Quit while active | process exits, inhibitor released | pass |
| `kill -9` then restart | no leaked inhibitor; app reports inactive | pass |
| Autostart flags | hidden start, indefinite active; `kaeru open` shows window | pass |
| Settings → Startup | writes/removes only Kaeru's own autostart entry | pass |
| Tray (SNI properties) | item registered, tooltip with state/remaining, 5 pixmaps | pass (properties; panel visuals not re-inspected) |

Screenshots from the session are in `/tmp/opencode/` (throwaway, not
committed). The delivery report records the exact observed status values.

## 2a. Tray menu status/action correction (post-0.2.0, 2026-11-17)

**Deployment correction:** the first pass below used `build/kaeru` from the
development tree. The installed `/usr/bin/kaeru` remained 0.2.0-1, which
predates the source fix, so the user's actual application was unchanged.
The fix was repackaged as 0.2.0-2, installed, and then verified in the real
GNOME session. The development-build pass is supplementary evidence; the
installed-package acceptance table below is the authoritative result.

The tray menu was tested as an actual menu, not just as tooltip/pixmap
properties:

- **SNI path:** the running app registered with the session
  `org.kde.StatusNotifierWatcher`. The exported menu was read from the
  item's `/MenuBar` object over `com.canonical.dbusmenu` (labels, enabled
  flags, separators), actions were triggered through DBusMenu `Event`, and
  the menu was also opened on the real GNOME panel with right-click for
  screenshots.
- **XEmbed path:** `stalonetray` in Xephyr hosted the same
  `QSystemTrayIcon`; the local Qt menu was opened with right-click and driven
  with `xdotool` (mouse and Down/Right/Return). This verifies the actual
  rendered labels and enabled states, not only exported properties.
- **Failure/denial path:** an isolated session/system bus ran a fake
  `org.kde.StatusNotifierWatcher` and fake
  `org.freedesktop.portal.Desktop`. The fake portal delayed its
  `org.freedesktop.portal.Request.Response` and then returned code 1
  (denied); logind could not rescue the request because the system bus was
  isolated too. This exercises the real coordinator, tray state mapping and
  menu export with a simulated backend (clearly not native portal evidence).

| Workflow | Expected | Actual | Result |
|---|---|---|---|
| Initial state | `Kaeru: OFF`, status readable (not grey), `Turn off Kaeru` disabled | exact match; status action enabled | pass |
| Tray → 15 minutes | `ACTIVATING…` until confirmed, off disabled | `Kaeru: ACTIVATING…`, off disabled | pass |
| Activation confirmed | `Kaeru: ON`, off enabled, without reopening the menu | exact match | pass |
| Click `Turn off Kaeru` | real inhibition released; `Kaeru: OFF`; off disabled | status inactive, no bindings, off disabled | pass |
| Main GUI → 15 minutes | tray menu changes to ON without reopen/restart | exact match (menu screenshot) | pass |
| Tray → 2 hours while GUI open | GUI updates to ~1 h 59 m; one request, no stack | GUI showed `1h 59m`; two capability bindings (sleep + idle), not duplicate requests | pass |
| 5 s timed request expiry | status returns to OFF and off disabled | exact match after expiry; no binding left | pass |
| Indefinitely → off | ON while held, OFF after manual deactivation | exact match | pass |
| Denied activation | `Kaeru: ERROR`, never ON; off disabled | exact match; tooltip carried the denial detail | pass |
| Failure (`Failed`) | `Kaeru: ERROR`, never ON; off disabled | covered by `test_traystate` mapping and coordinator failure tests | pass (unit) |

The exact labels were also asserted in `test_traystate`:
`Kaeru: OFF`, `Kaeru: ON`, `Kaeru: ACTIVATING…` and `Kaeru: ERROR`, with
the off action enabled only for `Active`/`PartiallyActive`.

### Installed-package acceptance (0.2.0-2)

Running binary `/usr/bin/kaeru` (md5 `53eb7ec1eeb60c48b078b4e9a0a2e9a8`),
matching the packaged 0.2.0-2 binary. The process was launched through
`/usr/share/applications/io.github.kaeru.Kaeru.desktop`
(`GIO_LAUNCHED_DESKTOP_FILE`). The actual GNOME panel menu was opened and
clicked with `xdotool`; the same process's exported DBusMenu was queried
for the exact labels and enabled flags.

| Workflow | Expected | Actual | Result |
|---|---|---|---|
| Package identity | installed app is 0.2.0-2, not 0.2.0-1 | `dpkg-query` 0.2.0-2; `/usr/bin/kaeru` hash equals packaged binary | pass |
| Initial / after off / after expiry | `Kaeru: OFF`, off disabled | actual GNOME menu `Kaeru: OFF`; `Turn off Kaeru` greyed | pass (screenshot) |
| Tray → 15 minutes | ON after confirmation, off enabled | actual menu `Kaeru: ON`; off enabled; tooltip active | pass (screenshot) |
| Click `Turn off Kaeru` | real portal inhibition released; OFF; off disabled | actual mouse click; status inactive; `diagnostics` bindings `[]`; menu OFF/disabled | pass |
| Main GUI → 2 hours | tray updates to ON / ~1 h 59 m | tray tooltip `1h 59m`; menu ON; GUI `1h 59m` | pass (screenshot) |
| 5 s expiry | OFF and off disabled without reopening the app | actual menu OFF; bindings `[]` | pass |
| `ACTIVATING…` visual | exact label while pending | production portal confirmed in <0.2 s, too fast to capture; exact mapping is unit-tested and was observed with the delayed fake backend | pass (unit/supplementary) |
| Activation failure (`ERROR`) | exact label, never ON, off disabled | not forced against the production portal; mapping unit-tested and fake-denial pass | pass (unit/supplementary) |

Screenshots: `/tmp/opencode/actual-menu-old-crop.png` (old 0.2.0-1 stale
menu), `/tmp/opencode/actual-submenu-on-crop.png` (installed 0.2.0-2,
`Kaeru: ON` with `Turn off Kaeru` enabled),
`/tmp/opencode/actual-submenu-off-after-expiry-crop.png` (installed
0.2.0-2, `Kaeru: OFF` with `Turn off Kaeru` disabled). These are throwaway
paths, not committed.

## 3. Deferred-module read-only verification (0.2.0)

- Pre-seed a sandbox database with a marker row, record its SHA-256.
- Start Kaeru with `--background`, call `diagnostics --json`, then quit.
- Observed: hash unchanged, marker row intact, diagnostics
  `work-log-storage.status = deferred`.
- This proves the deferred Work Log does not open, migrate, checkpoint or
  close the preserved database.

## 4. Integration tests (real desktop)

| Test | Method | Evidence |
|---|---|---|
| Portal inhibit acquire/release | GUI durations + `kaeru status --json` | active with `portal` backend; inactive and no Kaeru inhibitor after release/turn-off/quit |
| Logind sleep inhibit | 0.1.1 evidence; portal is the primary path in 0.2.0 | `docs/evidence-wp02.md`; not re-run as a standalone test in 0.2.0 |
| Denial/failure path | unit-tested with fake backend | `test_coordinator`; live portal denial not tested in 0.2.0 |
| Timed expiry | 5 s request | auto-cleared; no inhibitor |
| Suppression | turn off, then choose a duration | new duration clears suppression and activates |
| Quit cleanup | quit while active | inhibitor released |
| Single instance | second launch | forwards to the running instance (0.1.1 evidence; unchanged path) |
| Autostart toggle | Settings → Startup | file created/removed; exact Exec verified |
| Tray presence | SNI property query | registered item, tooltip, pixmaps |
| Tray menu content/actions | DBusMenu labels/enabled flags + `Event`; XEmbed menu clicks | `Kaeru: OFF/ON/ACTIVATING…/ERROR`, off action gated on confirmed activation; see §2a |
| CLI/IPC | all 0.2.0 verbs | meaningful output/exit codes; deferred commands report not available |
| Real suspend/resume | **manual only** | not run; user session protection |
| Audible notification | **manual only** | not observed |

Note: portal inhibitors do not appear in `systemd-inhibit --list` on this
system; app status/DBus evidence is authoritative, and the command is used
only to confirm no Kaeru logind inhibitor remains.

## 5. Packaging tests

- `dpkg-buildpackage -us -uc -b` clean build.
- `lintian kaeru_0.2.0-1_amd64.deb` → only informational
  `initial-upload-closes-no-bugs`.
- `appstreamcli validate --no-net` → successful (one pedantic note).
- `desktop-file-validate` → clean.
- Package md5sums → 32/32 OK; SHA-256 recorded in `dist/0.2.0/SHA256SUMS`.
- Extract-run of `/usr/bin/kaeru` from the `.deb` → reports 0.2.0.
- **Not run (root required):** `dpkg -i`/`apt install`, upgrade, remove,
  reinstall, and user-data preservation across those operations. Procedure
  and request are in `docs/DELIVERY_REPORT_0.2.0.md` §9.

## 6. Resource measurement (0.1.1, not repeated in 0.2.0)

- Idle CPU target < 0.5 % of one core; RSS target < 100 MB background.
- 0.1.1: RSS 55.6 MB, idle CPU 0.26 % over 300 s.
- 0.2.0 changes are not expected to regress this materially, but no fresh
  measurement was taken; do not cite the old numbers as 0.2.0 evidence.

## 7. Known test limitations (declared)

1. Real suspend/resume not auto-triggered — manual procedure only.
2. Ubuntu 24.04 native validation not performed (no machine).
3. Wayland/KDE not evaluated.
4. Tray menu *content/actions* are verified through DBusMenu and the
   XEmbed-rendered Qt menu; a specific panel's theme, icon scaling and
   placement remain desktop-side and are not asserted.
5. DST machine-clock change not performed; validated via TZ injection.
6. The user installed 0.2.0-2 (uninstall 0.2.0-1 + install 0.2.0-2) and it
   was verified in the real session. The full automated
   upgrade/downgrade/remove/reinstall matrix remains blocked by sudo.
7. Audible notification playback is daemon-side and was not observed.

## 8. WP-08 / WP-09 verification (0.2.0-3 checkpoint)

- Clean Release build: zero warnings; `ctest`: 20/20 suites passing, including
  new suites for duration validation/persistence (`awakeduration`,
  `settings`), update parsing/error honesty (`updates`), complete theme
  application (`theme`), safe window restore (`windowgeometry`) and
  interactive tray state/detail (`traystate`).
- Real GNOME session (development binary, isolated `KAERU_TEST_HOME`): the
  exported DBusMenu showed the status item as
  `Kaeru: OFF\nInactive`, then `Kaeru: ON\n… remaining` with a green status
  dot after a tray left-click activation. Right-click opened the same compact
  menu without changing state. The menu contained `Custom…` immediately below
  `Indefinitely` and no `Turn off Kaeru` action.
- Installed 0.2.0-3 verification (binary md5 `44000dc6…`, matching the staged
  package): right-click opened the compact menu without changing state;
  the exported status item showed `Kaeru: OFF\nInactive` (red dot) and
  `Kaeru: ON\nIndefinite` (green dot); the expanded submenu showed
  `Custom…` below `Indefinitely` and no `Turn off Kaeru` action. Clicking the
  status item over DBusMenu toggled active → inactive → active with real
  portal inhibition. Selecting 15 minutes and then 2 hours while active
  restarted the timer (≈15 m, then ≈2 h); `Custom…` produced 1 h 30 m and
  saved `custom:5400000`. After restarting Kaeru, that custom choice remained
  and an SNI `Activate` call started a fresh 1 h 30 m session.
- Screenshot evidence (throwaway paths):
  `/tmp/opencode/final-menu-off2-crop.png` (installed OFF),
  `/tmp/opencode/final-menu-on-crop.png` (installed ON),
  `/tmp/opencode/final-submenu-kbd-crop.png` (expanded submenu),
  `/tmp/opencode/final-custom-dialog-crop.png` (custom dialog),
  `/tmp/opencode/final-gui-overview-crop2.png` (main GUI),
  `/tmp/opencode/theme-fix2-crop.png` and
  `/tmp/opencode/theme-dark-dialog-crop.png` (complete dark theme).
- Theme coverage on the refreshed installed package (binary md5
  `2e966b72…`, matching `dist/0.2.0-3/kaeru_0.2.0-3_amd64.deb` SHA-256
  `348cf18c…`): System/Light and Dark were each applied live; selecting Dark
  changed sidebar, cards, central content area, table, inputs and modal
  dialog together, and Light restored all of them. Screenshots:
  `/tmp/opencode/theme-fix2-crop.png` (Dark),
  `/tmp/opencode/theme-dark-dialog-crop.png` (Dark dialog),
  `/tmp/opencode/final-light2-crop.png` (Light),
  `/tmp/opencode/final-diagnostics-crop.png` (Dark table). Preference was
  restored to System at the end.
- About/update-check on the installed package:
  `/tmp/opencode/final-about-crop.png` shows official artwork, 0.2.0,
  license/acknowledgements, the official repository link and no Quit button;
  `/tmp/opencode/final-update-check-crop.png` shows the honest result
  “No published release is available yet. Update status is unknown.” — not a
  false “up to date” claim.
- Limitation — GNOME left-click: `QSystemTrayIcon::Trigger` toggles correctly
  where the platform delivers it, and a direct SNI `Activate` call toggles
  the installed app. On this GNOME 42 AppIndicator host, a physical left-click
  on the icon is consumed by the shell (no SNI method call observed) and does
  not toggle; right-click opens the menu. This is a shell integration
  limitation; the interactive status item and duration menu remain fully
  functional. The exact status-item click is the recommended toggle path on
  this host.
- Limitation: DBusMenu labels are plain text, so individual-word colors and
  per-line font sizes cannot be expressed in the GNOME panel. The status dot
  is green/red and the text is exact; Qt-hosted menus use rich text.
- Full implementation record: `docs/handoff/KAERU_WP08_WP09.md`.
