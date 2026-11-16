# WP-04 — Focus polish + Notifications Evidence

Environment: Pop!_OS 22.04, GNOME 42, X11 `DISPLAY=:1`,
`XDG_RUNTIME_DIR=/run/user/1000`, user `banana`. Binary: `kaeru 0.1.1`
(build tree). All runs sandboxed via `KAERU_TEST_HOME`.

## 1. Focus state machine — unit tested (existing, extended)

`ctest -j9` → 10/10 suites pass, including:

- `test_focus` — custom countdown lifecycle, pause freeze/resume (regression
  test for the pause() ordering bug fixed in WP-02), Pomodoro progression
  Work→ShortBreak→Work→LongBreak with autoNext, skip semantics (custom stops,
  pomodoro advances), extend +5 min, upcoming-work override, stop cancels
  timers (no orphan countdowns), quiet-hours windows incl. midnight wrap.
- `test_notify` (new) — notification policy decisions:
  `decideFocusIntervalNotification(false, *)` → no (disabled);
  `(true, true)` → no (quiet hours); `(true, false)` → yes.
  `decideKeepAwakeExpiryNotification` gate both ways.

## 2. Native notification: normal path — PASS

Script `/tmp/opencode/notify_test.sh` (documented here; not committed):

```
kaeru --background (sandbox kaeru-notify-test, quietHoursEnabled=false)
dbus-monitor --session "interface='org.freedesktop.Notifications'" &
kaeru focus start --duration 5s
```

Actual capture (kaeru → daemon):

```
method call … sender=:1.130 -> destination=org.freedesktop.Notifications;
  interface=org.freedesktop.Notifications; member=Notify
   string "io.github.kaeru.Kaeru"
   uint32 0
   string "io.github.kaeru.Kaeru"
   string "Focus countdown complete"
   string "Your focus countdown has finished."
```

Count of kaeru Notify calls to the daemon: **1** (expected 1 — no duplicates;
Custom mode text, not the Pomodoro break text). **PASS**

## 3. Native notification: quiet hours suppress — PASS (bug found & fixed)

Same procedure with all-day quiet hours (`quietHoursEnabled=true,
quietFrom=00:00, quietTo=23:59`) via `/tmp/opencode/quiet_test.sh`:

```
Notify calls during all-day quiet hours: 0        ← expected 0
config after run: quietHoursEnabled=true, quietTo=23:59  ← preserved
```

**Bug found by this test (fixed):** `SettingsPage::loadFromSettings()` called
`setChecked()` on the notification checkboxes, whose `toggled` signals ran
`onNotificationsChanged` — persisting the NOT-YET-LOADED quiet-hours widget
state (false / 00:00) back into Settings, silently disabling the user's quiet
hours at every launch. Fixed by blocking all change signals while seeding the
view (`QSignalBlocker` on every checkbox/timeedit/combobox; module checkboxes
included). Same class of bug was also fixed in `FocusPage` seeding (its
`valueChanged` handlers persisted widget minimums over user settings).
Verified: config survives a full launch + focus run byte-identical.

## 4. Quiet-hours semantics (implementation notes)

- Enforced by *not sending* the notification; system Do Not Disturb is never
  touched (spec §3/§6.3, ADR-002 non-interference).
- FocusController::quietHoursActive() evaluates the window from persisted
  minutes; `Application::applyFocusSettingsFromSettings()` seeds the
  controller from Settings at startup and after Settings-page changes.
- Sound: optional; passed as the XDG `sound-name` hint ("complete") via
  `Notifications::notify`. Playback is the daemon's job (GNOME 42 plays via
  libcanberra). Audible output is a manual verification item (no audio
  capture in this environment) — honestly labeled.

## 5. Focus ↔ Keep Awake wiring (verified earlier, unchanged)

`Application` wires `intervalStarted` → `applyFocusKeepAwake(...)` per
settings (keep awake during work / breaks, both off by default). Covered by
`test_coordinator` (keep-awake request lifecycle) + code inspection; native
effect visible via `kaeru status` during a focus run (effective
system-sleep+session-idle while "keep awake during work" enabled).

## 6. Not yet run (honest gaps)

- Audible sound verification (manual).
- GUI automation of the Focus page widgets (manual + screenshot; the page is
  fully wired: routine picker, custom minutes, defaults, options, all six
  controls with enable/disable states per `refresh()`).
- Real Pomodoro timing over 25+ minutes (logic covered by injected-clock
  unit tests; a real-time soak is a WP-07 regression item).
- Notifications daemon `GetCapabilities` check for "body-markup"/"sound" —
  not required for delivery (plain text + hint are spec-conformant).