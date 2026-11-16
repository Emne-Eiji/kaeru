# WP-06 — Reports, Filters, CSV Evidence

Environment: Pop!_OS 22.04, GNOME 42, X11, user `banana`. Binary: `kaeru 0.1.1`
(build tree). All runs sandboxed via `KAERU_TEST_HOME`.

## 1. Unit tests — `test_reports` (new) + full suite

`ctest -j9` → 12/12 suites pass. `test_reports` covers:

- `totalsByProjectAndFocus` — two projects with work+paused segments →
  per-project work/break/session counts correct (Alpha: 600 s work + 120 s
  break, 1 session; Beta: 300 s work); focus_sessions totals (1 session,
  25 min achieved); out-of-range queries see nothing.
- `openSegmentsNeverCount` — a still-open segment contributes nothing to
  project totals (honest accounting).
- CSV guard (in `test_csv`): fields beginning with `= + - @`, tab or CR get
  an apostrophe prefix; quoting of commas/quotes/newlines.

## 2. Native CLI export — PASS

Script `/tmp/opencode/export_test.sh`: record a work session with project
`R&D =evil()`, stop, then:

```
kaeru export --format csv --from 2026-11-16 --to 2026-11-16 \
  --output /tmp/opencode/export-test.csv
→ Exported 1 row(s) to /tmp/opencode/export-test.csv
```

File content:

```
session_id,segment_id,project,start_utc,end_utc,duration_minutes,kind,source
1,1,R&D =evil(),2026-11-16T08:13:44Z,2026-11-16T08:13:46Z,0,work,manual
```

- 1 data row (the recorded session's single work segment).
- `duration_minutes` = 0 — 2 seconds rounded DOWN to whole minutes (honest;
  never rounded up).
- Formula-injection check: no field begins with `=`, `+`, `-`, `@` — the
  `=evil()` text is mid-field and harmless; leading-trigger cases are covered
  by `test_csv` (apostrophe prefix).
- Default output path (no `--output`) = `$PWD/kaeru-worklog-YYYYMMDD-YYYYMMDD.csv`.

## 3. Reports page (GUI)

- Date range (From/To, default last 7 days), project filter (All projects +
  project list), auto-refresh on change.
- Daily table (local days via UTC offset): work / break+paused / total;
  project breakdown rows below; focus time (achieved) in the summary line.
- CSV export button → save dialog (default filename per uiux §6) → summary
  dialog with row count and destination.
- Manual GUI verification (screenshot) is a WP-07 regression item; the
  underlying data paths are unit- and CLI-tested above.

## 4. Data deletion (Settings)

- `SettingsPage` "Delete all Kaeru data…" → first confirmation (what exactly
  is deleted: work-log DB, settings, cache) → second confirmation (exact
  paths) → removes `<config>/kaeru`, `<data>/kaeru`, `<cache>/kaeru` and
  asks the user to restart Kaeru (the running process keeps its open handles
  until restart — stated in the dialog).
- Only Kaeru's own directories are ever touched (no home traversal).
- GUI-driven; manual verification item (the dialog text is quoted in
  uiux-design §6 and implemented verbatim).

## 5. Not yet run (honest gaps)

- GUI screenshots of the reports page (WP-07 regression).
- Real focus→worklog native run (option `focusRecordInWorkLog` → focus_sessions
  inserts; unit-tested; native check with a live Pomodoro is a WP-07 soak).
- Ubuntu 24.04 validation — declared environment gap.