# ADR-004 — Storage: SQLite via Qt SQL, versioned migrations, WAL, checkpoints

**Status:** APPROVED (implementing under user authorization of 2026-11-15)
**Date:** 2026-11-15

## Context

Spec requires local-first persistence with integrity constraints, versioned migrations, backup before risky migrations, and crash recovery that never invents work. Stack default: SQLite. Available: Qt SQL with SQLite driver (Qt 6.2).

## Options

1. SQLite via Qt SQL (`QSqlDatabase`) — in default stack, zero new dependencies. **Chosen.**
2. sqlite3 C API directly — more control, more code, same result. Rejected (no benefit at this scale).
3. Flat files/JSON — no transactional integrity/queries. Rejected.

## Decision

- File: `$XDG_DATA_HOME/kaeru/kaeru.db`; per-connection `PRAGMA foreign_keys=ON`, `journal_mode=WAL`, `synchronous=FULL`.
- Schema versioned via `schema_migrations`; migrations applied in a transaction with a pre-migration file backup (`backups/kaeru-pre-vN.db`) when version increases.
- Tables v1: `projects`, `sessions`, `session_segments`, `session_revisions`, `focus_sessions`, `recovery_checkpoints`. `schedules`/`presets` deferred until approved.
- Constraints: partial unique index enforcing ≤1 active session; `CHECK` non-negative durations and ordered segment times; FKs with `ON DELETE CASCADE`; transactional project switch (close old segment + open new in one transaction).
- Recovery checkpoints every 60 s while a segment is open, storing monotonic/boot anchors so recovered durations are provable; gaps after last checkpoint are marked `uncertain` for user review.
- Parameterized SQL only; no task notes in logs; DB file not world-readable (user-only dir).

## Trade-offs

Qt SQL is a thin wrapper; if finer control is needed later (e.g., custom VFS), the storage layer is isolated behind repositories so a backend swap stays local.
