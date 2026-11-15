# WP-03 — Tray, Single Instance, Autostart Evidence (partial: packaging blocked)

Environment: Pop!_OS 22.04 (Ubuntu base), GNOME 42, X11 `DISPLAY=:1`,
`XDG_RUNTIME_DIR=/run/user/1000`, user `banana` (uid 1000), systemd 249,
Qt 6.2.4. Binary: `kaeru 0.1.1` (build tree). All runs sandboxed with
`KAERU_TEST_HOME` (no real user data touched).

## 1. Tray (live X11 session) — PASS

```
rm -rf /tmp/opencode/kaeru-tray-test && mkdir -p /tmp/opencode/kaeru-tray-test
KAERU_TEST_HOME=/tmp/opencode/kaeru-tray-test DISPLAY=:1 XDG_RUNTIME_DIR=/run/user/1000 \
  ./build/kaeru &
```

Objective evidence (not just a screenshot):

- SNI watcher (`gnome-shell` owns `org.kde.StatusNotifierWatcher`) lists kaeru:
  `busctl --user get-property org.kde.StatusNotifierWatcher /StatusNotifierWatcher
   org.kde.StatusNotifierWatcher RegisteredStatusNotifierItems`
  → `as 2 ":1.90/org/ayatana/NotificationItem/caffeine_cup_empty" ":1.149/StatusNotifierItem"`
  (`:1.149` = kaeru's connection, pid 16120).
- Item properties:
  `busctl --user get-property :1.149 /StatusNotifierItem org.kde.StatusNotifierItem ToolTip`
  → `"Kaeru — inactive"` (state-reflecting).
  `Menu` → `/MenuBar` (DBusMenu registered, `org.freedesktop.DBusMenu.GetLayout`
  and `Event` present per introspect). `ItemIsMenu` → `false`.
- Main window exists: `xdotool search --name "Kaeru"` finds the window.
- Screenshot captured: `/tmp/opencode/tray-full.png` (1920×1080 x11grab frame;
  NOT committed — may contain unrelated desktop content; user can inspect).

Limitation (recorded, see §4): on THIS machine the tray icon image itself is
blank because the Qt6 SVG icon engine is absent (qtsvg not installed, sudo
blocked); tooltip/menu/window evidence above is unaffected.

## 2. Single instance (forward + exit) — PASS (after fix)

Commands:

```
KAERU_TEST_HOME=/tmp/opencode/kaeru-tray-test DISPLAY=:1 XDG_RUNTIME_DIR=/run/user/1000 \
  ./build/kaeru --background &          # primary instance
timeout 10 ./build/kaeru --background   # second launch
```

Expected: second launch forwards ShowWindow to the primary and exits 0;
instance count stays 1; primary keeps serving.

Actual: `second launch exit=0`; `instances after: 1`; `kaeru status` still
served by the primary.

Bugs found and fixed during this test (both recorded in CHANGELOG):

1. `SingleInstance::tryAcquire` passed `AllowReplacement`, so a second launch
   could take over the bus name instead of forwarding (spec §7 violation).
   Removed; D-Bus releases names of dead connections automatically.
2. `registerService` returns a **valid** reply with `ServiceNotRegistered`
   when the name is taken (name clash is not a D-Bus error). `m_primary =
   reply.isValid()` was true even on failure — the second launch ran as a
   second full instance. Fixed: `m_primary = reply.isValid() && reply.value()
   == ServiceRegistered`. Verified with the Qt 6.2 header
   (`qdbusconnectioninterface.h`: `ServiceNotRegistered=0, ServiceRegistered=1,
   ServiceQueued=2`).

## 3. Autostart backend (sandbox harness) — PASS

Harness `/tmp/opencode/autostart_harness.cpp` (throwaway, not part of the
project) exercises the real `AutostartBackend` with
`KAERU_TEST_HOME=/tmp/opencode/kaeru-autostart-test`. 15/15 checks pass:

```
entry path: <sandbox>/autostart/io.github.kaeru.Kaeru.desktop
PASS: not enabled initially / no entry file initially
PASS: enable() succeeds; isEnabled() true; entry exists
PASS: Exec line is 'kaeru --background'; X-GNOME-Autostart-enabled=true
PASS: exactly one new file in autostart dir; only Kaeru's own entry added
PASS: re-enable idempotent; disable() removes file; disable idempotent
PASS: autostart dir back to original contents (nothing else touched)
ALL PASS
```

Honest scope note: this verifies the backend (file lifecycle, opt-in path,
own-file-only guarantee). The Settings-page toggle that calls it
(`SettingsPage::onStartupChanged`) is wired to the same backend; GUI toggle
verification is manual (uiux) / covered by code review.

## 4. Environment gap: Qt6 SVG rendering unavailable locally — BLOCKS local icon verification

```
QT_QPA_PLATFORM=offscreen /tmp/opencode/harness_build/svg_probe
→ QIcon isNull=0   pixmap isNull=1 size=0x0
```

`libqt6svg6` is not installed (only Qt5's qsvgicon exists) and `sudo apt
install` requires a password this session cannot supply. Consequence:

- Tray icon image + window icon render blank on this dev machine (tooltips,
  menu, SNI registration unaffected — §1).
- The `.deb` will declare `libqt6svg6` as a runtime dependency, so installing
  the package on a normal desktop pulls the SVG icon engine and icons render.
- Local icon rendering re-test is scheduled once qtsvg is installed
  (requires the user to run the apt command; see packaging blocker below).

## 5. Packaging (`.deb`) — BLOCKED (missing toolchain, sudo password required)

`dpkg-buildpackage` (dpkg-dev) is present, but `debhelper`/`dh`, `lintian`,
`devscripts` are not installed and `sudo apt install -y debhelper lintian
devscripts` fails with "a terminal is required to read the password".
The packaging files (`debian/`) are not yet written; they will be added when
the toolchain is available (user-run install of the three named packages,
per the original authorization).

## 6. Not yet run (honest gaps)

- `.deb` build/install/upgrade/remove/reinstall with data preservation — §5.
- Icon rendering re-test — §4.
- Desktop-entry launch (needs installed package).
- Focus native run, Work Log, Reports — WP-04/05/06.