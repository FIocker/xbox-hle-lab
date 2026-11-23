"""Run one command in the App Sandbox VM over SSH and print its output.

Usage: python vm_exec.py "<command>" [timeout-seconds]

Reads until the session closes, not just until an exit code is reported, so
buffered stdout is not lost when the guest reports the code before the data.
"""

import base64
import os
import sys
import time
from pathlib import Path

LAB = Path(os.environ.get("LAB", r"."))
API_DIR = LAB / "tools/appsandbox/extracted/headless-api"
sys.path.insert(0, str(API_DIR))
import asb  # noqa: E402

VM = "<VM>"
command = sys.argv[1] if len(sys.argv) > 1 else "cmd /c ver"
timeout = float(sys.argv[2]) if len(sys.argv) > 2 else 120.0

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

c = asb.connect()
code, opened = c.ssh_open(VM, command=command)
if code != 200:
    print("ssh_open failed:", code, opened)
    sys.exit(1)
sid = opened["sshSessionId"]

deadline = time.time() + timeout
out = []
err = []
exit_code = None
quiet = 0
while time.time() < deadline:
    code, r = c.ssh_read(VM, ssh_session_id=sid, timeout_ms=2000)
    if code != 200:
        print("ssh_read failed:", code, r)
        break
    got = False
    for key, sink in (("stdoutBase64", out), ("stderrBase64", err)):
        if r.get(key):
            sink.append(base64.b64decode(r[key]).decode("utf-8", "replace"))
            got = True
    if r.get("exitCode") is not None:
        exit_code = r["exitCode"]
    if got:
        quiet = 0
    else:
        quiet += 1
    if exit_code is not None and (r.get("closed") or quiet >= 3):
        break
    time.sleep(0.05)

c.ssh_close(VM, ssh_session_id=sid)
text = "".join(out)
errtext = "".join(err)
print(text)
if errtext:
    print("--- stderr ---")
    print(errtext)
if exit_code is not None:
    print(f"--- exit {exit_code} ---")
else:
    print("--- no exit code reported (still running or session closed) ---")
