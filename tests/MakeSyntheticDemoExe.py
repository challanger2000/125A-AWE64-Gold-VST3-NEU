#!/usr/bin/env python3
"""Generate a PUBLIC-DOMAIN-FREE, freshly synthesized DOS MZ executable fixture.

The fixture is NOT Creative DEMO.EXE. It receives actual VST3 NoteOn over the original MPU-401 and only then
exercises original emulated
PC-speaker/PIT registers in DOSBox-X. It is used solely in public CI to
probe the actual compiled VST3's PCM path. No copyrighted ROM, driver, or
SoundFont is supplied. Private production VST3 packages must never use it.
"""
from pathlib import Path
import struct
from MakeMidiTriggeredDemoExe import build_midi_triggered_exe


def build_test_mz_exe():
    # Force a complete VST3-MIDI -> original MPU-401 -> DOS PC-speaker
    # -> original DOSBox-X mixer -> VST3 process() audio roundtrip.
    # A note is mandatory: mere MIDI output silence cannot pass this gate.
    return build_midi_triggered_exe()


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
