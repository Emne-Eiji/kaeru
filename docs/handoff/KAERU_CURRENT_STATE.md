# Kaeru — Current State (exact, as of 2026-11-16)

> Handoff package document. Every fact below was verified by direct
> execution or source inspection during the handoff preparation. Where a
> fact could not be verified, it is labeled as such.

## 1. Repository and Git

| Item | Value |
|---|---|
| Repository | `/home/banana/Constellation/Idea Dev/opencode/Kaeru/kaeru` (application repo) |
| Workspace root | `/home/banana/Constellation/Idea Dev/opencode/Kaeru` (contains `frog_icons/` originals, `kaeru/`, documentation package) |
| Branch | `main` |
| Latest commit | `299f008` — "WP-03 completion evidence and documentation" |
| Working tree | **clean** (verified `git status` empty at handoff time) |
| Untracked/uncommitted | none |
| Remote | none configured; nothing pushed anywhere |

Commit history (newest first):

```
299f008 WP-03 completion evidence and documentation
82e2df6 Debian packaging (WP-03 completion): debhelper + lintian-clean .deb
26b5a6d Official frog icon identity (ADR-008): tray states, launcher, resources
5ebecee WP-07: hardening, resource measurement, delivery report
7f181e2 WP-06 reports page, per-project/focus totals, real CSV export
400291a WP-05 work log end-to-end (manager, storage wiring, page, CLI, recovery)
a593f05 WP-04 focus notifications with quiet-hours policy, settings-view load fix
0c46ad0 test_clocks: tolerate 1s skew between separate 'now' reads (flake fix)
b898cb9 WP-03 (partial) tray/single-instance/autostart
21cc621 WP-02 storage/UI/tests/evidence
53ad601 WP-02 core
bd3b4c6 WP-01 design package
```

## 2. Environment (verified)

| Component | Version |
|---|---|
| OS | Pop!_OS 22.04 LTS (Ubuntu 22.04 base), GNOME 42, X11 (`DISPLAY=:1`) |
| Qt | 6.2.4 (widgets/gui/dbus/sql/test/svg) |
| qtsvg | libqt6svg6 6.2.4, icon engine `libqsvgicon.so` present |
| CMake | 3.22.1 |
| GCC | 11.4.0 |
| debhelper | 13.6 |
| lintian | 2.114.0 |
| dpkg-buildpackage | present (dpkg-dev) |
| Session | user `banana` (uid 1000), `XDG_RUNTIME_DIR=/run/user/1000`, TZ America/Toronto |
| sudo | **password required** (no passwordless sudo in this session) |

Primary target platform (declared, not tested): Ubuntu 24.04 LTS amd64.
Directly tested: Pop!_OS 22.04 only.

## 3. Build state

- Build dir: `kaeru/build` (CMake, `CMAKE_BUILD_TYPE=Release` verified in the
  cache at handoff time; reconfigure if in doubt).
- Clean Release build: **succeeds with zero warnings**
  (`-Wall -Wextra -Wpedantic`), verified by full rebuild during WP-07 and
  re-verified during handoff (`cmake --build build -j12` → exit 0).
- Binary: `kaeru/build/kaeru` (also staged at
  `/tmp/opencode/kaeru-pkg-extract/usr/bin/kaeru` from the package).
- App version string: `kaeru 0.1.1 (app id io.github.kaeru.KaerU)`.

Build commands:

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/KaerU/kaeru"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

## 4. Test state

- `ctest --test-dir build` (run from `kaeru/build`): **13/13 suites pass**,
  ~0.2 s total. Suites: clocks, registry, coordinator, timerengine, focus,
  worklog, storage, csv, cliparser, notify, workmanager, reports, traystate.
- Tests set `QT_QPA_PLATFORM=offscreen` via CMake test properties and
  isolate data via `KAERU_TEST_HOME` (QTemporaryDir) — the real user home
  is never touched.
- No compiler warnings in the test build either.

```bash
cd "/home/banana/Constellation/Idea Dev/opencode/KaerU/kaeru/build"
ctest --output-on-failure -j9
```

## 5. Debian package artifacts

