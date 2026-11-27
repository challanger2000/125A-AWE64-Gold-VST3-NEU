#!/usr/bin/env python3
"""Verify a COMPLETE private owner ZIP, offline. No acquisition or asset publication."""
import argparse
import hashlib
import json
from pathlib import PurePosixPath
import zipfile

BUNDLE = "125A AWE64 Gold.vst3/Contents/"
ROM = BUNDLE + "Resources/ROM/"
ORIGINALS = {
    "awe32.raw": (1048576, "sha256", "4e143b94f758734f594ded78f4e5115635975c16fa37fabeae0baa4938ca710e"),
    "Driver.exe": (40197, "git-blob", "e10b8705c6a46444a998c4f6aa74a1cda2081506"),
    "SYNTHGM.SF2": (42726, "sha256", "78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f"),
}

def verify(path, commit):
    with zipfile.ZipFile(path) as archive:
        seen = set()
        for item in archive.infolist():
            name = item.filename
            p = PurePosixPath(name)
            if p.is_absolute() or ".." in p.parts or "\\" in name or ":" in name or name.casefold() in seen:
                raise ValueError(f"unsafe or duplicate entry: {name}")
            seen.add(name.casefold())
        missing = [ROM+n for n in ORIGINALS if ROM+n not in archive.namelist()]
        if missing:
            raise ValueError("Missing original files: " + ", ".join(missing))
        manifest = json.loads(archive.read("BUILD-IDENTITY.json").decode("utf-8-sig"))
        if manifest["product_commit"] != commit:
            raise ValueError("wrong product revision")
        for name,(size,kind,expected) in ORIGINALS.items():
            info = archive.getinfo(ROM+name)
            if info.file_size != size:
                raise ValueError("wrong asset size: "+name)
            b = archive.read(info)
            digest = (hashlib.sha1(f"blob {size}".encode()+bytes([0])+b).hexdigest()
                      if kind=="git-blob" else hashlib.sha256(b).hexdigest())
            if digest != expected:
                raise ValueError("wrong original asset hash: "+name)
        for name in ("125A AWE64 Gold.vst3", "Engine.dll"):
            b = archive.read(BUNDLE+"x86_64-win/"+name)
            if hashlib.sha256(b).hexdigest() != manifest[name]:
                raise ValueError("wrong binary hash: "+name)
        forbidden = {"awe64golddosboxdev.vst3", "awe64dosboxruntime.dll", "demo.exe", "private"}
        for item in archive.infolist():
            if any(part.casefold() in forbidden for part in PurePosixPath(item.filename).parts):
                raise ValueError("obsolete product path: "+item.filename)
        if archive.testzip() is not None:
            raise ValueError("ZIP CRC failure")
    print("COMPLETE_PRIVATE_OWNER_PACKAGE_PASS "+commit)

if __name__ == "__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("zip")
    p.add_argument("--commit",required=True)
    a=p.parse_args()
    verify(a.zip,a.commit)
