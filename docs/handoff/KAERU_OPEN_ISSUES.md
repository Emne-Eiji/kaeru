# Kaeru — Open Issues and Unresolved Validation

> Handoff package document. The items here are **user-reported acceptance
> issues and honest gaps**. They are not resolved merely because automated
> tests pass. Nothing in this file has been fixed by the handoff author
> (documentation-only assignment).

## USER-REPORTED ACCEPTANCE ISSUES — UNRESOLVED

### Issue A — Two Debian packages (debug symbols confuse delivery)

**User report:** two `.deb` files were found, one described as debugging
symbols for Kaeru.

**What the build actually produces (verified):**

| File | Size | What it is |
|---|---|---|
| `kaeru_0.1.1-1_amd64.deb` | 361,064 B | the runtime package (binary, desktop, icons, manpage) |
| `kaeru-dbgsym_0.1.1-1_amd64.ddeb` | 5,746,782 B | auto-generated debug symbols |

- The dbgsym package is created automatically by debhelper 13
  (`dh_strip --dbgsym-migration`-style behavior): control shows
  `Package-Type: ddeb`, `Auto-Built-Package: debug-symbols`,
  `Depends: kaeru (= 0.1.1-1)`, `Section: debug`, `Priority: optional`.
  Its payload is `/usr/lib/debug/.build-id/ce/7eafc….debug` (matching the
  Build-Ids field).
- This is **standard Debian behavior** for debhelper ≥ 13; it is not a Kaeru
  packaging bug.
- **Normal users only need `kaeru_0.1.1-1_amd64.deb`**; the dbgsym is
  optional and only useful for debugging crashes. It is safe to delete it.
- The main package contains every runtime component: binary, desktop entry,
  metainfo, manpage, hicolor icons, docs. Verified by content audit.
- **Delivery-instruction gap:** the delivery report and the owner-facing
  instructions never told the user which file to install. The correct
  instruction is: `sudo apt install ./kaeru_0.1.1-1_amd64.deb`. This is a
  documentation problem, not a package problem — but it confused the owner,
  which is a real usability defect of the delivery.

**Status:** explained; not changed (no packaging changes allowed in this
assignment). Proposed next step (needs owner approval): either ignore the
dbgsym in the handoff instructions, or configure `dh_strip --no-automatic-dbgsym`
explicitly if the owner wants a single-file release.

### Issue B — Keep Awake: 2 hours and Indefinite "fail" while 15m/30m/1h work

**User report:** duration options 15 min / 30 min / 1 hour work; 2 hours and
Indefinite do not work correctly.

**What was verified during the handoff (direct execution):**

- CLI path (`kaeru awake on --duration 2h --mode both`, same Application
  methods the GUI chips call): **works** —
  `"keepAwake":{…"remainingMs":7197983,…"status":"active"}` (remaining ≈ 2 h).
- Indefinite (`kaeru awake on` with no duration → `awakeIndefinite`):
  **works** — `"remainingMs":-1,…"status":"active"`.
- Therefore the core request/expiry logic is not the cause; the failure is
  in the **GUI interaction layer** (OverviewPage chips / tray menu) or in
  how the user's sequence of clicks interacts with stacked requests.

**Concrete GUI-side facts found in code (candidate causes):**

1. `OverviewPage::refresh()` line ~180 does `m_awakeWarning->setVisible(false)`
   **on every refresh** (a 1 s tick refreshes the page). In
   `onDurationChosen(0)` (the Indefinitely chip) the warning label is shown,
   then `refresh()` runs at the end of the handler and **immediately hides
   it** — the "will stay on until you turn it off" warning is never visible.