| File | Location | Size | Notes |
|---|---|---|---|
| `kaeru_0.1.1-1_amd64.deb` | `KaerU/kaeru_0.1.1-1_amd64.deb` (repo parent) **and** `KaerU/kaeru/dist/` (identical, md5 `cdee11ef35468f01f1f5009999982db5`) | 361,064 B | the runtime package |
| `kaeru-dbgsym_0.1.1-1_amd64.ddeb` | `KaerU/kaeru-dbgsym_0.1.1-1_amd64.ddeb` (repo parent only) | 5,746,782 B | auto-generated debug symbols (optional; see Open Issues A) |
| `kaeru_0.1.1-1_amd64.buildinfo` / `.changes` | repo parent + `dist/` | — | build metadata |
| `kaeru-0.1.1-Linux-amd64.tar.gz` (+ `.sha256`) | `KaerU/kaeru/dist/` | 376,453 B | staged install tree |

- `lintian kaeru_0.1.1-1_amd64.deb` → only
  `W: initial-upload-closes-no-bugs` (informational; accepted, documented).
- `desktop-file-validate` clean; md5sums 32/32 OK.
- Package contents: `/usr/bin/kaeru`, desktop entry, metainfo, manpage,
  hicolor SVGs (awake/focused/sleep) + PNGs at 16–256, docs/copyright.
- Depends: shlibs Qt6 libs + `libqt6sql6-sqlite` + `libqt6svg6`;
  Recommends `xdg-desktop-portal`. No maintainer scripts, no conffiles.
- **Not tested (root required):** `dpkg -i`, upgrade, remove, reinstall.
  Verified instead via `dpkg-deb -x` + run (see `docs/evidence-wp03.md`).

## 6. Runtime facts (verified live during this project)

- Background instance + CLI over D-Bus: `kaeru status`, `kaeru awake on
  --duration 30m --mode system` → portal inhibition active
  (`"backend":"portal","status":"active"`); 2h and indefinite durations
  also activate correctly via the CLI path (handoff investigation).
- Tray: SNI item registered on the session watcher; ToolTip reflects state
  ("Kaeru — inactive", "Kaeru — keep awake active · …",
  "Kaeru — focus running · …"); panel diff isolates the icon.
- Notifications: focus-interval completion sends exactly one
  org.freedesktop.Notifications.Notify; quiet hours suppress it.
- Work log: `kaeru work start|pause|resume|stop`; SQLite at
  `$XDG_DATA_HOME/kaeru/kaeru.db` (WAL); CSV export works.
- Single instance: second launch forwards ShowWindow and exits 0.

## 7. Documentation map (authoritative)

- Product spec: `PROJECT_SPEC.md` + the documentation package
  (`KaerU_Documentation_Package_v0.1.1/`) — **do not rewrite**.
- Design: `docs/architecture.md`, `docs/uiux-design.md`, ADRs
  `docs/decisions/ADR-001…008`.
- Status: `docs/capability-matrix.md` (honest per-capability status),
  `docs/traceability.md`, `docs/testing.md`, `docs/privacy.md`.
- Evidence: `docs/evidence-wp02..06.md`, `docs/evidence/` (tray crops),
  `docs/delivery-report.md`, `docs/icon-assets.md`.
- Agent rules: `AGENTS.md`.

## 8. Known contradictions between docs and code (found during handoff)

1. `docs/capability-matrix.md` and the delivery report describe the tray
   icons as "renders once libqt6svg6 present" — rendering now works, but
   the **focused expression renders black** in Qt's SVG renderer
   (Open Issues E); the docs predate that finding.
2. `OverviewPage.cpp` line ~189 still shows "Manual work tracking arrives
   with the Work Log milestone" although WP-05 shipped the Work Log
   (stale UI text).
3. `RecoveryManager` is documented as the recovery component; the actual
   logic lives in `WorkSessionManager::recoverInterrupted` (the class is a
   stub).
4. `docs/evidence-wp02.md` mentions `busctl … CanSuspend` as evidence;
   the property flip was superseded by `systemd-inhibit --list` evidence
   (the file says so; the capability matrix wording varies).
5. The delivery report's commit table was written before the final two
   commits; the current table in `docs/delivery-report.md` §8 lists them
   (verify hashes against `git log` before citing).