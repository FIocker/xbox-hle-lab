"""Create the isolated <VM> Windows VM through App Sandbox's local API.

No network, no clipboard/folder sharing, GPU assigned later (after guest updates).
Credentials are generated at runtime and written to $LAB/vm/appsandbox-vm.json, which is
git-ignored. They are never printed.
"""

import json
import os
import secrets
import sys
from pathlib import Path

LAB = Path(os.environ.get("LAB", r"."))
API_DIR = LAB / "tools/appsandbox/extracted/headless-api"
sys.path.insert(0, str(API_DIR))
import asb  # noqa: E402

ISO = str(LAB / "iso/Windows.iso")
OUT = LAB / "vm/appsandbox-vm.json"

c = asb.connect()
existing = c.list()
print("existing VMs:", json.dumps(existing)[:300])

name = "<VM>"
password = secrets.token_urlsafe(18)

code, body = c.create(
    name=name,
    osType="windows11",
    imagePath=ISO,
    ramMb=8192,
    cpuCores=8,
    hddGb=128,
    gpuMode=0,
    networkMode=0,
    adminUser="<GUESTUSER>",
    adminPass=password,
    sshEnabled=True,
    sshDeployKey=True,
)
print("create:", code, json.dumps(body)[:300])
if code != 202:
    sys.exit(1)

vm_id = body["id"]
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps({
    "id": vm_id,
    "name": name,
    "adminUser": "<GUESTUSER>",
    "adminPass": password,
    "iso": ISO,
    "networkMode": 0,
    "gpuMode": 0,
}, indent=2), encoding="utf-8")
print("credentials written to", OUT)

print("waiting for the guest to come online (up to 40 min)...")
status = c.wait_online(vm_id=vm_id, timeout=2400)
print("wait_online:", json.dumps(status)[:500])
