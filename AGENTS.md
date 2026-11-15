# Kaeru — Agent Notes

This file is for agents working on Kaeru (see also the authoritative protocol in the documentation package).

- Authoritative docs: `PROJECT_SPEC.md`, `docs/*`, `docs/decisions/*`. Follow `AGENT_OPERATING_PROTOCOL.md` in the documentation package.
- Stack floor: Qt 6.2 (Pop!_OS 22.04 dev machine). Do not use newer Qt APIs without an ADR.
- Architecture rules: core has no GUI/DBus/SQL dependencies; platform adapters behind interfaces; all user commands go through `CommandRouter`.
- Never synthesize input activity; never log private content (tasks/tags/notes); never fabricate test results; label capabilities honestly.
- Tests must run in an isolated data home (`KAERU_TEST_HOME`) and never touch the user's real data.
- Keep the build green at each commit; keep commits small and logical.
- Packaging: `debian/` only via debhelper; no network in maintainer scripts; user data preserved on removal.
- App ID: `io.github.kaeru.Kaeru` (single constant `KAERU_APP_ID`).
