# ADR-007 — UI toolkit and theming: Qt Widgets + QSS, GNOME color-scheme aware

**Status:** APPROVED (implementing under user authorization of 2026-11-15)
**Date:** 2026-11-15

## Context

Spec's default stack is Qt 6 Widgets. GNOME 42 is the host; light/dark themes required; restrained, native-feeling design; accessibility required (contrast, focus order, screen-reader labels, keyboard navigation).

## Options

1. Qt Widgets + QSS theme — default stack; mature; accessible widgets; works with X11/Wayland. **Chosen.**
2. Qt Quick/QML — more expressive animation, but heavier dependency surface (qml modules) and less idiomatic for forms/tables; rejected for 1.0.
3. GTK — foreign toolkit; rejected by stack policy.

## Decision

- Qt Widgets, fixed navigation shell: left sidebar (6 pages), central stacked content, persistent status header.
- Two palettes (light/dark) from one QSS + QPalette pair; default follows `org.freedesktop.appearance` color-scheme via xdg-desktop-portal Settings interface (native), manual override in Settings.
- Type scale: 13 px base, 11 px captions, 28–40 px timer numerals; spacing on a 4/8/12/16/24 px scale; semantic status colors with text labels + icons (never color-only); all interactive elements reachable by Tab in reading order; `QAccessible` names on every control; keyboard shortcuts for primary actions.
- 24 px logical icon grid; HiDPI-safe SVG assets; all strings translatable (tr()-wrapped).

## Trade-offs

QSS is less powerful than native theming, but guarantees visual consistency across light/dark and does not fight GNOME theme drift.