2. Clicking chips in sequence **stacks requests** (each chip adds a new
  `InhibitionRequest`; the registry keeps them all). The UI's remaining
   display uses `earliestRemainingMs` — the **earliest** expiry among all
   stacked requests, not the newest. After clicking 15m then 30m then 1h
   then 2h, the display shows the *earliest* (e.g. ~14 m from the 15m
   request), so the 2h chip "appears not to work". Same for Indefinite
   (remaining shows the older timed request's countdown, not ∞).
3. The tray menu duplicates the same chip semantics (`resumeAutomaticRules();
   app->awakeOn(...)` per entry).

**What could NOT be established:** the exact reproduction of the owner's
session (GUI automation of the overview page was not possible; no
screenshot/video of the failing interaction exists). The two code facts
above fully explain the observed symptoms under sequential clicking, but
confirmation requires the owner's reproduction.

**Proposed reproduction/diagnosis procedure for the next agent:**

1. Launch the GUI (build tree or installed), open Overview.
2. Click 15m, then 30m, then 1h, then 2h, then Indefinitely **without
   waiting for expiry**; observe the remaining-time label after each click.
   Expectation from code: label shows the earliest stacked expiry (bug).
2b. Click 2h on a fresh instance (no prior requests): the request
    activates (status Active, remaining ≈ 2 h) — if that also "fails" for
    the owner, capture `kaeru status --json` and the journal
    (`journalctl --user`) for portal errors.
3. Check whether the Indefinite warning label ever becomes visible (it
    should not — refresh hides it).
4. Only after reproducing, decide the fix with the owner (options: replace
    stacked requests instead of adding; show the newest expiry; fix the
    warning persistence; clarify tray semantics).

**Status:** root cause not confirmed end-to-end; two concrete GUI defects
identified; product decision required (stacking semantics).

### Issue C — Focus Timer design/implementation rejected by the owner

**User report (verbatim assessment):** "Focus work session design and
implementation and function is horrible!"

**What the current implementation actually is (code-verified):**

- `FocusPage` (`src/ui/pages/FocusPage.{h,cpp}`): a two-column form —
  left: status badge, big timer label, phase info, six buttons
  (Start/Pause/Resume/Skip/Extend/Stop); right: routine picker
  (Pomodoro/Custom countdown), custom minutes, Pomodoro defaults (work/
  short/long/every), option checkboxes (keep awake during work/breaks,
  record in work log, auto-next).
- `FocusController` (`src/core/FocusController.{h,cpp}`): Idle → Work →
  ShortBreak/LongBreak …; **autoNext is off by default**, so after each
  interval the phase returns to **Idle** and the next interval only starts
  when the user presses Start again (the page's Start uses
  `startPomodoro()`, which **resets** `m_completedWork` and the interval
  counter). Interval transitions therefore require manual restarts, and
  the completed-work counter resets every time the routine is restarted —
  a user who follows the intended flow (start → work → start → break …)
  ends up with the counter at 1 forever.
- Pause cancels the countdown and recreates it on resume with the frozen
  remainder (correct but surprising); skip in Pomodoro jumps to the next
  phase as if completed (achieved time = elapsed).
- Notifications on interval completion (policy-gated, quiet hours).
- The UI is settings-dense rather than glanceable; there is no dedicated
  "focus mode" presentation, no break overlay, no progress ring — the
  spec's uiux describes a simple page, and that is what was built.
- Unit tests pass (state machine transitions, pause/resume, quiet hours);
  **no native/UX validation was ever performed by the owner until now.**

**Why passing tests did not establish acceptable design:** the tests verify
the state machine's internal consistency, not whether the interaction
matches what a person expects from a focus timer. The owner's verdict is
the acceptance criterion; it overrides the implementation's self-certification.

**Status:** owner rejection stands; **do not redesign without the owner's
direction** (what specifically is horrible: flow? visuals? the manual
restarts? the dense page?). Proposed next step: a design conversation
with the owner, then a focused rework — possibly a minimal single-timer
flow (start once, intervals flow automatically, big glanceable timer)
rather than the current settings-form design.

### Issue D — Work Log "records work prematurely"

**User report:** "Work log written and work prematurely."

**What the code actually does (verified by source audit):**

- A work session + first work segment are created **only** by explicit
  `work start` (GUI WorkLogPage Start, tray Work Log ▸ Start, or
  `kaeru work start`). No auto-start exists anywhere (no caller of
  `startSession`/`start()` besides the UI/router paths; `prepareQuit`
  only *stops* an open session).
- `recordFocusWork` (focus interval completion with `recordInWorkLog`
  enabled, off by default) writes to **`focus_sessions`**, never to
  work-log segments — no double counting.
- Segments accrue continuously from start until **pause/stop/lock/suspend**
  — there is **no idle detection** (declared out of 1.0 scope). If the
  user starts recording and then does something else without locking or
  stopping, the time is recorded as work. The app cannot know the user is
  not working; that is the honest limitation, but it may be exactly what
  the owner experienced.
- On graceful quit (`prepareQuit`) an open session is stopped and **saved**
  — quitting writes the session even if the user never explicitly stopped.
- Recovery at startup closes interrupted sessions at the last checkpoint
  (or segment start) — never invents work.
- Totals only count **closed** segments of kind `work`; paused/idle time
  goes to `breakMs`, open segments never count.

**Assessment:** from code alone, nothing "premature" writes work data
without a user action — the plausible mechanisms for the owner's
experience are (a) an explicitly started session left running (no idle
detection), (b) the graceful-quit save of an open session, or (c) an
expectation mismatch about when recording should start/stop. This cannot
be decided from code; it **requires product-owner clarification** of the
intended recording semantics (e.g. should recording auto-pause on
inactivity? should quit discard instead of save? should focus recording be
the default?).

**Status:** requires product-owner decision; no code change made.

### Issue E — "icon in focus mode is broken!" — REPRODUCED

**User report:** the icon shown in focus mode is broken.

**Handoff investigation (direct rendering probe, Qt 6.2.4):**

- The tray/window icons load from Qt resources `:/icons/frog/*.svg`
  (`src/ui/Icons.cpp`), i.e. the SVG files — not the PNGs.
- Rendering `focused.svg` through Qt's QSvgRenderer produces a **black
  face**: at 50×50, face pixels are `(0,0,0,255)` while `awake.svg` and
  `sleep.svg` render the correct green `(76,175,80,255)` at the same
  coordinates. At 100×100 the focused render has ~7958 dark pixels vs
  ~2100 for the others (face area painted dark), confirming the face is
  mis-rendered.
- The cause is the `focused.svg` construction: two `<clipPath>`
  elements (`#lidL`, `#lidR`) with `clip-path="url(#…)"` on the white
  eye circles. Qt's SVG (Tiny) renderer mishandles this construct (likely
  clip-path coordinate handling inside the transformed group), painting
  the face black. The **supplied PNGs rasterize correctly**
  (`focused-64.png` center = (76,175,80) — produced by `rsvg-convert`,
  which handles the clipPath correctly).
