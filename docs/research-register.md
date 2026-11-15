# Kaeru — Research and Source Register

Rules followed: primary sources preferred; external content treated as untrusted input; no wholesale code/asset copying; licenses verified; nothing incorporated without registration.

| # | Source | Type / version | Date accessed | What was learned | Incorporated into Kaeru? |
|---|---|---|---|---|---|
| 1 | XDG Desktop Portal — `org.freedesktop.portal.Inhibit` (v3) https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Inhibit.html | Specification (docs site) | 2026-11-15 | Inhibit(window, flags, options)→handle; flags 1/2/4/8 (Logout/Switch/Suspend/Idle); release via Request.Close(); Response signal semantics; handle_token; CreateMonitor/StateChanged; QueryEndResponse | Yes — PortalInhibitBackend implements exactly this API (no code copied) |
| 2 | systemd `org.freedesktop.login1` D-Bus API https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html | Official API docs (systemd 262; API stable vs 249) | 2026-11-15 | Inhibit(what,who,why,mode)→fd; what values incl. sleep/idle; block/delay modes; fd released on last close; PrepareForSleep(true/false); CanSuspend values incl. inhibited/inhibitor-blocked; ListInhibitors | Yes — LogindInhibitBackend + PowerMonitor (own implementation) |
| 3 | Qt `QSystemTrayIcon` docs https://doc.qt.io/qt-6/qsystemtrayicon.html | Official docs (Qt 6.12 rendering; notes apply back) | 2026-11-15 | Linux SNI support incl. GNOME; isSystemTrayAvailable; late-tray-arrival auto-add; GNOME activation caveats since 3.26 | Yes — TrayController design (Qt-native SNI path + fallback) |
| 4 | Qt "What's New in Qt 6.5" https://doc.qt.io/qt-6/whatsnew65.html | Official docs | 2026-11-15 | Confirmed no tray/SNI changes in 6.5; supported API surface check for version floor | Design constraint only |
| 5 | Local Qt 6.2.4 binary inspection (`strings libQt6Gui.so.6.2.4`) | Primary evidence | 2026-11-15 | QStatusNotifierItemAdaptor + org.kde.StatusNotifierItem/Watcher present in Qt 6.2 GUI → SNI tray works on this machine | Confirms ADR-006 |
| 6 | Freedesktop StatusNotifierItem spec (referenced by Qt docs) https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem/ | Specification | 2026-11-15 | SNI semantics (menu, tooltip, activation) — consumed via Qt's implementation, not reimplemented | No direct use (Qt implements) |
| 7 | Local environment commands (WP-00) | Executed evidence | 2026-11-15 | systemd 249, portal v3 services, GNOME 42 appindicator enabled, gsettings idle/suspend policy, logind/gnome signals | Foundation for all ADRs |
| 8 | Debian Policy / debhelper usage knowledge (packaging) | Packaging docs knowledge | 2026-11-15 | control/rules/changelog/copyright/install structure; dh_shlibdeps dependency resolution; lintian checks | debian/ packaging (original files) |

**License notes:** Kaeru's own code and assets are original, licensed GPL-3.0-or-later. No third-party code or assets are incorporated. Runtime dependencies (Qt 6 LGPL-3/GPL-3 exceptions, SQLite public domain, systemd LGPL-2.1+) are system packages; their licenses are listed in `docs/license-inventory.md`.
