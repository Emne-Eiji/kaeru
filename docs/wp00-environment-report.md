# WP-00 — Environment Investigation Report

**Work order:** KAE-WP-00 · **Status:** COMPLETE (AGENT-VERIFIED) · **Date:** 2026-11-15
**Read-only confirmation:** no files created/modified, no packages installed/removed/updated, no configuration or settings changed, no repository initialized during WP-00. All facts below come from executed read-only commands on the primary machine.

## 1. Machine and session facts (verified)

| Fact | Value | Evidence |
|---|---|---|
| OS | Pop!_OS 22.04 LTS (`ID=pop`, `ID_LIKE="ubuntu debian"`, `VERSION_CODENAME=jammy`) | `/etc/os-release`, `lsb_release -a` |
| Kernel | `7.0.9-76070009-generic` (System76 kernel), `x86_64` | `uname -a` |
| Package architecture | `amd64` native, `i386` foreign | `dpkg --print-architecture(s)` |
| Desktop | GNOME 42.9 (`gnome-shell 42.9-0ubuntu2.3pop0`), Pop shell extensions, `XDG_CURRENT_DESKTOP=pop:GNOME` | env vars, `dpkg -l` |
| Session type | **X11** (`XDG_SESSION_TYPE=x11`, `DISPLAY=:1`); Wayland not in use this session | env vars |
| Session bus | `unix:path=/run/user/1000/bus` (UID 1000) | env vars |
| CPU / RAM / disk | 12 cores, 15 GiB RAM, 59 GiB free on `/home` filesystem (root fs 87% used) | `nproc`, `free -h`, `df -h` |
| Power hardware | Laptop: `BAT0` + `AC0` present; `system76-power` daemon active | `/sys/class/power_supply`, `systemctl is-active` |
| Locale / timezone | `en_US.UTF-8`, **America/Toronto (EST, UTC−05)**, NTP synchronized, RTC in UTC | `timedatectl` |
| Hostname | `pop-os` | `uname -n` |

## 2. Toolchain (verified)

| Component | Version | State |
|---|---|---|
| gcc / g++ | 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04.3) | installed |
| CMake | 3.22.1 | installed |
| make / ninja | 4.3 / not installed | make installed |
| git | 2.34.1 | installed |
| pkg-config | 0.29.2 (Qt6 uses CMake configs, not pkg-config) | installed |
| Qt 6 | **6.2.4** — `qt6-base-dev`, `qt6-base-dev-tools`, `libqt6{core,gui,widgets,dbus,sql,sql-sqlite,concurrent,test}6` all installed; CMake config dirs present under `/usr/lib/x86_64-linux-gnu/cmake/Qt6*` | installed |
| SQLite | CLI 3.37.2 + Qt SQLite driver | installed |
| Python | 3.10.12 | installed |