- Consequence: in the real application the focused (focus-running) tray
  icon is a black blob — exactly the owner's report.

**Status:** reproduced and root-caused. **Not fixed** (fixing is next-agent
work with owner approval). Fix direction options (owner decision):
(a) use the supplied PNG variants for the tray icon at runtime (they are
correct), (b) restructure `focused.svg` without clipPath (e.g. pre-cut
paths), (c) ship a fixed SVG from the original generator — note the
original files in `frog_icons/` must remain untouched, so any change must
be an application-side asset decision documented in ADR-008.

## Other known defects / unresolved validation (verified)

| # | Item | Status |
|---|---|---|
| 1 | dpkg-database lifecycle tests (install/upgrade/remove/reinstall) | **not tested** — root required; extraction-run verified instead |
| 2 | Ubuntu 24.04 native validation | **not tested** — declared gap (Pop!_OS 22.04 only) |
| 3 | Real suspend/resume cycle | **not tested** — manual procedure only (user session protection); boot-clock behavior unit-tested with injected clocks |
| 4 | Audible notification sound | **not tested** — `sound-name` hint sent; playback is daemon-side |
| 5 | Full GUI usability / screenshots | **partial** — tray/panel verified with screenshots; full-window flows (focus, work log, reports) not screenshot-verified |
| 6 | Focus Timer usability | **rejected by owner** (Issue C) |
| 7 | Work Log correctness | **disputed by owner** (Issue D) |
| 8 | 2h/indefinite Keep Awake | **works via CLI; GUI stacked-request display bug identified** (Issue B) |
| 9 | Focused tray icon | **broken (black face)** — reproduced (Issue E) |
| 10 | OverviewPage stale Work Log text | minor UI inconsistency (says "arrives with the Work Log milestone") |
| 11 | `session_revisions` table | written by nothing yet (provenance deferred) |
| 12 | `RecoveryManager` class | stub; real logic in WorkSessionManager |

## Status register (legend)

- **Verified by direct execution** — reproduced live during this project
  (native inhibition, tray states, notifications, work-log DB contents,
  package extraction-run, icon probes).
- **Verified by automated tests** — 13/13 Qt Test suites (sandboxed).
- **Verified by source-code inspection** — code facts in this file
  (stacked requests, warning hide, clipPath rendering, caller audit).
- **Documented but not independently verified** — Ubuntu 24.04, real
  suspend, audible sound, full GUI flows.
- **User-reported failure** — Issues A–E (A: explained/standard; B–E
  open).
- **Blocked** — dpkg lifecycle tests (root), any apt operation (password).
- **Requires product-owner decision** — B (stacking semantics), C (focus
  redesign direction), D (recording semantics), E (fix direction).

## Do-not-touch list for the next agent

- `../frog_icons/` — original artwork; must remain byte-identical.
- `KaerU_Documentation_Package_v0.1.1/MASTER_SPECIFICATION.md` — do not
  rewrite the approved spec.
- Git history — do not rewrite/reset; do not push anywhere.
- User data (real `$XDG_*` dirs) — all testing must stay in
  `KAERU_TEST_HOME` sandboxes.