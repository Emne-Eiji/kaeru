# ADR-003 — Clock model: boot-time countdowns, monotonic stopwatches, TZ-aware deadlines

**Status:** APPROVED (implementing under user authorization of 2026-11-15)
**Date:** 2026-11-15

## Context

Spec §6.3/§6.6: elapsed countdowns must continue through suspend and complete exactly once; focus/work time must exclude suspend by default; wall-clock deadlines must be calendar-based with timezone metadata and recalculation on clock change. Wall-clock subtraction alone is forbidden for durations. Qt 6.2 offers `QElapsedTimer` (monotonic; stops during suspend) and `QDateTime` but no boot-clock.

## Options

1. Wall-clock everywhere — breaks on clock changes; forbidden by spec for durations.
2. Monotonic everywhere — countdowns freeze across suspend (wrong semantics).
3. **Three clock kinds, injected** — chosen: `MonotonicClock` (CLOCK_MONOTONIC), `BootClock` (CLOCK_BOOTTIME, Linux; includes suspend), `WallClock` (QDateTime UTC + QTimeZone for display/deadlines). Tests inject `FakeClock`.

## Decision

- Keep-awake countdown timers and Pomodoro interval progression: **BootClock** (suspend-inclusive; auto-completes once after resume).
- Focus/work stopwatch accrual and Work Log segment durations: **MonotonicClock** (suspend-exclusive); on `PrepareForSleep(true)` accrual pauses; on resume stays paused per spec.
- Wall-clock deadlines ("until 5:00 PM"): stored as UTC instant + timezone ID; re-derived on timer tick; clock-change detection via low-frequency (60 s) wall-vs-boot drift check; recompute rather than accumulate.
- All persisted timestamps: UTC ISO-8601 + timezone ID stored for display; reports split at local midnight of the session timezone.
- Portability: `CLOCK_BOOTTIME` is Linux-specific (per spec's Linux-native scope); guarded by `#ifdef CLOCK_BOOTTIME` with monotonic fallback and honest capability reporting.

## Trade-offs

CLOCK_BOOTTIME needs a tiny platform adapter beyond Qt — acceptable (Linux-native app). DST behavior: boot/monotonic are immune; wall deadlines recompute from the stored timezone, so DST shifts resolve to the correct next occurrence; tested via timezone injection (no machine-clock changes).