**Qt 6.2.4 tray capability (binary-verified):** `strings libQt6Gui.so.6.2.4` contains `QStatusNotifierItemAdaptor`, `org.kde.StatusNotifierItem`, `org.kde.StatusNotifierWatcher` → Qt 6.2's QPA implements StatusNotifierItem on Linux. `libQt6Widgets` contains no XEmbed references on this build → SNI is the tray path.
**Constraint:** Qt 6.2 is the floor; code must not use APIs newer than Qt 6.2 (Ubuntu 24.04's Qt 6.4 is a superset).

## 3. Native desktop integration surface (verified present, not yet runtime-tested)

| Capability | Status | Evidence |
|---|---|---|
| systemd-logind | systemd 249 active; `org.freedesktop.login1.Manager.Inhibit(what,who,why,mode)→fd`, `ListInhibitors`, `PrepareForSleep(b)`, `CanSuspend`, `BlockInhibited` available | `busctl`, systemd 262 docs (API stable) |
| Existing inhibitors | GNOME Shell / gsd-power / gnome-session already hold `sleep` delay inhibitors | `systemd-inhibit --list` |
| XDG portal | xdg-desktop-portal 1.14.4 + `xdg-desktop-portal-gnome` + `-gtk` services active; `org.freedesktop.portal.Inhibit` interface **version 3** exposing `Inhibit(window, flags, options)→handle` (flags: 1=Logout, 2=User Switch, **4=Suspend, 8=Idle**), release via `Request.Close()` | `busctl --user introspect`, xdg-desktop-portal docs |
| Session idle policy | `org.gnome.desktop.session idle-delay=300` (blank after 5 min); `sleep-inactive-ac-type='suspend'`, `sleep-inactive-ac-timeout=1800` (auto-suspend after 30 min idle on AC) | `gsettings get` |
| Screen lock signals | `org.gnome.ScreenSaver` (`GetActive`, `ActiveChanged`) present; logind `LockedHint` path also available | `busctl introspect` |
| Tray | `gnome-shell-extension-appindicator` **enabled**; SNI watcher hosted by gnome-shell | `gnome-extensions list --enabled` |
| Notifications daemon | `org.freedesktop.Notifications` owned on session bus (PID 4807) | `busctl status` |
| Idle detection (Mutter) | `org.gnome.Mutter.IdleMonitor` present (ObjectManager at `/org/gnome/Mutter/IdleMonitor`) | `busctl introspect` |

## 4. Packaging tooling

- Present: `dpkg-dev 1.21.1`, `build-essential 12.9`, `fakeroot 1.28`, `dpkg-buildpackage`.
- **Missing (needed in WP-03):** `debhelper` (candidate 13.6ubuntu1), `lintian` (2.114.0), `devscripts` (2.22.1). Will be installed via named `sudo apt install` only, per authorization.

## 5. Test infrastructure on this machine

- `xdotool` (X11 automation) — installed; `ffmpeg` (X11 screen capture via `x11grab`) — installed → screenshot evidence possible.
- `scrot`, `gnome-screenshot`, `import`, `notify-send` — not installed (not needed; ffmpeg + DBus notifications suffice).
- GUI testing can run against the live X11 session (`DISPLAY=:1`).

## 6. Candidate desktop/session test matrix

| Target | Status |
|---|---|
| Pop!_OS 22.04 + GNOME 42 + X11 (this machine) | **Primary validation environment** |
| Ubuntu 24.04 LTS `amd64` | Package target; Qt 6.4; portal/Qt APIs used are ≥6.2-compatible. Full integration validation **not performed** (no such machine available) — honest gap, listed in capability matrix |
| Wayland session | Untested (session runs X11) — marked not evaluated |
| KDE Plasma / Debian 13 / arm64 | Not evaluated — candidate targets only, per spec §5.2 |

## 7. Risks, assumptions, gaps

1. **OS divergence:** spec's primary target is Ubuntu 24.04; development machine is Pop!_OS 22.04. Mitigation: target the common Qt 6.2+/systemd 249+ subset; build a clean `debhelper` package for `jammy` and `noble` layout (package depends on runtime libs, not distro specifics); document the gap honestly.
2. **Real suspend/resume test** would suspend the user's active desktop — not auto-triggered. Manual procedure provided in `docs/testing.md`.
3. DST wall-clock boundary is in the future; TZ/DST behavior is validated by timezone injection in tests, not by changing the machine clock.
4. GNOME 42 portal `Inhibit` support for `idle` flag is expected (xdg-desktop-portal-gnome 42) but the effective screen-blank behavior must be confirmed live in WP-02.
5. `CanSuspend` property (logind) flips to `inhibited`/`inhibitor-blocked` while a `sleep:block` inhibitor is held — planned as direct, non-fake inhibition evidence.
6. Portals in non-sandboxed (plain desktop) apps are supported by spec; `window` identifier may be empty for tray-only apps — to be confirmed live.

## 8. Read-only statement

No files were created or modified, no packages installed/removed, no settings or services changed, and no repository was created during WP-00. The repository `kaeru/` was initialized only after WP-00 was complete, as permitted for WP-01.
