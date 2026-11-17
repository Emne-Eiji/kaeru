# Kaeru

**Kaeru** (蛙 — "frog") is a lightweight, Linux-native desktop utility that
keeps your computer awake on request.

- Ask your desktop to prevent automatic **system sleep** and **screen
  blanking** for 15 minutes, 30 minutes, 1 hour, 2 hours, or indefinitely.
- Choosing a new duration **replaces** the current request; durations never
  stack. One clear Keep Awake session at a time.
- Honest state: Kaeru distinguishes *requested* from *confirmed active* and
  shows the backend and any limitation.
- Tray controls and an opt-in autostart entry, with a visible fallback
  window when no system tray is available.
- Local-first: no accounts, no cloud, no telemetry, no synthesized input
  activity, and no changes to global power settings.

The primary interface is the GUI: `kaeru` opens the **Keep Awake** page,
**Settings** and **Diagnostics**. Ordinary use needs no terminal.

## Release scope (0.2.0)

This release focuses on a dependable Keep Awake feature. The **Focus Timer**,
**Work Log** and **Reports** are **deferred**: they are not shown in the GUI
or offered by the CLI. Their source, tests and any existing stored data are
preserved for a future release. The local database is not deleted or
rewritten by this version.

## Install (Debian package)

Build or obtain `kaeru_0.2.0-3_amd64.deb` (the 0.2.0-2 checkpoint added the
live tray status; 0.2.0-3 adds WP-08/09 tray interaction, custom durations,
the About page and complete theming):

```bash
sudo apt install ./kaeru_0.2.0-3_amd64.deb
```

The debug-symbols `.ddeb` is optional and only useful for debugging; normal
users should install the main `.deb`. See `dist/README.md` for the release
layout and checksums.

Project repository: https://github.com/Emne-Eiji/kaeru

Runtime: a Qt 6 desktop session with `xdg-desktop-portal` is recommended;
Kaeru falls back to a logind `sleep:block` inhibitor for system sleep when
the portal is unavailable. Display-awake inhibition requires the portal.

## Build from source

```bash
sudo apt install build-essential cmake qt6-base-dev debhelper
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

The unit tests run in an isolated data home and never touch real user data.

## Run

```bash
./build/kaeru              # open the window
./build/kaeru --background # start hidden in the tray
kaeru status               # human-readable Keep Awake state
kaeru status --json        # machine-readable state
kaeru awake on --duration 30m
kaeru awake indefinite
kaeru awake off
kaeru diagnostics
kaeru quit
```

Durations accept forms such as `90s`, `30m`, `1h30m`.

## Autostart

Settings → Startup offers separate opt-in switches:

- **Start Kaeru after login** writes a per-user XDG autostart entry:
  `Exec=kaeru --background --awake-on-login` when the second switch is also
  enabled.
- **Start Keep Awake when Kaeru starts** enables an indefinite request at
  startup.

Disabling the first switch removes only Kaeru's own autostart entry.

## Honest status and limitations

- A submitted request is not reported as active until the desktop confirms
  it. If a backend cannot serve a capability, Kaeru says so.
- Keep Awake prevents automatic sleep; logging out, shutting down, closing
  a laptop lid, or a low battery can still end the session.
- Sound notifications depend on the desktop notification daemon and the
  configured sound theme.
- Validated on Pop!_OS 22.04 (Ubuntu 22.04 base) with GNOME 42 / X11.
  Ubuntu 24.04, Wayland and real suspend/resume were not re-tested for this
  release; see `docs/capability-matrix.md` and the delivery report.

## Documentation

- `docs/DELIVERY_REPORT_0.2.0.md` — what was fixed, verified and deferred
- `docs/handoff/KAERU_OPEN_ISSUES.md` — issue-by-issue resolution record
- `docs/architecture.md` — system design
- `docs/uiux-design.md` — UI/UX system
- `docs/capability-matrix.md` — what is tested vs not
- `docs/privacy.md` — data and privacy
- `docs/testing.md` — test strategy and GUI acceptance harness
- `docs/decisions/` — architecture decision records

License: GPL-3.0-or-later (see `LICENSE`).
