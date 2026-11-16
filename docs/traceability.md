# Kaeru — Requirement Traceability Matrix (0.2.0)

Status vocabulary: `implemented`, `tested` (with evidence), `partial`,
`deferred`, `unavailable`, `not evaluated`.

**0.2.0 scope:** Keep Awake is the only user-facing module. Focus, Work Log
and Reports are deferred from the GUI/CLI; their source and tests remain for
a future release. A "tested" row for deferred code means the preserved unit
tests pass, not that the feature is available.

| Req ID | Requirement (spec §) | Design artifact | Implementation | Tests / evidence | Status |
|---|---|---|---|---|---|
| KAE-PROD-001 | Lightweight native utility, independent modules (§2) | architecture.md | Application module switches | 0.2.0 forces KeepAwake on, Focus/WorkLog off; UI/tray expose Keep Awake only | implemented (single-module release) |
| KAE-SAFE-001 | No fake input/presence falsification/monitoring (§3) | ADR-002, privacy.md | nothing implements input synthesis; code audit | code audit; no synthetic input on any Keep Awake path | enforced by design |
| KAE-AWAKE-001 | Native inhibition with verified lifecycle (§6.1) | ADR-002 | PortalInhibitBackend, LogindInhibitBackend | 0.2.0 live portal GUI verification (all durations/replacement/expiry/quit); portal activation race fixed; `test_coordinator` | tested |
| KAE-AWAKE-002 | Request registry, overlap, suppression precedence (§6.2) | architecture.md §3.1 | InhibitionRegistry/Coordinator, ManualAwakeSession | `test_registry`, `test_coordinator`, `test_manualawake`; GUI replacement/turn-off/suppression verified | tested |
| KAE-LIFE-001 | Single instance, autostart, safe startup/quit (§7) | ADR-005 | Application, ControlService, AutostartBackend | autostart flags `--background --awake-on-login` verified; quit releases inhibitor; kill/restart no leak; 0.1.1 single-instance evidence unchanged | tested |
| KAE-UI-001 | Approved UI/UX system, accessible states (§8) | uiux-design.md, ADR-007 | three-page shell, stable Keep Awake card, tray, theme, a11y names | 0.2.0 GUI screenshots + state dumps; 15/15 suites; icon regression tests | implemented + GUI-verified (acceptance pending) |
| KAE-FOCUS-001 | Countdown + Pomodoro state machine (§6.3) | ADR-003 | FocusController, TimerEngine, FocusPage (preserved) | `test_focus`, `test_timerengine` pass; module forced off; no 0.2.0 UI/CLI | **deferred** |
| KAE-WORK-001 | Manual work session/project/segment tracking (§6.4) | architecture.md §3.3 | WorkSessionManager + repositories (preserved) | `test_workmanager`/`test_worklog` pass; module off; database not opened in 0.2.0 | **deferred / read-only** |
| KAE-DATA-001 | SQLite integrity, recovery, migration (§10) | ADR-004 | storage/*, RecoveryManager (preserved) | `test_storage` passes; 0.2.0 sandbox: pre-seeded DB hash unchanged across start/quit | **deferred / preserved** |
| KAE-REPORT-001 | Reports, filters, CSV (§6.4) | architecture.md | ReportsPage, CsvExport (preserved) | `test_reports`, `test_csv` pass; not in 0.2.0 UI/CLI | **deferred** |
| KAE-PRIV-001 | Local-first privacy, no telemetry (§2/§10) | privacy.md | no network code | audit: no sockets; privacy.md | enforced |
| KAE-PKG-001 | Maintainable .deb + lifecycle tests (§13) | testing.md §5 | debian/* | `kaeru_0.2.0-1_amd64.deb` built (328,640 B); lintian 1 informational; AppStream/desktop clean; 32/32 md5sums; extraction-run verified; **dpkg lifecycle pending root** | tested (build) / lifecycle pending |
| KAE-TEST-001 | Honest unit/integration/packaging evidence (§14) | testing.md | tests/* | **15 Qt Test suites (15/15 pass, zero warnings)**; 0.2.0 GUI/portal/tray/package evidence in `DELIVERY_REPORT_0.2.0.md` | tested |
| KAE-REL-001 | v1.0 release gate (§14) | testing.md §release gate | — | clean Release build, 15/15 tests, main `.deb` + optional dbgsym + SHA256SUMS; root lifecycle pending — see 0.2.0 delivery report | partial (release candidate ready) |

## Open decisions register

| Topic | Resolution | Where |
|---|---|---|
| App ID / D-Bus names | `io.github.kaeru.Kaeru` (ADR-001) | docs/decisions/ADR-001 |
| Backend preference | portal-first, logind fallback (ADR-002) | docs/decisions/ADR-002 |
| Clock sources | boot/monotonic/wall split (ADR-003) | docs/decisions/ADR-003 |
| Keep Awake concurrency | one manual session; new duration replaces old (0.2.0) | ManualAwakeSession + delivery report §3 |
| Suppression boundary for `awake off` | explicit new duration resumes; no schedules in 0.2.0 | Application + CLI |
| Checkpoint interval | 60 s (preserved, gated off in 0.2.0) | WorkSessionManager |
| Idle detection | out of scope; honest “unavailable” | capability matrix |
| Tray | SNI via QSystemTrayIcon + fallback (ADR-006) | docs/decisions/ADR-006 |
| Icon identity | official frog expressions; PNG runtime (ADR-008 + 0.2.0 note) | docs/decisions/ADR-008-frog-icon-identity.md |
| Focus / Work Log / Reports | deferred in 0.2.0; source/data preserved | docs/handoff/KAERU_OPEN_ISSUES.md |
