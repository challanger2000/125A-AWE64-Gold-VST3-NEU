#!/usr/bin/env python3
"""Generate a PUBLIC-DOMAIN-FREE, freshly synthesized DOS MZ executable fixture.

The fixture is NOT Creative DEMO.EXE. It only exercises original emulated
PC-speaker/PIT registers in DOSBox-X. It is used solely in public CI to
probe the actual compiled VST3's PCM path. No copyrighted ROM, driver, or
SoundFont is supplied. Private production VST3 packages must never use it.
"""
from pathlib import Path
import struct
from MakeSpeakerCom import build_probe


def build_test_mz_exe():
    code = bytearray(build_probe())
    # Keep fixture sounding for ~2 seconds in real DOS, enough to let the
    # independently scheduled VST3 host thread request original PCM frames.
    timer = bytes((0x83, 0xfa, 0x09))
    if code.count(timer) != 1:
        raise ValueError("Unexpected PC-speaker timer fixture")
    code = code.replace(timer, bytes((0x83, 0xfa, 0x24)))
    header = bytearray(64)
    length = len(header) + len(code)
    # MZ EXE with CS:IP=0000:0000, SS:SP=0000:FFFE, no relocations.
    # The code itself uses real DOS INT 1Ah and the hardware IO ports.
    words = [
        0x5A4D, length % 512, (length + 511) // 512,
        0, 4, 0x1000, 0xFFFF, 0, 0xFFFE,
        0, 0, 0, 0x1C, 0
    ]
    struct.pack_into("<14H", header, 0, *words)
    return bytes(header + code)


def stage_fixture(bundle: Path):
    bundle = bundle.resolve(strict=True)
    if bundle.name.lower() != "awe64golddosboxdev.vst3":
        raise ValueError("Refuse fixture outside exact development VST3")
    binary_dir = bundle / "Contents" / "x86_64-win"
    if not (binary_dir / "AWE64GoldDOSBoxDev.vst3").is_file():
        raise FileNotFoundError("Original VST3 binary missing")
    if not (binary_dir / "AWE64DOSBoxRuntime.dll").is_file():
        raise FileNotFoundError("Original DOSBox-X DLL missing")
    folder = bundle / "Contents" / "Resources" / "PRIVATE"
    folder.mkdir(parents=True, exist_ok=True)
    for n in ("awe32.raw", "DEMO.EXE", "SYNTHGM.SF2"):
        if (folder / n).exists():
            raise FileExistsError("Refuse overwrite of ANY private owner file: " + n)
    (folder / "awe32.raw").write_bytes(bytes(1024 * 1024))
    (folder / "DEMO.EXE").write_bytes(build_test_mz_exe())
    (folder / "SYNTHGM.SF2").write_bytes(b"CI FIXTURE; NOT A CREATIVE SOUNDFONT")
    print("SYNTHETIC_DOS_MZ_FIXTURE_ONLY_NO_CREATIVE_DATA")


if __name__ == "__main__":
    import argparse
    p = argparse.ArgumentParser()
    p.add_argument("--bundle", required=True, type=Path)
    a = p.parse_args()
    stage_fixture(a.bundle)
