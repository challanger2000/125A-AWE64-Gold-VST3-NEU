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
import tempfile
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
    # Only the filter isolation suite may need to wait for natural tails.
    extra = 90 if os.environ.get('AWE64_TEST_VOICE_OVERLAYS') == '1' else 30
    deadline = time.monotonic() + seconds + extra
    # DOSBox emits large diagnostic output. Undrained PIPE handles can
    # deadlock the guest or its shutdown before poll() ever sees exit.
    # Use disposable local spools; do not publish their contents as artifacts.
    out = tempfile.TemporaryFile(mode="w+b")
    err = tempfile.TemporaryFile(mode="w+b")
    process = subprocess.Popen(command, stdout=out, stderr=err)
    def tail(stream):
        stream.seek(0, os.SEEK_END)
        stream.seek(max(0, stream.tell()-16384))
        return stream.read().decode("utf-8", errors="replace")
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
            process.kill()
            process.wait(timeout=8)
            print(tail(out), file=sys.stderr)
            print(tail(err), file=sys.stderr)
            raise TimeoutError("Real Creative-DOS VST3 host exceeded private test timeout")
        stdout, stderr = tail(out), tail(err)
        if not log.is_file():
            print(stdout, file=sys.stderr)
            print(stderr, file=sys.stderr)
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
            process.wait(timeout=8)
        out.close()
        err.close()


def validate_voice_overlay_report(stdout: str):
    """Validate native evidence, including Windows CRLF from binary spools."""
    lines = stdout.splitlines()
    if lines.count("REAL_VST3_VOICE_CONTROLS_PCM_PASS") != 1:
        raise RuntimeError("Bundled ten-control musical PCM gate failed")
    if lines.count("REAL_VST3_VOICE_PCM_SUMMARY=10/10 NEUTRAL_RESTORE=PASS") != 1:
        raise RuntimeError("Missing or ambiguous ten-control summary")
    results = [re.fullmatch(r"VOICE_PCM_RESULT ID=(\d+) .* PASS", line)
               for line in lines if line.startswith("VOICE_PCM_RESULT ")]
    if any(m is None for m in results) or sorted(int(m[1]) for m in results) != list(range(1100,1110)):
        raise RuntimeError("Missing or ambiguous per-control PCM evidence")
    silence = [line for line in lines if line.startswith("FILTER_PRE_NOTE_SILENCE ")]
    if len(silence) != 9 or any(not re.fullmatch(r"FILTER_PRE_NOTE_SILENCE .* PASS", line)
                                for line in silence):
        raise RuntimeError("Missing measured silence before a filter trial")


def run_dual_creative_command(command: list[str]):
    # Capture native stderr independently: ordinary DOSBox logs are not
    # PowerShell errors. Only native exit and one exact PASS line count.
    proc = subprocess.run(command, timeout=90, capture_output=True,
                          text=True, encoding="utf-8", errors="replace",
                          check=False)
    marker = "REAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS"
    # Print only test-owned result lines; do not publish native guest logs.
    for line in proc.stdout.splitlines():
        if line.startswith(("DUAL_VST3_", "REAL_VST3_TWO_ACTIVE_COMPONENTS_")):
            print(line, flush=True)
    if proc.returncode or proc.stdout.splitlines().count(marker) != 1:
        raise RuntimeError(f"Dual original VST3 gate FAILED; exit=0x{proc.returncode & 0xffffffff:08x}")
    print(marker, flush=True)
    return True


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
        try:
            proc = subprocess.run(command, timeout=40, text=True,
                                  capture_output=True, check=False)
        except subprocess.TimeoutExpired as exc:
            # Synthetic data only: preserve the last actual native startup
            # and cleanup messages even when the child hits the watchdog.
            for label, stream in (("stdout", exc.stdout), ("stderr", exc.stderr)):
                if isinstance(stream, bytes):
                    stream = stream.decode("utf-8", errors="replace")
                print(f"SYNTHETIC_NATIVE_TIMEOUT {label}:\n{(stream or '')[-12000:]}",
                      file=sys.stderr, flush=True)
            raise
    print(proc.stdout[-5000:], end="")
    print(proc.stderr[-5000:], end="", file=sys.stderr)
    expected = ("PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED" if original
                else "SYNTHETIC_DOS_FIXTURE_REAL_VST3_HOST_PCM_PASS")
    if original and os.environ.get("AWE64_TEST_EXTERNAL_SF2") and \
            "REAL_VST3_EXTERNAL_SF2_STATE_ROUNDTRIP_PASS" not in proc.stdout:
        raise RuntimeError("External SF2 VST3 state roundtrip missing")
    if original and os.environ.get("AWE64_TEST_EXTERNAL_SF2") and \
            "REAL_VST3_ACTIVE_EXTERNAL_SF2_RESTART_PASS" not in proc.stdout:
        raise RuntimeError("Active external SF2 guest restart missing")
    if original and os.environ.get("AWE64_TEST_VOICE_OVERLAYS") == "1":
        validate_voice_overlay_report(proc.stdout)
    if original and os.environ.get("AWE64_TEST_PERFORMANCE") == "1":
        if "PERF_HOST_RATE=44100 " in proc.stdout and "REAL_VST3_SUSTAIN_PCM_PASS" not in proc.stdout:
            raise RuntimeError("Original Creative CC64 sustain PCM gate missing")
        rows = [re.fullmatch(r"PERF_QUEUE_BLOCKS=(\d+) AUDIO_DROPPED_FRAMES=(\d+) MIDI_DROPPED_EVENTS=(\d+) UNDERRUN_FRAMES=(\d+)", line) for line in proc.stdout.splitlines()]
        rows = [match.groups() for match in rows if match is not None]
        if len(rows) != 1:
            raise RuntimeError("Missing or ambiguous measured queue statistics")
        blocks, audio_drop, midi_drop, underrun = map(int, rows[0])
        if not blocks or audio_drop or midi_drop or underrun:
            raise RuntimeError(f"Realtime queue stability failed: audio drops={audio_drop}, MIDI drops={midi_drop}, underrun frames={underrun}")
        if proc.stdout.splitlines().count("REAL_VST3_TRANSPORT_LATENCY_PCM_PASS; QUEUE GATE SEPARATE") != 1:
            raise RuntimeError("Native transport and latency evidence missing")
        if ("PERF_CPU_TIME_NOT_VERIFIED" in proc.stdout.splitlines() or
                proc.stdout.splitlines().count("PERF_CPU_COUNTER_POSITIVE_CONTROL_PASS") != 1 or
                proc.stdout.splitlines().count("PERF_CPU_MEASURE_METHOD=ETW_SCHEDULED_QPC; CI GUEST SCHEDULED TIME") != 1 or
                proc.stdout.splitlines().count("REAL_VST3_PERFORMANCE_MEASUREMENT_PASS; CI CPU NOT OWNER CPU") != 1):
            raise RuntimeError("CPU time measurement NOT VERIFIED; transport/latency gates passed")
    if proc.returncode or expected not in proc.stdout:
        raise RuntimeError(f"Actual Windows VST3 factory/host process() PCM test FAILED; exit=0x{proc.returncode & 0xffffffff:08x}")
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
    p.add_argument("--dual-original", action="store_true")
    args = p.parse_args()
    if args.dual_original:
        if args.original:
            p.error("--original and --dual-original are mutually exclusive")
        run_dual_creative_command([str(args.exe.resolve(strict=True)),
                                   str(args.plugin.resolve(strict=True)),
                                   "--dual-original"])
    else:
        run_native_host(args.exe, args.plugin, args.original, args.seconds)
