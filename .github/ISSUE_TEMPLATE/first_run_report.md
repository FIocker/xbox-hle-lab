---
name: First-run report
about: A title ran under xcl and stopped. Record where it stopped.
title: "[first run] "
labels: first-run
---

# First-run report

One title per report. Report the first failure only; later stages belong in their own issue.

## Title

| | |
|---|---|
| Title and publisher | |
| Build or version string | |
| Run from a legal copy, in a GPU-PV VM | |

## Runtime

| | |
|---|---|
| xcl commit | |
| Branch | |
| Patches applied, by file name and commit id | |
| Config (`log_level`, `XCL_*` variables) | |

## Where it stopped

| | |
|---|---|
| First failing call, by name | |
| First failing RVA, with the image base you used | |
| `rax` and return code | |
| Other registers of interest (`rdx`, `r8`) | |
| Last log line before the stop | |

## Import resolution

Counts from `report.json` or the log:

| Module | Resolved | Stubbed |
|---|---|---|
| | | |

Stubs that were called before the failure:

Total stubs remaining:

## What was implemented before this failure

Each patch or HLE change, and what it fixed. Give the run number this report refers to.

-

## Evidence level

- [ ] Verified: I ran this and can attach the log
- [ ] Reported: someone else ran it, source or link below
- [ ] Inferred: reasoning from a log, not directly observed

## Do not attach

No game files, executables, keys, or dumps. Quote log lines rather than attaching a capture that
contains game bytes. Attachments of this kind are removed and the issue is closed.
