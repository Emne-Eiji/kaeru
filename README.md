# Kaeru

**Kaeru** (蛙 — "frog") is a lightweight, Linux-native desktop utility for Ubuntu-style systems that helps you:

- prevent unwanted automatic system sleep (verified native inhibition, not settings changes),
- optionally keep the display awake,
- run timed keep-awake sessions,
- organize focused work and breaks (countdown + Pomodoro),
- record and review your work hours honestly — keeping the computer awake is never mistaken for working,
- start automatically after login only when you explicitly enable it,
- be controlled from the system tray, with a visible fallback when no tray exists.

Local-first: no accounts, no cloud, no telemetry. No fake input, no monitoring.

- License: GPL-3.0-or-later (see `LICENSE`)
- Primary target: Ubuntu 24.04 LTS `amd64` (`.deb`); validated on Pop!_OS 22.04 (Ubuntu base) — see `docs/capability-matrix.md`

## Build (from source)

```bash
sudo apt install build-essential cmake qt6-base-dev libsqlite3-dev debhelper
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

## Install (package)

```bash
dpkg-buildpackage -us -uc
sudo apt install ../kaeru_<version>_amd64.deb
```

Run `kaeru` from your applications menu, or `kaeru --help` for CLI control.

## Documentation

- `PROJECT_SPEC.md` — product requirements digest
- `docs/architecture.md` — system design
- `docs/uiux-design.md` — UI/UX system
- `docs/capability-matrix.md` — what is tested vs not
- `docs/privacy.md` — data and privacy
- `docs/testing.md` — test strategy and evidence
- `docs/traceability.md` — requirements ↔ code ↔ tests
- `docs/decisions/` — architecture decision records
