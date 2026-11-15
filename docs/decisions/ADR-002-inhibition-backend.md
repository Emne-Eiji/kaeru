# ADR-002 — Inhibition backend strategy: portal-first with logind fallback

**Status:** APPROVED (implementing under user authorization of 2026-11-15)
**Date:** 2026-11-15

## Context

Two native inhibition mechanisms exist on the primary environment (WP-00):

- **XDG Desktop Portal `Inhibit` v3** (`org.freedesktop.portal.Inhibit`): async, returns a Request handle; release via `Request.Close()`; flags 4=Suspend (system sleep), 8=Idle (session idle/screen blank); requires response-signal handling; integrates with the session's power policy (on GNOME routes through gnome-session → logind).
- **systemd-logind `Inhibit`** (`org.freedesktop.login1.Manager.Inhibit`): returns an inhibitor FD (`sleep`, `idle`, etc., mode `block`/`delay`); release by closing the FD; `sleep:block` prevents automatic suspend; `idle:block` affects only logind's own idle logic, **not** screen blanking; observable via `systemd-inhibit --list` and `CanSuspend`.

Capabilities must be distinct (spec §6.1): system sleep vs session/display idle vs screen locking.

## Options

1. Portal-only — no fallback when portal missing/broken. Rejected (robustness).
2. logind-only — cannot honestly provide display-idle inhibition on GNOME (idle inhibitor ≠ blank prevention). Rejected for the display capability.
3. **Portal-first, per-capability selection, logind fallback** — chosen.
4. Third-party wrappers (e.g., linking libappindicator-style GTK libs) — new dependency, rejected.

## Decision

- Capability `SystemSleep`: try portal flag 4; if portal unavailable/denied/failed, use logind `sleep:block` FD.
- Capability `SessionIdle` (display awake): portal flag 8 (primary); if portal cannot provide it, the capability is honestly reported `Unavailable` (logind cannot provide display-awake; do not fake it). Never claim it via logind.
- Screen locking is never inhibited. Shutdown/logout/user-switch/lid are never inhibited.
- Each request binding holds exactly one handle; lifecycle ownership per backend (portal: keep request object, close on release; logind: keep FD, close on release, `FD_CLOEXEC` set to prevent child inheritance).
- Backend loss (D-Bus name owner change, prepare-for-sleep, denial, failure) demotes bindings and triggers re-resolution; requested vs effective state always reported separately.
- Testing evidence: logind path via `systemd-inhibit --list` + `CanSuspend` property change; portal path via Response signal + observed inhibitor set delta of the session.

## Trade-offs

Portal integrates with session semantics and Wayland-friendly flows; logind FD is simple and directly observable. Supporting both adds code, but either path is independently testable and the honest-status requirement is easier to meet when the backend is explicit.
