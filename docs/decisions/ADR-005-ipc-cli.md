# ADR-005 — IPC and CLI: one binary, D-Bus session service, shared CommandRouter

**Status:** APPROVED (implementing under user authorization of 2026-11-15)
**Date:** 2026-11-15

## Context

Spec §12: UI and CLI must use the same core command handlers; session-scoped IPC with access restrictions; meaningful exit codes; second launch must contact/activate the existing instance; machine-readable status with real backend state.

## Options

1. Separate CLI binary calling a daemon over DBus — two binaries, daemon-like semantics; spec forbids a pre-login daemon but a user-session service would still be heavier than needed. Rejected.
2. Single binary; GUI mode owns DBus service name `io.github.kaeru.Kaeru`; CLI mode is the same binary run with a recognized command, talking DBus to the running instance. **Chosen.**
3. No IPC; CLI manipulates files directly — violates "UI and CLI use same handlers" and unsafe with a running instance. Rejected.

## Decision

- Single executable `kaeru`. `argv[1]` recognized as CLI command → `QCoreApplication` + DBus client call; otherwise → GUI mode.
- Service: `io.github.kaeru.Kaeru`, interface `io.github.kaeru.Kaeru.Control1`, path `/io/github/kaeru/Kaeru`. Ownership on the session bus = single-instance lock; a plain second launch activates the running instance's window and exits 0.
- Methods: `Status()` (JSON incl. backend/capability state), `AwakeOn(durationSec, mode)`, `AwakeOff()`, `FocusStart/Pause/Resume/Skip/Stop`, `WorkStart(project)/Pause/Resume/Stop`, `PauseAll()`, `ExportCsv(path, from, to)`, `Diagnostics()` (JSON), `ShowWindow()`, `Quit()`.
- Exit codes: 0 ok · 2 usage · 3 not running · 4 failed · 5 denied/unsupported.
- Access: session-bus ownership limits to same user/session; no other secrets involved.

## Trade-offs

CLI requires a running GUI instance (per spec: "Never require a second application instance to control the first" is satisfied; the reverse — CLI without instance — returns exit 3 with a clear message; `--background` starts a hidden-instance GUI). This keeps one process model, no headless daemon.
