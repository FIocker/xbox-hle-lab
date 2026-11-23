# Contributing

This repository is a lab notebook plus small tools. Help is welcome where it makes a result
repeatable, and refused where it would turn the repository into a distribution channel.

Read `POSTING-POLICY.md` before anything else. It is shorter than this file and covers the same
ground from the publishing side.

## What we accept

- **Patches to xcl**, under `patches/xcl/`, as `git format-patch` output with the base commit
  stated. They must apply with `git am` on that commit and build afterwards.
- **Probe tools** that measure something: GPU capability, memory layout, present behaviour, root
  signature deserialization. Single-file C++ built with `/MT`, no dependencies beyond the Windows
  SDK, so a reader can compile it from the README without a project system.
- **Documentation corrections**: a wrong RVA, a stale commit id, a claim that no longer holds.
  Quote the old text and the corrected text in the PR.
- **Reproducible first-run reports** that follow the issue template. A report that names the commit,
  the RVA, the return code, and the log line is worth more than a paragraph of description.

## What we cannot accept

- Game binaries, game assets, extracted textures, shaders or audio, decryption keys, the leaked
  package, or any link to it.
- Code from projects with an incompatible licence: GPL-3, non-commercial, or no licence at all.
- Credentials, tokens, VM passwords, machine names, or absolute host paths.
- Instructions for obtaining the game or for bypassing DRM or anti-tamper.
- A claim presented without evidence, or an evidence label promoted without new evidence.

## Checks before you open a PR

There is no CI. These are the gates, run from the repository root.

Confirm what is staged:

```powershell
git status --short
git diff --stat
```

Scan for secrets and host paths:

```powershell
Select-String -Path (git ls-files) -Pattern 'password|token|gho_|ssh-|PRIVATE KEY|C:\\Users\\|/home/'
```

Scan for binaries and captures:

```powershell
git ls-files | Select-String -Pattern '\.(exe|dll|zip|bin|rpf|ttpf|xvd|iso)$'
```

Check a patch in a throwaway clone at the base commit you named:

```sh
git am --check /path/to/your.patch
```

Then build it: `cargo build --release` for xcl, or the `cl` line in the README for a probe. Note that
`git am -3` can auto-merge a duplicate function with no conflict marker, so the build is part of the
check, not an optional extra. If your change alters behaviour, include the log before and after.

Re-read the diff itself, not the summary you wrote about it.

## Labelling evidence

Every claim in a PR, issue, or document carries one of three labels.

**Verified**: you ran it, and the log, hash, or measurement is attached. Someone else can repeat it
on the same commit. Example: `XMemVirtualAlloc(0x49400000, type=0x20003000)` returned NULL, with the
log line.

**Reported**: the claim comes from documentation, an upstream commit, or another project, and you
have not reproduced it here. Name the source. Example: the 33-page limit on the page table comes
from GDK console documentation.

**Inferred**: reasoning from evidence, not directly observed. Say what would confirm or falsify it.
Example: the assert follows the GameInput callback with no intervening HLE call, so the title
probably derived the value itself.

A reported claim does not become verified because it is repeated. Keep the label that the evidence
supports.

## Licence of a contribution

Licences apply per scope, and by opening a PR you agree your contribution is offered under the
licence of the paths it touches.

| Paths | Licence |
|---|---|
| `tools/**`, original code | MIT |
| `docs/**`, prose | CC BY 4.0 |
| `patches/xcl/**` | GPL-2.0 |

Anything under `patches/xcl/` is a derivative of xcl, which is GPL-2.0, so it cannot be MIT. Do not
send xcl patches under any other terms.
