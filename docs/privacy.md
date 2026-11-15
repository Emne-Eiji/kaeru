# Kaeru — Privacy and Data Management

**Principle:** local-first; awake is not work; nothing leaves the machine.

## What Kaeru stores

| Data | Location | Purpose |
|---|---|---|
| Settings (module switches, preferences) | `$XDG_CONFIG_HOME/kaeru/kaeru.conf` | app behavior |
| Work Log (projects, sessions, segments, revisions), focus history, recovery checkpoints | `$XDG_DATA_HOME/kaeru/kaeru.db` (+ WAL) | user-facing history/reports |
| Pre-migration backups | `$XDG_DATA_HOME/kaeru/backups/` | data safety |
| Autostart entry (only if user enables) | `~/.config/autostart/io.github.kaeru.Kaeru.desktop` | startup |

## What Kaeru never does

- No network access in normal operation (no telemetry, no accounts, no cloud).
- No screenshots, no keystroke contents, no clipboard capture, no URL/window-title recording.
- No mouse/keyboard simulation.
- No reading of files unrelated to Kaeru's own data.
- No employer dashboards, exports on behalf of others, or productivity scoring.
- No logging of task names/tags/notes (private content stays out of logs and diagnostics).

## Honest semantics

- Keep Awake active means only "this app holds a sleep/idle inhibitor".
- Work Log records only what the user starts/pauses/stops or explicitly enables focus-recording.
- Suspend time is excluded from recorded work by default; uncertain gaps are marked and require user review.
- Labels distinguish recorded work / focus-session / manual / uncertain time. Computer-awake or logged-in time is never labeled work.

## Data controls

- Reports → CSV export: local file, user-chosen path.
- Settings → "Delete all Kaeru data…": explicit double confirmation; deletes only Kaeru's own config/data/cache dirs; app restarts to safe state.
- Uninstall (package removal): preserves user data by policy; the user can delete data via the in-app action or manually (`rm -rf ~/.config/kaeru ~/.local/share/kaeru` after uninstall — documented in README).
- Database file permissions: user-only directory (0700); SQLite not encrypted (documented limitation).
- IPC is session-bus scoped (same user's session); commands expose state and actions, not stored private content.

## Crash of privacy guarantees

Diagnostics never include private content; the backend event log contains capability/backend state only.
