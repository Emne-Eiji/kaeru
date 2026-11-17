# Kaeru — WP-08 / WP-09 implementation handoff

This document records the implementation at the 0.2.0-3 checkpoint. Earlier
0.2.0/0.2.0-2 records remain authoritative for the baseline Keep Awake fix.

## 1. Shared duration model (WP-08)

`src/core/AwakeDuration.h/.cpp` is the single source of truth for the six
user-facing choices: 15 minutes, 30 minutes, 1 hour, 2 hours, Indefinitely
and Custom. A custom duration is finite whole hours/minutes, from 1 minute
to 7 days (168 hours). Seven days was chosen because it is the existing
limit of the CLI parser (`Duration.cpp`); no narrower limit was invented.

Persistence uses the existing `QSettings` store via:

- `Settings::lastAwakeDurationToken()` / `setLastAwakeDurationToken()`
- tokens: `15m`, `30m`, `1h`, `2h`, `indefinite`, `custom:<milliseconds>`
- corrupt or unknown tokens fall back to `1h` in `Application::lastAwakeDuration()`

`Application::selectAwakeDuration()` persists the choice then starts or
replaces the single manual request. `activateLastAwakeDuration()` starts a
fresh request (new creation timestamp), and `toggleKeepAwake()` turns off a
confirmed active request or starts the last selection otherwise. Turning off
does not erase the saved choice. Tray and GUI call these methods rather than
implementing separate timer logic.

## 2. Tray (WP-08)

`TrayState.h` now carries `statusLine`, `detailLine`, `active` and `error`.
The menu structure remains compact:

1. interactive status (`Kaeru: ON/OFF/…`, green/red indicator, gray detail)
2. `Kaeru` duration submenu ending in `Custom…`
3. Open Kaeru / Settings
4. Quit Kaeru

`Turn off Kaeru` is gone. Clicking status toggles via
`Application::toggleKeepAwake()`. `Custom…` uses the same
`CustomDurationDialog` as the GUI.

### GNOME integration note

Qt exports the QSystemTrayIcon context menu over `com.canonical.dbusmenu` on
GNOME. DBusMenu labels are plain text; rich text exported by Qt would appear
as literal markup in the GNOME panel. The status item therefore uses plain
two-line text for DBusMenu hosts and carries ON/OFF state color in a generated
status dot icon. Qt-hosted menus receive rich text with a colored state word
and smaller gray detail. Left-click maps to `QSystemTrayIcon::Trigger` and
right-click to the native context menu; no `showMenu()` call is made on
right-click to avoid duplicate menus on X11.

Observed in the real GNOME session before the screen locked: the exported
menu showed `Kaeru: OFF\nInactive`, then `Kaeru: ON\n… remaining` with a green
dot after a left-click activation; right-click opened the menu without
changing state; the exported menu had no `Turn off Kaeru` action and showed
`Custom…` below `Indefinitely`. A final full second-click OFF/ON round trip
could not be completed while the desktop was locked.

## 3. Main GUI and About (WP-09)

- No File menu. Navigation: Keep Awake, Settings, Diagnostics, About.
- Window title is exactly `Kaeru`; `Ctrl+1…4` reach all four pages and
  `Ctrl+,` still opens Settings. Quit is only in the tray.
- Default 1100×760, minimum 900×650. `Settings::windowGeometry()` is
  validated through `fitWindowToAvailable()` before `setGeometry()`, so a
  saved position on a now-missing monitor is clamped into the active screen.
  Geometry is saved on close and in `MainWindow::~MainWindow()`.
- `AboutPage` uses the unmodified official frog artwork from Qt resources,
  the installed version, license/acknowledgements and the official repo URL.
- `UpdateChecker` calls
  `https://api.github.com/repos/Emne-Eiji/kaeru/releases/latest`, compares
  semantic numeric version parts, and emits an honest result. It only links
  to the release page; it never downloads or installs packages and never
  uses credentials. 404, 403/429, timeouts, malformed JSON and missing
  fields produce an error or “no published release” message, never “latest”.

## 4. Themes (WP-09)

`Theme::apply()` now switches to Qt's Fusion style, sets a complete QPalette
and installs a stylesheet covering main windows, dialogs, message boxes,
sidebar/selected item, cards, buttons, inputs, spin boxes, combo popups,
tables/headers, scrollbars, menus, tooltips, status bar and disabled states.
`MainWindow` re-applies the theme when `Settings::changed` or
`ThemeWatcher::schemeChanged` fires, so Light/Dark/System take effect
immediately and survive restart.

## 5. Tests added

`test_awakeduration`, `test_settings`, `test_updates`, `test_theme`,
`test_windowgeometry`, and an extended `test_traystate`. Total suite at this
checkpoint: 20 executables, all passing in a clean Release build.

## 6. Known limitations

- DBusMenu cannot color individual words or set per-line font sizes in the
  GNOME panel; state color is conveyed by the status dot and the exact
  ON/OFF text.
- GNOME appindicator click routing is shell-side. On this GNOME 42 host a
  physical left-click on the icon is consumed by the shell and does not
  produce `QSystemTrayIcon::Trigger`/SNI `Activate`; the interactive status
  item and a direct SNI `Activate` call both toggle correctly. Right-click
  reliably opens the menu. This is documented rather than claimed as a
  passing left-click acceptance item.
- Update checking requires network access to api.github.com. No release has
  been published yet, so the live check is expected to report “no published
  release”, not “up to date”.
- Ubuntu 24.04, Wayland and real suspend/resume remain unverified.
