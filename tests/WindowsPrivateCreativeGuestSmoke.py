#!/usr/bin/env python3
"""PRIVATE Windows-only integration gate for the original Creative DOS sound path.

Runs the exact original DOSBox-X-AWE32 DLL from a built development VST3 bundle
IN THIS TEST PROCESS with the owner's locally installed ROM, Creative DEMO.EXE
and SF2. Checks original EMU8000 register-write traces, authentic mixer PCM,
real host->MPU-401 MIDI, audio isolation and clean shutdown.

This is an intermediate DLL/guest gate, NOT a VST3-host/Studio-Pro test.
No original Creative bytes, traces, audio, or packed bundles are uploaded.
Public CI must NOT execute this test.
"""
import argparse
import ctypes
from collections import deque
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import threading
import time

ROM_SHA256 = "4e143b94f758734f594ded78f4e5115635975c16fa37fabeae0baa4938ca710e"
CREATIVE_DEMO_GIT_BLOB = "e10b8705c6a46444a998c4f6aa74a1cda2081506"
GM_SF2_SHA256 = "78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f"


def digest(path: Path, git_blob: bool = False) -> str:
    h = hashlib.sha1() if git_blob else hashlib.sha256()
    if git_blob:
        h.update(("blob %d\0" % path.stat().st_size).encode("ascii"))
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(256 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def find_private_bundle(bundle: Path):
    bundle = bundle.resolve(strict=True)
    if bundle.name.lower() != "awe64golddosboxdev.vst3":
        raise ValueError("Expected the single AWE64GoldDOSBoxDev.vst3 development bundle")
    binaries = bundle / "Contents" / "x86_64-win"
    runtime = binaries / "AWE64DOSBoxRuntime.dll"
    plugin = binaries / "AWE64GoldDOSBoxDev.vst3"
    assets = bundle / "Contents" / "Resources" / "PRIVATE"
    required = (
        (assets / "awe32.raw", ROM_SHA256, False, 1_048_576),
        (assets / "DEMO.EXE", CREATIVE_DEMO_GIT_BLOB, True, None),
        (assets / "SYNTHGM.SF2", GM_SF2_SHA256, False, 42_726),
    )
    for path in (plugin, runtime):
        if not path.is_file() or path.stat().st_size < 1024:
            raise ValueError("Missing compiled VST3 bundle binary: " + path.name)
    for path, sha, git_blob, expected_size in required:
        if not path.is_file() or path.is_symlink():
            raise ValueError("Private original asset not found: " + path.name)
        if expected_size is not None and path.stat().st_size != expected_size:
            raise ValueError("Private original asset has wrong size: " + path.name)
        if digest(path, git_blob) != sha:
            raise ValueError("Private original asset hash mismatch: " + path.name)
    if '"' in str(assets) or '"' in str(runtime):
        raise ValueError('Paths containing double quotes are not supported')
    return runtime, assets


def guest_arguments(assets: Path):
    return [
        "AWE64DOSBoxRuntime", "-noconfig", "-silent", "-nogui", "-nomenu",
        "-set", "sblaster sbtype=sbawe",
        "-set", "sblaster awe32ram=28672",
        "-set", "sblaster awe32rom=" + str(assets / "awe32.raw"),
        "-set", "midi mpu401=intelligent",
        "-set", "mixer rate=44100",
        "-c", 'mount c "' + str(assets) + '"',
        "-c", "c:",
        "-c", "DEMO.EXE /SBK:SYNTHGM.SF2",
    ]


def inspect_trace(trace: Path) -> int:
    # Original upstream EMU8000 trace format: <frame> <hexport> <hexvalue>.
    # Exclude reads (which begin with 'R ') and require real register writes.
    if not trace.is_file():
        return 0
    return sum(bool(re.match(r"^\d+ [0-9A-Fa-f]{3,4} [0-9A-Fa-f]{4}$", line))
               for line in trace.read_text(encoding="utf-8", errors="replace").splitlines())


def run_child(bundle: Path, seconds: float):
    if os.name != "nt":
        raise RuntimeError("This test must run on Windows x64")
    dll_file, assets = find_private_bundle(bundle)
    # This environment applies only to the child tester. Never set SDL
    # environment variables in the actual DAW/VST3 product.
    os.environ["SDL_VIDEODRIVER"] = "dummy"
    # Original mixer must choose its own isolated internal SDL2 dummy driver.
    os.environ.pop("SDL_AUDIODRIVER", None)
    with tempfile.TemporaryDirectory(prefix="awe64_private_trace_") as work:
        trace = Path(work) / "emu8k.trace"
        os.environ["EMU8K_TRACE"] = str(trace)
        with os.add_dll_directory(str(dll_file.parent)):
            dll = ctypes.WinDLL(str(dll_file))
            start = dll.AWE64_StartGuest
            start.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]
            start.restype = ctypes.c_int
            stop = dll.AWE64_RequestStop
            stop.argtypes = []
            stop.restype = None
            isolated_audio = dll.AWE64_IsInternalAudioOnly
            isolated_audio.argtypes = []
            isolated_audio.restype = ctypes.c_int

            midi_type = ctypes.CFUNCTYPE(
                ctypes.c_uint, ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_uint)
            pcm_type = ctypes.CFUNCTYPE(
                None, ctypes.c_void_p, ctypes.POINTER(ctypes.c_int16), ctypes.c_uint)
            register_midi = dll.AWE64_HostRegisterMidi
            register_midi.argtypes = [ctypes.c_void_p, midi_type]
            register_midi.restype = None
            register_pcm = dll.AWE64_HostRegisterPCM
            register_pcm.argtypes = [ctypes.c_void_p, pcm_type]
            register_pcm.restype = None

            pending = deque()
            midi_pulled = [0]
            pcm_frames = [0]
            peak_after_midi = [0]
            notes_sent = [False]

            @midi_type
            def read_midi(_context, dst, max_len):
                if not pending or max_len < len(pending[0]):
                    return 0
                message = pending.popleft()
                for i, byte in enumerate(message):
                    dst[i] = byte
                midi_pulled[0] += 1
                return len(message)

            @pcm_type
            def read_pcm(_context, pcm, frames):
                if not pcm or frames == 0:
                    return
                pcm_frames[0] += int(frames)
                # For proof require a nonzero real 16-bit sample after a note
                # entered the emulated MPU queue; never generate test tones.
                if notes_sent[0]:
                    for i in range(int(frames) * 2):
                        val = abs(int(pcm[i]))
                        if val > peak_after_midi[0]:
                            peak_after_midi[0] = val

            register_midi(None, read_midi)
            register_pcm(None, read_pcm)
            args = [s.encode("mbcs") for s in guest_arguments(assets)]
            argv = (ctypes.c_char_p * (len(args) + 1))(*args, None)
            guest_result = []
            thread = threading.Thread(
                target=lambda: guest_result.append(start(len(args), argv)), daemon=True)
            thread.start()

            started = time.monotonic()
            last_note = 0.0
            isolated_seen = False
            try:
                while time.monotonic() - started < seconds:
                    if not thread.is_alive():
                        raise RuntimeError("Original Creative guest or DOSBox returned before audible MIDI")
                    if isolated_audio() == 1:
                        isolated_seen = True
                    elapsed = time.monotonic() - started
                    # Repeat after startup because authentic SF2 loading can
                    # consume variable emulated DOS time. Use CC0, not CC32.
                    if elapsed >= 3 and elapsed - last_note >= 1.3:
                        pending.extend((b"\xB0\x00\x00", b"\xC0\x00",
                                        b"\x90\x3C\x70"))
                        notes_sent[0] = True
                        last_note = elapsed
                    if peak_after_midi[0] >= 128 and midi_pulled[0] >= 3:
                        break
                    time.sleep(.05)
            finally:
                if notes_sent[0]:
                    pending.append(b"\x80\x3C\x00")
                stop()
                thread.join(timeout=18)
            if thread.is_alive():
                raise RuntimeError("Original DOSBox did not terminate its worker within 18 seconds")
            if not isolated_seen:
                raise RuntimeError("DOSBox mixer failed internal SDL2 dummy-only isolation")
            if guest_result != [0]:
                raise RuntimeError("DOSBox/Creative guest failed to exit normally: " + repr(guest_result))
            writes = inspect_trace(trace)
            if writes == 0:
                raise RuntimeError("No actual original EMU8000 register writes in private trace")
            if midi_pulled[0] < 3:
                raise RuntimeError("Original emulated MPU-401 did not consume bank/program/note")
            if pcm_frames[0] == 0:
                raise RuntimeError("No original DOSBox PCM callback frames")
            if peak_after_midi[0] < 128:
                raise RuntimeError("Creative + original ROM/SF2 produced no detectable note PCM")
            print("PRIVATE_CREATIVE_DOS_AWE_PCM_PASS")
            print("Original EMU8000 register writes:", writes)
            print("Original mixer stereo PCM frames:", pcm_frames[0])
            print("Observed note-on PCM peak (16-bit):", peak_after_midi[0])
            print("MPU-401 MIDI packets delivered:", midi_pulled[0])
            print("Original DOSBox SDL2 hardware audio isolation: PASS")
            print("Original DOSBox worker stop and join: PASS")
            print("NOTE: This was the runtime DLL, NOT a complete VST3-host test.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--seconds", type=float, default=30.)
    parser.add_argument("--child", action="store_true", help=argparse.SUPPRESS)
    opt = parser.parse_args()
    if not (5 <= opt.seconds <= 120):
        parser.error("--seconds must be 5..120")
    if opt.child:
        run_child(opt.bundle, opt.seconds)
        return
    # A native crash or hang must not terminate the controlling test process.
    command = [sys.executable, str(Path(__file__).resolve()), "--child",
               "--bundle", str(opt.bundle), "--seconds", str(opt.seconds)]
    completed = subprocess.run(
        command, capture_output=True, text=True, check=False,
        timeout=opt.seconds + 25)
    if completed.returncode != 0 or "PRIVATE_CREATIVE_DOS_AWE_PCM_PASS" not in completed.stdout:
        print(completed.stdout[-3000:], file=sys.stderr)
        print(completed.stderr[-3000:], file=sys.stderr)
        raise SystemExit("PRIVATE_CREATIVE_DOS_AWE_PCM_FAIL")
    print(completed.stdout, end="")


if __name__ == "__main__":
    main()
