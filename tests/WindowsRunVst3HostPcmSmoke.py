#!/usr/bin/env python3
"""Run the ACTUAL compiled Windows VST3-host PCM gate in an isolated process.

Only synthetic (non-Creative) DOS fixtures. No proprietary AWE ROM or private
owner assets are ever uploaded to the public GitHub Actions worker.
"""
import argparse
from pathlib import Path
import subprocess
import sys


def run_native_host(exe: Path, vst3: Path, original: bool = False,
                    seconds: int = 45):
    if not (15 <= seconds <= 120):
        raise ValueError("Original-Creative native test seconds must be 15..120")
    command = [str(exe.resolve(strict=True)), str(vst3.resolve(strict=True))]
    if original:
        command.extend(["--original", str(seconds)])
    proc = subprocess.run(command, timeout=(seconds+30 if original else 40),
                          text=True, capture_output=True, check=False)
    print(proc.stdout[-5000:], end="")
    print(proc.stderr[-5000:], end="", file=sys.stderr)
    expected = ("PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED" if original
                else "SYNTHETIC_DOS_FIXTURE_REAL_VST3_HOST_PCM_PASS")
    if proc.returncode or expected not in proc.stdout:
        raise RuntimeError("Actual Windows VST3 factory/host process() PCM test FAILED")
    def count(label: str) -> int:
        lines = [line.removeprefix(label) for line in proc.stdout.splitlines()
                 if line.startswith(label)]
        if len(lines) != 1:
            raise RuntimeError("Missing/ambiguous native VST3 report: " + label)
        return int(lines[0])
    if count("VST3_HOST_NOTE_ON_EVENTS_SENT=") < 1:
        raise RuntimeError("No actual VST3 MIDI NoteOn through Steinberg IEventList")
    if original and count("VST3_HOST_NOTE_OFF_EVENTS_SENT=") < 1:
        raise RuntimeError("Original Creative VST3 gate did not send MIDI NoteOff")
    if not original and "ORIGINAL_CREATIVE_EMU8000_KLANG_BEWIESEN=NEIN" not in proc.stdout:
        raise RuntimeError("Synthetic test incorrectly claimed original Creative sound")
    # Even a private host that receives PCM is NOT a complete original
    # EMU8000 sound proof until the separate private trace and SHA checks.
    return True


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--exe", type=Path, required=True)
    p.add_argument("--plugin", type=Path, required=True)
    p.add_argument("--original", action="store_true",
                   help="Private Windows-only SHA-verified owner assets")
    p.add_argument("--seconds", type=int, default=45)
    args = p.parse_args()
    run_native_host(args.exe, args.plugin, args.original, args.seconds)
