---
name: Bug report
about: A tool, script, patch, or document does not behave as described
title: ""
labels: bug
---

# Bug report

One failure per issue. If several things fail, report the first one and list the rest.

## What you ran

The command or patch as entered, with its arguments:

```
```

## Runtime

| | |
|---|---|
| xcl commit | |
| Branch, if not `main` | |
| Patches applied, by file name and commit id | |

## Hardware and guest

| | |
|---|---|
| GPU and driver version | |
| Guest OS build (`cmd /c ver`) | |
| Guest session (interactive, or SSH) | |
| GPU-PV enabled (`gpuMode=1`) | |

## The failure

| | |
|---|---|
| Failing call or RVA | |
| Return value (`rax`, `HRESULT`, `NTSTATUS`) | |
| Log line, copied verbatim | |
| Log file name and SHA-256 | |

State the image base if you give an RVA.

## Reproduction

- [ ] Reproduces every run
- [ ] Reproduces sometimes (how many runs out of how many)
- [ ] Seen once, could not repeat

Steps from a clean state:

1.
2.

## Do not attach

No game files, executables, decryption keys, or dumps. That covers `.exe`, `.dll`, `.rpf`, `.ttpf`,
`.xvd`, and any capture that contains game bytes. Quote the failing log lines and give a hash of the
file instead. Attachments of this kind are removed and the issue is closed.
