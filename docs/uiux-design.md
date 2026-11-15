# Kaeru — UI/UX Design Specification (WP-01)

**Status:** Recommended design, approved for implementation under the user authorization of 2026-11-15.

## 1. Product character

A calm, restrained Linux desktop utility. "Kaeru" means *frog* in Japanese — the identity motif is a simple geometric frog face used sparingly (app icon, empty states). The interface prioritizes: current state → primary action → everything else. No dashboards, no decorative charts, no gamification.

## 2. Navigation and window

- Main window: 860×620 default (min 720×520, resizable, maximizable). Left sidebar navigation, 6 pages:

| Page | Purpose |
|---|---|
| Overview | Current state + primary actions |
| Focus | Timer + routine controls |
| Work Log | Active session + history |
| Reports | Totals, filters, CSV export |
| Settings | Module switches, preferences |
| Diagnostics | Capability matrix, backend state, errors |

- Sidebar: icon + label, active page highlighted with a subtle accent bar (not color-only — label weight + icon also change). Keyboard: `Ctrl+1..6` switch pages; all pages reachable by Tab order.
- Persistent status header (Overview top): Keep Awake state and Work Log state side by side, visually distinct (separate cards, different icons: sun for Keep Awake, pen/segment for Work Log).

## 3. Overview page (hierarchy)

1. **Keep Awake card** — big state word (Off / Active / Partially active / …), reason, backend, remaining time (mm:ss, large numerals), the chosen mode ("System sleep", "Display", "Both"). Primary action: single prominent Start/Stop toggle button; duration selector (chips: 15m 30m 1h 2h Custom… Until… Indefinite*) as a segmented row above the button. Indefinite shows a warning label. "Turn off now" is the explicit suppression control when active.
2. **Work Log card** — state (Not recording / Recording / Paused), current project, elapsed, Start/Pause/Stop inline.
3. **Focus card** — compact: current interval, phase, time remaining, Start/Pause/Stop/Skip; full controls on the Focus page.
4. **Attention strip** — non-blocking inline area for errors/partial states/recovery prompts with a recovery action (e.g., "Review interrupted session").
   Empty/error/partial states are rendered inline in each card ("Idle inhibition unavailable on this desktop" + Open Diagnostics link).

## 4. Focus page

- Large timer numerals (40 px) with phase label and interval count ("Work 2 of 4"). Phase-colored progress ring is decorative only (state always labeled).
- Routine selector: Custom countdown / Pomodoro. Pomodoro controls: Start, Pause, Resume, Stop, Skip interval, Extend +5 min, change upcoming interval (spin box, applies to next interval).
- Options row (persisted): Keep awake during focus ☐ (off), Keep awake during breaks ☐ (off), Record focus time in Work Log ☐ (off), Associate project ☐ (off; project picker when on), Auto-start next interval ☐ (off), Notifications ☑, Sound ☐, Quiet hours (from/to, default off).

## 5. Work Log page

- Active session panel: project combo (+ new project), task field, tags field, note field, Start / Pause / Resume / Stop / Switch project (switch closes current segment transactionally and continues into the new project).
- History: session table (date, project, task, recorded, source badge manual/focus, billable flag), daily/weekly totals header. Row context menu: Edit (task/tags/note/billable/project), Delete (confirm), Correct times (segment editor: adjust start/end with provenance "user correction" revision recorded).
- Honest labels everywhere: "Recorded work time", "Focus-session time", "Uncertain gap (needs review)", "Idle/unclassified" — awake/login time is never called work.
- Empty state: frog illustration + "No sessions yet — start recording when you begin working."

## 6. Reports page

- Range presets (Today / This week / Last 7 days / Custom), project + tag filters, category breakdown table: Recorded work, Focus-session, Manual entries, Uncertain, Idle detected (0.0 unless idle detection enabled).
- Daily/weekly totals table + total row; CSV export button (filename `kaeru-worklog-YYYYMMDD-YYYYMMDD.csv`), export summary dialog with row count and destination; delete-data workflow in Settings (explicit double confirmation, states exactly what is deleted, local-only).

## 7. Settings page

Grouped: Modules (Keep Awake, Focus, Work Log — independent switches), Startup ("Start after login" ☐ off; "Keep awake on login" ☐ off — separate), Notifications (enable, sound, quiet hours), Appearance (theme: System/Light/Dark), Data (storage path display, backup folder, "Delete all Kaeru data…" guarded action), About (version, license).

## 8. Diagnostics page

- Capability table (Capability / Status / Backend / Detail): System-sleep inhibition, Session-idle inhibition, Screen-lock detection, Suspend events, Tray, Notifications, Idle detection, Autostart, Battery (deferred).
- Statuses: Implemented+tested / Available / Unavailable / Permission denied / Error — with plain-language detail and recovery hints. Backend state log (last 50 events: acquire/release/loss/restart), no private data (notes/tags never logged).

## 9. States system (all pages)

`empty · loading · ready · active · paused · unsupported · permission-denied · partial · failed · recovering` — each has: icon, text label, action. Status never relies on color alone (icon + word + shape: dot/check/!/×).

## 10. Tray

Menu per spec §8.4: status lines (Keep Awake: state/Reason/Remaining), Keep Awake submenu (15m/30m/1h/2h/Custom…/Until…/Indefinitely), Turn off now, Focus submenu, Work Log submenu (+Today total), Pause All, Open Kaeru, Settings, Diagnostics, Quit. Inapplicable entries disabled (not hidden). Left-click opens main window. No-tray fallback: compact 260 px window with the same actions.

## 11. Typography, spacing, color (light/dark)

- Font: system default (GNOME: Cantarell/Adwaita Sans); monospace numerals for timers (`font-feature-settings: tnum`).
- Spacing scale 4/8/12/16/24; cards separated by 12 px, internal padding 16 px, corner radius 8 px.
- Light: bg #FAFAFA, card #FFFFFF, text #24292F, accent #2E7D32 (green = awake/positive), focus blue #1A73C8, work red #C0392B, warn #B7791F. Dark: bg #1E1F22, card #26272B, text #E6E6E6, accents lifted for ≥4.5:1 contrast. All text/interaction contrast ≥ WCAG AA (4.5:1; 3:1 for large numerals).
- Semantic colors always paired with icon+word.

## 12. Accessibility checklist

- Keyboard: full tab order, primary shortcuts (Ctrl+N start/stop awake, Ctrl+F focus page, Ctrl+W work page, Space primary action on page), Escape closes dialogs.
- Screen readers: accessibleName/Description on every control; state changes announced (Qt accessibility events).
- Focus visibility: 2 px accent outline on keyboard focus, never removed.
- High-contrast themes inherit system palette; SVG icons use currentColor where possible.
- Translatable strings via tr(); no hard-coded user-facing text.

## 13. Original assets

Hand-authored SVG frog icon set (base + 6 state variants) + 5 sidebar glyphs + 3 status glyphs, all original work by this project, licensed GPL-3.0-or-later with the project. No third-party assets.
