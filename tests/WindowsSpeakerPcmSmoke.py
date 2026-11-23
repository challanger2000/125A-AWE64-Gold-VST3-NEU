#!/usr/bin/env python3
"""Actual non-silent PCM from DOSBox-X, produced by ORIGINAL emulated hardware.

The DOS guest toggles IBM PIT/PC-speaker I/O in a public 16-bit .COM. The
real DOSBox-X mixer (via the original SDL2 dummy clock) must deliver samples
to AWE64_HostRegisterPCM. No tone generator or substitute synth runs in host.

AWE32 is also initialized with a ZERO ROM, but the observed audible samples
come from PC speaker, NOT proven EMU8000/Creative sound. Public CI is safe.
"""
import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time

from MakeSpeakerCom import build_probe


def guest_arguments(folder: Path):
    return [
        "AWE64DOSBoxRuntime", "-noconfig", "-nogui", "-nomenu",
        "-set", "sblaster sbtype=sbawe",
        "-set", "sblaster awe32ram=28672",
        "-set", "sblaster awe32rom=" + str(folder / "awe32.raw"),
        "-set", "midi mpu401=intelligent",
        "-set", "mixer rate=44100",
        "-c", 'mount c "' + str(folder) + '"',
        "-c", "c:",
        "-c", "SPEAKER.COM",
        "-c", "exit",
    ]


def run_guest(dll_path: Path):
    if os.name != "nt":
        raise RuntimeError("The original DOSBox Windows x64 DLL is required")
    with tempfile.TemporaryDirectory(prefix="awe64_dos_speaker_") as temp:
        folder = Path(temp)
        (folder / "awe32.raw").write_bytes(bytes(1024 * 1024))
        (folder / "SPEAKER.COM").write_bytes(build_probe())
        os.environ["SDL_VIDEODRIVER"] = "dummy"
        # Original runtime mixer forces SDL dummy; do not open physical audio.
        os.environ.pop("SDL_AUDIODRIVER", None)
        with os.add_dll_directory(str(dll_path.parent)):
            lib = ctypes.WinDLL(str(dll_path))
            start = lib.AWE64_StartGuest
            start.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]
            start.restype = ctypes.c_int
            is_internal = lib.AWE64_IsInternalAudioOnly
            is_internal.argtypes = []
            is_internal.restype = ctypes.c_int
            pcm_sig = ctypes.CFUNCTYPE(
                None, ctypes.c_void_p, ctypes.POINTER(ctypes.c_int16),
                ctypes.c_uint)
            register = lib.AWE64_HostRegisterPCM
            register.argtypes = [ctypes.c_void_p, pcm_sig]
            register.restype = None
            stats = {"frames": 0, "peak": 0, "nonzero_frames": 0}

            @pcm_sig
            def receive_original_mixer_pcm(_ctx, samples, frames):
                if not samples or not frames:
                    return
                stats["frames"] += frames
                for i in range(frames):
                    l, r = int(samples[2 * i]), int(samples[2 * i + 1])
                    peak = max(abs(l), abs(r))
                    if peak:
                        stats["nonzero_frames"] += 1
                    if peak > stats["peak"]:
                        stats["peak"] = peak

            register(None, receive_original_mixer_pcm)
            args = [x.encode("mbcs") for x in guest_arguments(folder)]
            argv = (ctypes.c_char_p * (len(args) + 1))(*args, None)
            outcome = []
            worker = threading.Thread(
                target=lambda: outcome.append(start(len(args), argv)),
                daemon=True)
            worker.start()
            internal_observed = False
            try:
                deadline = time.monotonic() + 25.
                while worker.is_alive() and time.monotonic() < deadline:
                    if is_internal() == 1:
                        internal_observed = True
                    time.sleep(.015)
                if worker.is_alive():
                    raise RuntimeError("Original DOSBox-X did not exit after DOS speaker COM")
                worker.join(timeout=1)
            finally:
                # Never leave native callback referencing freed Python state.
                # On worker hang, process-isolated parent kills this child.
                if not worker.is_alive():
                    register(None, pcm_sig())
            if outcome != [0]:
                raise RuntimeError("Real original DOSBox-X guest exit: " + repr(outcome))
            if not internal_observed:
                raise RuntimeError("Original SDL2 hardware audio was not isolated to dummy")
            if stats["frames"] < 1024:
                raise RuntimeError("Original DOSBox mixer produced insufficient PCM frames")
            if stats["peak"] < 128 or stats["nonzero_frames"] < 512:
                raise RuntimeError("Original DOSBox PCM had no valid non-silent PC-speaker tone: "
                                   + repr(stats))
            print("ORIGINAL_DOSBOX_NONZERO_PCM_FROM_DOS_HARDWARE_PASS")
            print("ORIGINAL_MIXER_STEREO_FRAMES=" + str(stats["frames"]))
            print("ORIGINAL_MIXER_PEAK_PCM16=" + str(stats["peak"]))
            print("ORIGINAL_MIXER_NONZERO_FRAMES=" + str(stats["nonzero_frames"]))
            print("HARDWARE_AUDIO_DEVICE=dummy")
            print("ORIGINAL_CREATIVE_EMU8000_SOUND_PROVED=NO")
            print("PROPRIETARY_ROM_OR_CREATIVE_BINARY_USED=NO")


def run_parent(dll_path: Path):
    cmd = [sys.executable, str(Path(__file__).resolve()), "--child", str(dll_path)]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=38,
                            check=False)
    if result.returncode or "ORIGINAL_DOSBOX_NONZERO_PCM_FROM_DOS_HARDWARE_PASS" not in result.stdout:
        print(result.stdout[-3500:], file=sys.stderr)
        print(result.stderr[-3500:], file=sys.stderr)
        raise SystemExit("Original DOSBox hardware-to-host non-silent PCM test failed")
    print(result.stdout, end="")


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] not in ("--child", "--parent"):
        raise SystemExit("Usage: WindowsSpeakerPcmSmoke.py --parent|--child original-DLL")
    path = Path(sys.argv[2]).resolve(strict=True)
    if sys.argv[1] == "--parent":
        run_parent(path)
    else:
        run_guest(path)
