#!/usr/bin/env python3
"""Run the ACTUAL compiled Windows VST3-host PCM gate in an isolated process.

Only synthetic (non-Creative) DOS fixtures. No proprietary AWE ROM or private
owner assets are ever uploaded to the public GitHub Actions worker.
"""
import argparse
from pathlib import Path
import subprocess
import sys


def run_native_host(exe: Path, vst3: Path):
    proc = subprocess.run([str(exe.resolve(strict=True)),
                           str(vst3.resolve(strict=True))],
                          timeout=40, text=True, capture_output=True, check=False)
    print(proc.stdout[-5000:], end="")
    print(proc.stderr[-5000:], end="", file=sys.stderr)
    if proc.returncode or "SYNTHETIC_DOS_FIXTURE_REAL_VST3_HOST_PCM_PASS" not in proc.stdout:
        raise RuntimeError("Actual Windows VST3 factory/host process() PCM test FAILED")
    if "VST3_HOST_NOTE_ON_EVENTS_SENT=" not in proc.stdout:
        raise RuntimeError("No actual VST3 MIDI NoteOn routed through Steinberg IEventList")
    if "ORIGINAL_CREATIVE_EMU8000_KLANG_BEWIESEN=NEIN" not in proc.stdout:
        raise RuntimeError("Original Creative sound proof incorrectly claimed")
    return True


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--exe", type=Path, required=True)
    p.add_argument("--plugin", type=Path, required=True)
    args = p.parse_args()
    run_native_host(args.exe, args.plugin)
