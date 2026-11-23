#!/usr/bin/env python3
"""REAL Windows VST3 NEGATIVE CONTROL: PC-speaker sound must be SILENT.

The public fixture contains only our OWN 16-bit DOS PC-speaker executable
and zero ROM. With AWE64_ORIGINAL_CREATIVE_ONLY=1 the original DOSBox-X
speaker and OPL devices must be disabled INSIDE THE REAL VST3. The same
MIDI fixture that produces 979 nonzero PCM frames without the flag must
now emit NO detectable audio. This test is intentionally expected to
return nonzero due to SILENCE, NOT because of a crash, timeout, DLL error
or a missing Steinberg component.

NO original Creative ROM/SF2/DEMO.EXE is ever used in this negative test.
"""
import argparse
import os
from pathlib import Path
import subprocess


def prove_non_creative_silenced(exe: Path, plugin: Path):
    native_exe = exe.resolve(strict=True)
    native_plugin = plugin.resolve(strict=True)
    env = os.environ.copy()
    env["AWE64_ORIGINAL_CREATIVE_ONLY"] = "1"
    env["SDL_AUDIODRIVER"] = "dummy"
    result = subprocess.run(
        [str(native_exe), str(native_plugin), "--negative"],
        env=env, text=True, capture_output=True, timeout=30, check=False)
    # Exit code 1 is required but INSUFFICIENT: a broken VST3/DLL also
    # returns 1. It must specifically have RUN and reached the sound test
    # before rejecting the fake Creative-tone attempt as SILENCE.
    if (result.returncode != 1 or
            "Real VST3 host process() emitted only silence" not in result.stderr or
            "peak=0, nonzero=0" not in result.stderr):
        raise RuntimeError(
            "Original-only VST3 sound exclusion NEGATIVE CONTROL FAILED: "
            "return=" + str(result.returncode) +
            " stdout=" + result.stdout[-1200:] +
            " stderr=" + result.stderr[-1800:])
    if "PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED" in result.stdout:
        raise RuntimeError("Synthetic DOS speaker was falsely accepted as original Creative")
    print("REAL_VST3_PRIVATE_CREATIVE_ONLY_SILENCES_PC_SPEAKER_PASS")
    print("PUBLIC_SYNTHETIC_ROM_AND_DOS_FIXTURE_ONLY=YES")
    print("ACTUAL_ORIGINAL_CREATIVE_SOUNDFONT_AUDIO_PROVED=NO")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--plugin", required=True, type=Path)
    arguments = parser.parse_args()
    prove_non_creative_silenced(arguments.exe, arguments.plugin)
