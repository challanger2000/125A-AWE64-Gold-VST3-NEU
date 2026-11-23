#!/usr/bin/env python3
"""Run the ACTUAL compiled Windows VST3-host PCM gate in an isolated process.

Only synthetic (non-Creative) DOS fixtures. No proprietary AWE ROM or private
owner assets are ever uploaded to the public GitHub Actions worker.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import time


def run_private_creative_host(command: list[str], plugin: Path, seconds: int):
    """Monitor Creative's own DOS error output, without publishing assets.

    The original unmodified DOS program prints explicit errors for missing
    AWE32 hardware, failed initialization and SoundFont load. Without DOS
    stdout redirection all three failures look like unhelpful VST3 silence.
    A per-run unique 8.3 log, set only by the private PowerShell launcher,
    also prevents accepting a stale success or a previous test's errors.
    """
    name = os.environ.get("AWE64_CREATIVE_BOOT_LOG", "")
    if not re.fullmatch(r"B[0-9A-F]{7}\.LOG", name):
        raise RuntimeError("Private Creative test requires a fresh 8.3 DOS boot logfile")
    log = plugin.resolve(strict=True).parent.parent / "Resources" / "PRIVATE" / name
    if log.exists():
        raise RuntimeError("Private Creative boot diagnostic already exists; refusing stale log")
    deadline = time.monotonic() + seconds + 30
    process = subprocess.Popen(command, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, text=True, errors="replace")
    try:
        while process.poll() is None and time.monotonic() < deadline:
            if log.is_file():
                # DOS stdout is buffered; errors normally flush as DEMO.EXE
                # exits. Never rely on a non-error startup banner for PASS.
                diagnostic = log.read_bytes()[:32768].decode("cp437", errors="replace")
                errors = [line.strip() for line in diagnostic.splitlines()
                          if line.lstrip().upper().startswith("ERROR:")]
                if errors:
                    raise RuntimeError("Original Creative DOS startup failure: " +
                                       "; ".join(errors)[:1800])
            time.sleep(.2)
        if process.poll() is None:
            raise TimeoutError("Real Creative-DOS VST3 host exceeded private test timeout")
        stdout, stderr = process.communicate(timeout=8)
        if not log.is_file():
            raise RuntimeError("Creative DEMO.EXE never created its DOS stdout boot log")
        final_text = log.read_bytes()[:32768].decode("cp437", errors="replace")
        failures = [line.strip() for line in final_text.splitlines()
                    if line.lstrip().upper().startswith("ERROR:")]
        if failures:
            raise RuntimeError("Original Creative DOS startup failure: " +
                               "; ".join(failures)[:1800])
        return subprocess.CompletedProcess(command, process.returncode, stdout, stderr)
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate(timeout=8)


def run_native_host(exe: Path, vst3: Path, original: bool = False,
                    seconds: int = 45):
    if not (15 <= seconds <= 120):
        raise ValueError("Original-Creative native test seconds must be 15..120")
    command = [str(exe.resolve(strict=True)), str(vst3.resolve(strict=True))]
    if original:
        command.extend(["--original", str(seconds)])
    if original:
        proc = run_private_creative_host(command, vst3, seconds)
    else:
        # Never change the already proven PUBLIC synthetic DOS host path.
        proc = subprocess.run(command, timeout=40, text=True,
                              capture_output=True, check=False)
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
