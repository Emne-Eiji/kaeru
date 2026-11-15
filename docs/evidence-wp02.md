# WP-02 — Unit and Native Integration Evidence

Rule (testing.md §header): exact command, environment, expected, actual, pass/fail.
Mocked/fake results are never labeled native evidence; unit tests below are
explicitly non-native (FakeInhibitionBackend), while §2 is real desktop evidence.

Environment (all §2 runs): Pop!_OS 22.04 LTS, GNOME 42, X11 `DISPLAY=:1`,
`XDG_RUNTIME_DIR=/run/user/1000`, user `banana` (uid 1000), kernel 5.19-ish
(Pop 22.04 HWE), systemd 249, xdg-desktop-portal (GNOME backend) running.
Binary: `kaeru 0.1.1` Debug build, app id `io.github.kaeru.Kaeru`.
Sandbox for CLI runs: `KAERU_TEST_HOME=/tmp/opencode/kaeru-int-test`
(all data isolated from the real user home; user data never touched).

## 1. Unit tests (fake clocks/backends — NOT native evidence)

Command: `cd kaeru/build && ctest -j9`
Expected: 9/9 suites pass. Actual: **9/9 passed** (0.15 s total).

| Suite | Assertions (summary) | Result |
|---|---|---|
| test_clocks | fake time advance; monotonic non-decreasing; CLOCK_BOOTTIME available; wall UTC/Toronto consistency; duration parse accept/reject (incl. `1h30m`, trailing digits); ISO date; formatters | PASS |
| test_registry | add/remove/expire (Duration + UntilTime), earliestRemaining, primaryReason by priority, clear + changed signal | PASS |
| test_coordinator | requested≠active until confirmed; expiry releases; suppress blocks new requests until resume; module disable; Pause All (registry cleared, no auto-restart); failure→fallback by backendId; failure without fallback honest; permission-denied distinct; unsupported capability never faked; binding lost→re-acquire; removal releases | PASS |
| test_timerengine | countdown completes once on boot clock (incl. across simulated suspend); stopwatch accrual pauses at suspend and stays paused after resume (spec); wall deadline fires on pass; cancel; invalid inputs rejected | PASS |
| test_focus | conservative defaults (recording off, autoNext off); custom lifecycle; pause freezes / resume restarts (regression test for pause() ordering bug); pomodoro progression Work→Short→Work→Long (autoNext); skip semantics; extend; upcoming-work override; stop cancels; quiet-hours windows incl. midnight wrap | PASS |
| test_worklog | open segments never count; paused excluded from totals; append closes previous (no overlap); single active session; closing session closes segments; checkpoints never add report data; focus sessions separate; daily totals with -5h offset | PASS |
| test_storage | DB open + WAL + quick_check; migrations versioned/idempotent; project CRUD + guards; session round-trip; FK integrity enforced; backup-before-migration file created | PASS |
| test_csv | quoting (commas/quotes/newlines); formula-injection prefixes (`=`,`+`,`-`,`@`,tab); header + open-segment row format; write/truncate + row count; filename convention | PASS |
| test_cliparser | GUI/default; help/version; --background; status; awake on with parsed durationMs (rejected malformed); export dates normalized; unknown command/mode/date rejected; missing operand passes to runner (usage exit 2 handled there) | PASS |

## 2. Native inhibition evidence (real desktop, not mocked)

### 2.1 Portal path — system-sleep inhibition (`--mode system`)

