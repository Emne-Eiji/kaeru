# Kaeru — Requirement Traceability Matrix

Status vocabulary: `implemented`, `tested` (with evidence), `partial`, `unavailable`, `not evaluated`.

| Req ID | Requirement (spec §) | Design artifact | Implementation | Tests / evidence | Status |
|---|---|---|---|---|---|
| KAE-PROD-001 | Lightweight native utility, independent modules (§2) | architecture.md | Application module switches | UI/tray/CLI toggles release resources (WP-02+ tests) | in progress → WP-07 |
| KAE-SAFE-001 | No fake input/presence falsification/monitoring (§3) | ADR-002, privacy.md | nothing implements input synthesis; code audit | code review WP-07; grep audit | enforced by design |
| KAE-AWAKE-001 | Native inhibition with verified lifecycle (§6.1) | ADR-002 | PortalInhibitBackend, LogindInhibitBackend | docs/evidence-wp02.md §2.1–2.3 (real portal Response(0) + systemd-inhibit block delta + logind sleep:block); integration re-run per release | tested |
| KAE-AWAKE-002 | Request registry, overlap, suppression precedence (§6.2) | architecture.md §3.1 | InhibitionRegistry/Coordinator | test_registry, test_coordinator (all pass); suppression/quit native check evidence-wp02 §2.4 | tested |
| KAE-LIFE-001 | Single instance, autostart, safe startup/quit (§7) | ADR-005 | Application, ControlService, AutostartBackend | quit release verified natively (evidence-wp02 §2.4); single-instance/autostart tests WP-03 | partial |
| KAE-UI-001 | Approved UI/UX system, accessible states (§8) | uiux-design.md, ADR-007 | ui/* pages + theme + a11y | GUI tests + screenshots | WP-01→07 |
| KAE-FOCUS-001 | Countdown + Pomodoro state machine (§6.3) | ADR-003 | FocusController, TimerEngine | test_focus, test_timerengine (all pass); native focus run + notification evidence docs/evidence-wp04.md | tested |
| KAE-WORK-001 | Manual work session/project/segment tracking (§6.4) | architecture.md §3.3 | WorkSessionManager + repositories | test_workmanager (10 tests); native lifecycle + crash recovery docs/evidence-wp05.md | tested |
| KAE-DATA-001 | SQLite integrity, recovery, migration (§10) | ADR-004 | storage/*, RecoveryManager | test_storage, test_worklog, test_workmanager (migrations, FK, totals, checkpoints, conservative recovery) | tested |
| KAE-REPORT-001 | Reports, filters, CSV (§6.4) | architecture.md | ReportsPage, CsvExport, SessionRepository totals | test_reports, test_csv; native CLI export docs/evidence-wp06.md | tested |
| KAE-PRIV-001 | Local-first privacy, no telemetry (§2/§10) | privacy.md | no network code | audit: no sockets; privacy.md | enforced |
| KAE-PKG-001 | Maintainable .deb + lifecycle tests (§13) | testing.md §4 | debian/* | packaging tests WP-03/07 | WP-03/07 |
| KAE-TEST-001 | Honest unit/integration/packaging evidence (§14) | testing.md | tests/* | docs/testing evidence records | all |
| KAE-REL-001 | v1.0 release gate (§14) | testing.md §release gate | — | final regression WP-07 | WP-07 |

## Open decisions register

| Topic | Resolution | Where |
|---|---|---|
| App ID / D-Bus names | `io.github.kaeru.Kaeru` (ADR-001) | docs/decisions/ADR-001 |
| Backend preference | portal-first, logind fallback (ADR-002) | docs/decisions/ADR-002 |
| Clock sources | boot/monotonic/wall split (ADR-003) | docs/decisions/ADR-003 |
| Suppression boundary for `awake off` | explicit user action resumes; no schedules in 1.0 | CommandRouter + docs |
| Checkpoint interval | 60 s | RecoveryManager |
| Idle detection | out of 1.0; honest "unavailable" | capability matrix |
| Tray | SNI via QSystemTrayIcon + fallback (ADR-006) | docs/decisions/ADR-006 |
