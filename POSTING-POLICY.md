# Posting policy

Nothing is pushed here until it passes this gate. The repository is **public**: every file is
readable by strangers, search engines and rights holders, so write for that audience.

## Allowed

- Our original code: probes, VM automation, watcher, build scripts.
- Patches against upstream GPL-2.0 projects, as patches (not as a vendored fork).
- Method documents: verification levels, failure taxonomy, measurement rules, race kits.
- Evidence summaries that describe *observations*: "call X returned E_NOTIMPL at RVA Y".
- Hashes of our own artefacts (binaries we built, captures we produced).
- Names of public projects and public commits.

## Not allowed

- Game binaries, game assets, extracted textures/shaders/audio, keys, decryption material.
- The leaked package, its torrent/magnet, or any mirror link.
- Hashes of the leaked package or of the game executable.
- Instructions for obtaining the game or bypassing DRM/anti-tamper.
- Decompiled or disassembled output of third-party binaries: conclusions and RVAs only.
- Credentials, tokens, VM passwords, local absolute paths, or machine names.
- Third-party code without a compatible licence (TomBox has none; Leonida GPL-3; PercyStar
  non-commercial; ScarlettX none).
- Claims without evidence. If it is "reported", say so; if it is ours, attach the log or hash.

## Verification gate before any commit

1. `git status` — inspect every file.
2. Scan for secrets: `password`, `token`, `gho_`, `ssh-`, `PRIVATE KEY`, `C:\Users\`, `/home/`.
3. Scan for binaries: `*.exe *.dll *.zip *.bin *.rpf *.ttpf`.
4. Scan for game references that would imply redistribution: torrent, magnet, scene release names.
5. Re-read the diff, not the summary.
6. Run `git diff --stat` and confirm the change set matches the intent.

A commit that fails any check is not pushed. Fix, re-verify, then commit.
