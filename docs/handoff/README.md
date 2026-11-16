# Kaeru Handoff Package — README (entry point)

This directory is the primary handoff for the next coding agent taking over
Kaeru. It exists because the previous agent's "WP-00…WP-07 complete"
declaration does **not** mean the product passed user acceptance: the owner
has tested the application and reported four-plus issues that remain
**unresolved** (two `.deb` confusion, Keep Awake 2h/Indefinite, Focus Timer
UX rejected, Work Log recording prematurely, broken focused tray icon).

## Reading order

1. **`NEXT_AGENT_ONBOARDING.md`** — start here: paths, environment, build/
   test/run commands, source map, approval boundary, honest status warning.
2. **`KAERU_PROJECT_HISTORY.md`** — what KaerU is (product philosophy,
    users, Keep Awake vs Work Log) and the full WP-00…WP-07 development
    history with per-work-order objectives, results, evidence and status.
3. **`KAERU_TECHNICAL_ARCHITECTURE.md`** — technical walkthrough: layering,
    entry point, wiring root, each subsystem (keep awake, clocks, focus,
    work log, storage, platform, UI, frog identity, build/packaging) with
    files, classes, behavior, limitations and tests.
4. **`KAERU_CURRENT_STATE.md`** — exact current facts: Git state
    (branch `main`, HEAD `299f008`, clean tree), environment versions,
    build/test results (13/13), Debian artifacts with paths and checksums,
    and documented-vs-code contradictions.
5. **`KAERU_OPEN_ISSUES.md`** — the owner-reported acceptance issues
    (A: two packages; B: 2h/indefinite Keep Awake; C: Focus Timer
    rejected; D: Work Log records prematurely; E: focused icon black
    face), the investigation results (E reproduced and root-caused; B
    CLI-works + GUI defects identified; C/D need owner direction), plus
    the full status register and do-not-touch list.

## One-paragraph summary

Kaeru 0.1.1 is a Qt 6 (Widgets) Linux keep-awake/focus/work-log utility
with a clean from-scratch Release build (zero warnings), 13/13 sandboxed
test suites, native portal/logind inhibition evidence, a lintian-clean
Debian package (`kaeru_0.1.1-1_amd64.deb`), and the official frog icon
identity (awake/focused/sleep) integrated into tray, launcher and window.
It is **not accepted**: the owner's real-world testing found the focused
tray icon broken (Qt SVG clip-path rendering — reproduced), Keep Awake 2h/
indefinite failing in the GUI despite working via the shared core (two
GUI defects identified; exact repro pending), a Focus Timer whose design
the owner calls "horrible", and a Work Log that "records work
prematurely". Those four items are documented in `KAERU_OPEN_ISSUES.md`
with evidence and proposed next steps; none were fixed by this handoff.

## Ground rules (restated)

- Do not modify the original artwork (`../frog_icons/`) or the approved
  Master Specification.
- Do not rewrite Git history; do not push to any remote.
- Do not trust prior completion claims; re-verify behavior; never weaken
  tests for green results.
- Product-owner approval is required for design/semantics changes (see
  `KAERU_OPEN_ISSUES.md` and `NEXT_AGENT_ONBOARDING.md` §8).