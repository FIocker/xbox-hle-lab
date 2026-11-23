"""Snapshot the clean VM, assign the RTX via GPU-PV, and boot it again."""

import json
import os
import sys
from pathlib import Path

LAB = Path(os.environ.get("LAB", r"."))
API_DIR = LAB / "tools/appsandbox/extracted/headless-api"
sys.path.insert(0, str(API_DIR))
import asb  # noqa: E402

VM = "<VM>"
c = asb.connect()

gpus = c.host()["gpus"]
print("gpus:", json.dumps(gpus))
if not gpus:
    print("no GPU visible to App Sandbox")
    sys.exit(1)
gpu = gpus[0]

code, snap = c.snap_take(VM, "post-install")
print("snapshot:", code, json.dumps(snap)[:300])

print("shutting down...")
print(c.shutdown(VM))
try:
    print("stopped:", c.wait(VM, target_states=["stopped"], timeout=180))
except Exception as e:
    print("wait stopped:", e)

code, edited = c.edit(VM, gpuMode=1, gpuId=gpu["id"])
print("edit:", code, json.dumps(edited)[:300])
if code != 200:
    sys.exit(1)

print("starting with GPU...")
print(c.start(VM))
status = c.wait_online(vm_id=None, name=VM, timeout=300) if False else None
# wait_online takes vm_id in this SDK; resolve it from list()
vm = next(v for v in c.list() if v["name"] == VM)
status = c.wait_online(vm_id=vm["id"], timeout=300)
print("online:", json.dumps(status)[:500])
