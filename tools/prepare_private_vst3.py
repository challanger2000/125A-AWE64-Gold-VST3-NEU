#!/usr/bin/env python3
"""Prepare a PRIVATE Windows VST3 integration-test bundle.

Copies an already built original DOSBox-X/VST3 DEV bundle and the owner's
existing licensed assets into the bundle's ROM resources. This script
does NOT download or publish Creative files and MUST NOT run in public CI.
It does not promote this DEV build to a release or prove original AWE sound.
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

REQUIRED = {
    "awe32.raw": ("sha256",
        "4e143b94f758734f594ded78f4e5115635975c16fa37fabeae0baa4938ca710e",
        1_048_576),
    "Driver.exe": ("git-blob",
        "e10b8705c6a46444a998c4f6aa74a1cda2081506", None),
    "SYNTHGM.SF2": ("sha256",
        "78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f",
        42_726),
}
FACEPLATE = ("git-blob",
             "7e57ef8e0c90d84fa4c951bea7815b0233762ca7", None)

def file_hash(path: Path, algorithm: str) -> str:
    if algorithm not in ("sha256", "git-blob"):
        raise ValueError("Unrecognized integrity algorithm")
    size=path.stat().st_size
    h=hashlib.sha256() if algorithm=="sha256" else hashlib.sha1()
    if algorithm=="git-blob":
        h.update(("blob "+str(size)+"\0").encode("ascii"))
    with path.open("rb") as stream:
        for block in iter(lambda:stream.read(256*1024),b""):
            h.update(block)
    return h.hexdigest()

def verify(path: Path, spec):
    algorithm,expected,expected_size=spec
    if not path.is_file() or path.is_symlink():
        raise ValueError("Required regular file missing: "+path.name)
    if expected_size is not None and path.stat().st_size!=expected_size:
        raise ValueError("Wrong size for "+path.name)
    if file_hash(path,algorithm)!=expected:
        raise ValueError("Integrity/hash mismatch: "+path.name)

def assemble(source: Path, assets: Path, destination: Path) -> Path:
    source=source.resolve(strict=True)
    assets=assets.resolve(strict=True)
    if not source.is_dir() or not assets.is_dir():
        raise ValueError("Source bundle and private asset folder must be directories")
    if destination.exists():
        raise FileExistsError("Refuse to overwrite an existing private candidate")
    if source.name.lower()!="125a awe64 gold.vst3":
        raise ValueError("Unexpected VST3 DEV bundle name")
    host=source/"Contents"/"x86_64-win"/"125A AWE64 Gold.vst3"
    core=source/"Contents"/"x86_64-win"/"Engine.dll"
    artwork=source/"Contents"/"Resources"/"125A_P165_Original_Faceplate_1000x708.png"
    for component in (host,core):
        if not component.is_file() or component.is_symlink() or component.stat().st_size<1024:
            raise ValueError("Missing real VST3 host or DOSBox core binary: "+component.name)
    verify(artwork,FACEPLATE)
    for name,spec in REQUIRED.items():
        verify(assets/name,spec)
    for item in source.rglob("*"):
        if item.is_symlink():
            raise ValueError("Refuse VST3 tree containing symbolic links")
    # Privacy fence: only our gitignored PRIVATE directory may receive
    # the licensed original Creative ROM / bank / executable.
    repository=Path(__file__).resolve().parents[1]
    private=repository/"PRIVATE"
    # Refuse a redirected PRIVATE root (including Windows junctions).
    if private.resolve()!=private:
        raise ValueError("Private root must not be redirected")
    output=destination.resolve()
    if output==private or not output.is_relative_to(private):
        raise ValueError("Private output must stay below the gitignored /PRIVATE directory")
    output.parent.mkdir(parents=True,exist_ok=True)
    if private.resolve()!=private or output.parent.resolve()!=output.parent:
        raise ValueError("Private output parent redirected")
    if output.exists():
        raise FileExistsError("Refuse to overwrite an existing private candidate")
    work=Path(tempfile.mkdtemp(prefix=".awe64_private_",dir=output.parent))
    try:
        candidate=work/source.name
        shutil.copytree(source,candidate)
        private_inside=candidate/"Contents"/"Resources"/"ROM"
        private_inside.mkdir(parents=True,exist_ok=True)
        for name,spec in REQUIRED.items():
            shutil.copy2(assets/name,private_inside/name)
            verify(private_inside/name,spec)
        if private.resolve()!=private or output.parent.resolve()!=output.parent:
            raise ValueError("Private output parent redirected during staging")
        if output.exists():
            raise FileExistsError("Refuse to overwrite an existing private candidate")
        candidate.rename(output)
    finally:
        shutil.rmtree(work,ignore_errors=True)
    return output

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--source-bundle",type=Path,required=True)
    parser.add_argument("--private-assets",type=Path,required=True)
    parser.add_argument("--output",type=Path,
        default=Path(__file__).resolve().parents[1]/"PRIVATE"/"125A AWE64 Gold.vst3")
    options=parser.parse_args()
    result=assemble(options.source_bundle,options.private_assets,options.output)
    print("PRIVATE DEVELOPMENT BUNDLE (NO RELEASE, NO CREATIVE SOUND VALIDATION):")
    print(result)
