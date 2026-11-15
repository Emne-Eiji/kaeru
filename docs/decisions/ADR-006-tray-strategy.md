# ADR-006 — Tray strategy: SNI via QSystemTrayIcon, fallback visible window

**Status:** APPROVED (implementing under user authorization of 2026-11-15)
**Date:** 2026-11-15

## Context

GNOME 42 (this environment) only hosts tray items via StatusNotifierItem/AppIndicator (`gnome-shell-extension-appindicator` enabled). Qt 6.2.4's QtGui contains `QStatusNotifierItemAdaptor` + `org.kde.StatusNotifierWatcher` strings (binary-verified in WP-00) → QSystemTrayIcon on Qt 6.2 Linux publishes an SNI item. Some environments (older WM trays) may only support XEmbed; some have no tray at all.

## Options

1. Own SNI publisher implemented with QtDBus — max control, works everywhere SNI exists, but duplicates Qt functionality.
2. **QSystemTrayIcon (Qt's SNI path) + fallback window** — chosen. Simpler, uses Qt's maintained implementation, verified present on this machine; fallback guarantees visible control.
3. libappindicator/Ayatana — GTK dependency in a Qt app; deprecated upstream. Rejected.

## Decision

- Use `QSystemTrayIcon` with a context menu built by `TrayController` from the same `CommandRouter` actions as the UI (spec §8.4 menu content).
- Detect availability: `QSystemTrayIcon::isSystemTrayAvailable()` with bounded retry/backoff (5 retries, 1 s apart) before declaring "no tray".
- Fallback: a compact always-available control window ("Kaeru controls") with the core actions; main window close only hides when tray is usable, otherwise the fallback stays visible.
- Icon states: inactive / awake / focus / recording / paused / error — distinct icon + tooltip text (not color-only).

## Trade-offs

If a Qt regression broke SNI, the fallback keeps the product usable; an own-SNI publisher remains a future option (documented) without architectural change since `TrayController` is an adapter.
