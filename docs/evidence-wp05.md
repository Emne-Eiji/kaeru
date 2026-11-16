# WP-05 — Work Log Evidence (storage, lifecycle, recovery, boundaries)

Environment: Pop!_OS 22.04, GNOME 42, X11, user `banana`. Binary: `kaeru 0.1.1`
(build tree). All runs sandboxed via `KAERU_TEST_HOME`.

## 1. Unit tests — `test_workmanager` (new, 10 tests) + full suite

`ctest -j9` → 11/11 suites pass. The work-manager suite covers (real SQLite in
a temp sandbox, simulated boundaries at the API):

| Test | Verified |
|---|---|
| startPauseResumeStop | start creates session+work segment; pause closes segment (session stays open); resume opens a new segment; stop closes both |
| startIsIdempotentResume | second `start` resumes the open session — never a second concurrent session |
| switchProjectIsTransactional | project switch closes the old segment exactly where the new one begins; source `project-switch` |
| suspendBoundaryClosesSegment | PrepareForSleep(start) closes the open segment; resume boundary stays paused; explicit resume starts a new segment |
| lockPausesOnlyWhenEnabled | `pauseWorkWhenLocked` off → no pause; on → paused; unlock does not auto-resume |
| checkpointsDoNotAffectTotals | open segments never count as work; immediate tick writes no checkpoint; checkpoints never appear in totals |
| recoveryIsConservative | crash simulation with a 2-min checkpoint → recovered exactly to the checkpoint (2 min, not the full 10); session closed; checkpoints deleted (never duplicate report entries) |
| recoveryWithoutCheckpointInventsNothing | no checkpoint → segment closed at its own start; zero work invented |
| focusWorkRecordedIntoFocusSessions | focus intervals → `focus_sessions` only (never work-log segments — no double counting); invalid values rejected |

## 2. Native lifecycle (CLI over D-Bus) — PASS

Script `/tmp/opencode/work_test.sh` (documented; not committed):

```
kaeru --background (sandbox kaeru-work-test)
kaeru work start --project "Native Test"   → "Work Log recording started"
kaeru status                               → "workLog":{"active":true,
                                              "startedAt":"2026-11-16T08:03:53Z",
                                              "todayWorkMs":0}
kaeru work pause                           → "Work Log recording paused"
kaeru work resume                          → "Work Log recording resumed"
kaeru work stop                            → "Work Log recording stopped and saved"
kaeru status                               → "workLog":{"active":false,"todayWorkMs":2000}
```

Database inspection (`sqlite3` on `$KAERU_TEST_HOME/kaeru/kaeru.db`):

```
projects:|1|Native Test
sessions:|1|closed|2026-11-16T08:03:53Z|2026-11-16T08:03:55Z
segments:|1|1|work|manual|2026-11-16T08:03:53Z|2026-11-16T08:03:54Z
segments:|2|1|work|manual|2026-11-16T08:03:54Z|2026-11-16T08:03:55Z
```

- Pause/resume produced exactly **2 segments**, non-overlapping (segment 1
  ends where segment 2 begins), both closed by stop.
- `todayWorkMs` = 2000 ms — the real recorded time between start and stop
  (honest, not rounded up).
- Session state `closed` with both segments closed.

## 3. Native crash recovery — PASS

Script `/tmp/opencode/recovery_test.sh`: start recording → `kill -9` →
restart.

```
status after recovery:
"workLog":{"active":false,
  "recoveryNotes":["Recovered interrupted session #1. No checkpoint was found,
   so only time up to the segment start (03:05) was recorded — no work time
   was invented."],
  "todayWorkMs":0}
database: sessions #1 closed (start == end); segment closed at start;
          recovery_checkpoints: 0 (deleted — never duplicate report entries)
```

The recovery is conservative and honest: no checkpoint existed (crash within
the first 60 s), so nothing after the segment start was invented.

## 4. Storage integration (Application)

- `Database` opened at `Paths::databaseFile()` with WAL + FK pragmas;
  `Migrations::apply` runs versioned migrations at startup (v1).
- Failure path: if the database cannot open, Work Log reports
  "storage unavailable" via Diagnostics capability `work-log-storage`
  (failed) and the page disables its controls — the rest of the app works.
- Checkpoints every 60 s while recording (`WorkSessionManager::tick` driven
  by the 1 Hz Application tick).

## 5. Boundaries wiring (Application)

- `LockMonitor::lockStateChanged` → `WorkSessionManager::onLockChanged`
  (pause when `pauseWorkWhenLocked` — default on, settings §worklog).
- `LogindPowerMonitor::prepareForSleep` → `onSuspendBoundary` (close open
  segment at suspend; stays paused after resume).
- `prepareQuit` (graceful) stops recording: segment + session closed cleanly
  (no recovery needed next start); inhibitors released.

## 6. Not yet run (honest gaps → WP-06)

- Reports page (daily/weekly totals + filters) and CSV export wiring —
  `ReportsPage` is still a placeholder; `CommandRouter::exportCsv` still
  errors honestly ("not available yet").
- Focus→Work Log linkage native run: option `focusRecordInWorkLog` is wired
  to `focus_sessions` inserts (unit-tested); a native focus-run check is
  scheduled with WP-06 reports.
- Idle-gap thresholds, revisions provenance UI — spec-optional for 1.0;
  tables exist (`session_revisions`), UI deferred.
- Ubuntu 24.04 validation — declared environment gap.