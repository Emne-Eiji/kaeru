# ADR-001 — Canonical application identity: `io.github.kaeru.Kaeru`

**Status:** APPROVED (implementing under user authorization of 2026-11-15; identity is reversible before any external publication)
**Date:** 2026-11-15

## Context

The spec requires a canonical reverse-DNS application ID and D-Bus naming derived from Kaeru, with no `AwakeFlow` residue, and forbids inventing a domain the project does not own. No organization or domain exists yet.

## Options

1. `io.github.kaeru.Kaeru` — GitHub-style namespace (Flatpak's recommended convention for projects hosted on GitHub without an owned domain). Does not claim ownership of any real domain.
2. `org.kaeru.Kaeru` — implies ownership of kaeru.org (not owned). Rejected.
3. `dev.kaeru.Kaeru` — similar domain-claim problem. Rejected.
4. Launching without any D-Bus identity — breaks single-instance and IPC requirements. Rejected.

## Decision

Use **`io.github.kaeru.Kaeru`** everywhere:

- Executable/CLI: `kaeru`
- D-Bus service name: `io.github.kaeru.Kaeru`
- D-Bus control interface: `io.github.kaeru.Kaeru.Control1` at object path `/io/github/kaeru/Kaeru`
- Desktop entry: `io.github.kaeru.Kaeru.desktop`; icon name: `io.github.kaeru.Kaeru`
- XDG directories remain short-form per spec: `kaeru/` under config/data/cache/runtime.

## Impact

User-visible only through launcher metadata. Renaming later is a mechanical change (constant `KAERU_APP_ID` in one header, referenced everywhere). If the user later assigns a real domain, a one-line change plus package-name update suffices before external publication.