Commands (background instance + CLI over D-Bus; each step's actual output recorded):

```
KAERU_TEST_HOME=/tmp/opencode/kaeru-int-test ./build/kaeru --background &
KAERU_TEST_HOME=/tmp/opencode/kaeru-int-test ./build/kaeru awake on --duration 10s --mode system
KAERU_TEST_HOME=/tmp/opencode/kaeru-int-test ./build/kaeru status
```

Actual while active:

```
{"app":"kaeru","background":true,…,
 "keepAwake":{"backend":"portal","effective":["system-sleep"],
              "reason":"Keep awake (timed)","remainingMs":9976,
              "requested":["system-sleep"],"status":"active"},…}
```

`systemd-inhibit --list` delta during the request (portal inhibit is
forwarded by GNOME to logind via gnome-session, which is why the inhibitor
owner is gnome-session-b, not kaeru directly):

```
banana  1000 banana 4571 gnome-session-b sleep   user session inhibited   block   ← present while active
```

After the 10 s expiry (waited 11 s):

```
kaeru status → "status":"inactive"
systemd-inhibit → gnome-session-b sleep:block gone (only gsd-media-keys power-key block remains)
```

Expected: active during window, inactive + no kaeru-related inhibitor after.
Actual: matches. **PASS** (native evidence: real portal grant + logind delta).

### 2.2 Portal path — idle/display inhibition (`--mode display`), D-Bus capture

`dbus-monitor --session "path_namespace='/org/freedesktop/portal/desktop'"` while
`kaeru awake on --duration 7s --mode display`:

```
method call … interface=org.freedesktop.portal.Inhibit; member=Inhibit
   uint32 8                      ← flags 8 = idle (display) inhibition
signal … interface=org.freedesktop.portal.Request; member=Response
   uint32 0                      ← Response code 0 = granted
method call … interface=org.freedesktop.portal.Request; member=Close   ← release path
```

Mid-flight `kaeru status`:

```
"keepAwake":{"backend":"portal","effective":["session-idle"],
             "remainingMs":4962,"requested":["session-idle"],"status":"active"}
```

Expected: Inhibit(flags=8) → Response(0) → active → Close on expiry.
Actual: matches. **PASS** (native evidence).

### 2.3 Logind fallback backend — real sleep:block inhibitor (ADR-002)

Harness (throwaway, in /tmp, NOT part of the project) instantiates kaeru's
`LogindInhibitBackend` directly against the live system bus:

```
/tmp/opencode/harness_build/logind_harness   (holds a SystemSleep binding 10 s)
```

Actual while held:

```
backendId=logind reachable=1 detail=org.freedesktop.login1.Manager available
supportedCapabilities: system-sleep=1 session-idle=0    ← logind cannot keep a display awake (not claimed)
acquire() returned binding c4aeaee9-… — holding 10s
BINDING ACTIVATED: c4aeaee9-…
systemd-inhibit --list:
Kaeru  1000 banana 12509 logind_harness sleep   kaeru logind integration test   block
```

After `release()`: `systemd-inhibit` no longer lists any kaeru inhibitor.

Expected: real sleep:block inhibitor while held; gone after release; idle
capability honestly reported unavailable. Actual: matches. **PASS**
(native evidence; FD lock semantics verified — inhibitor disappears only on
release/process exit, and the fd is FD_CLOEXEC-marked in the backend).

### 2.4 Quit cleanup + D-Bus CLI exit codes

```
./build/kaeru awake on --mode system     → "Keep awake requested (indefinite) — activation confirmed asynchronously"
./build/kaeru status                     → active (system-sleep+session-idle via portal)
./build/kaeru quit                       → (void reply; fixed CLI to use QDBusMessage)
pgrep -x kaeru                           → process exited
systemd-inhibit --list                   → gnome-session-b block gone
```

Expected: quit releases every inhibitor and exits; CLI quit returns 0.
Actual: matches (the earlier "Unexpected reply signature" CLI bug for the
void Quit reply was found by this test and fixed in `main.cpp`). **PASS**

### 2.5 CLI status over D-Bus (IPC sanity)

```
./build/kaeru status  → exit 0, full JSON (version 0.1.1, modules, keepAwake, focus)
```

Expected: JSON status from the running background instance. Actual: matches. **PASS**

### 2.6 GUI smoke (offscreen)

```
KAERU_TEST_HOME=/tmp/opencode/kaeru-gui-smoke QT_QPA_PLATFORM=offscreen \
  timeout 6 ./build/kaeru        → exit 124 (survived full 6 s; offscreen
                                    "raise()/propagateSizeHints()" notes are
                                    normal offscreen-platform messages)
```

Expected: MainWindow + all six pages construct without crash; sandbox
config/cache created under KAERU_TEST_HOME. Actual: matches. **PASS**
(Real X11 GUI + tray + screenshots are WP-03 per execution plan.)

## 3. Not yet run (honest gaps, scheduled)

- Real suspend/resume cycle — manual procedure only (user session protection),
  see testing.md §6.1. Boot-clock suspend-inclusive behavior is covered by
  injected-clock tests only until the user runs the manual procedure.
- `busctl … CanSuspend` property flip — superseded by the stronger
  `systemd-inhibit --list` block evidence above (the property is a query of
  the same logind inhibitors).
- Tray SNI registration + screenshots — WP-03.
- Ubuntu 24.04 native validation — no such machine (declared limitation).