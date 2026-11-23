"""Push a file into the App Sandbox VM using cmd echo + certutil (no PowerShell, no stdin).

Usage: python push_file_cmd.py <local> <remote e.g. C:/lab/gpu_probe.exe>
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
CHUNK = 3_000  # base64 ~4,000 chars, under cmd's 8,191 limit

local = Path(sys.argv[1])
remote = sys.argv[2].replace("/", "\\")
data = local.read_bytes()
parts = [data[i:i + CHUNK] for i in range(0, len(data), CHUNK)]
c = asb.connect()


def run(command, timeout=60):
    code, opened = c.ssh_open(VM, command=command)
    if code != 200:
        raise RuntimeError(f"ssh_open {code}: {opened}")
    sid = opened["sshSessionId"]
    exit_code, err = None, []
    deadline = time.time() + timeout
    while time.time() < deadline:
        code, r = c.ssh_read(VM, ssh_session_id=sid, timeout_ms=1500)
        if r.get("stderrBase64"):
            err.append(base64.b64decode(r["stderrBase64"]).decode("utf-8", "replace"))
        if r.get("closed") or r.get("exitCode") is not None:
            exit_code = r.get("exitCode")
            break
        time.sleep(0.05)
    c.ssh_close(VM, ssh_session_id=sid)
    return exit_code, "".join(err)


print(f"pushing {len(data)} bytes in {len(parts)} chunks")
run("cmd.exe /c mkdir C:\\lab")
bins = []
for i, chunk in enumerate(parts):
    b64 = base64.b64encode(chunk).decode("ascii")
    b64f = f"C:\\lab\\p{i:03d}.b64"
    bin_ = f"C:\\lab\\p{i:03d}.bin"
    bins.append(bin_)
    code, err = run(f"cmd.exe /c echo {b64}>{b64f}")
    if code != 0:
        print(f"chunk {i} write failed {code} {err[:200]}")
        sys.exit(1)
    code, err = run(f"cmd.exe /c certutil -f -decode {b64f} {bin_} >nul")
    if code != 0:
        print(f"chunk {i} decode failed {code} {err[:200]}")
        sys.exit(1)
    if (i + 1) % 10 == 0:
        print(f"  {i + 1}/{len(parts)}")

join = "cmd.exe /c copy /b " + "+".join(bins) + f" {remote} >nul"
code, err = run(join, timeout=180)
print("join:", code, err[:200])
if code != 0:
    sys.exit(1)
code, err = run(f"cmd.exe /c dir {remote}")
print("dir:", code, err[:200])

