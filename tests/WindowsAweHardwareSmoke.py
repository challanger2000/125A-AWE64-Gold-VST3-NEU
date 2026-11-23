#!/usr/bin/env python3
"""Windows original DOSBox-X AWE32 *hardware* gate without proprietary ROM.

This deliberately uses an all-zero disposable 1-MiB ROM, never the user's
Creative ROM and never an actual Creative executable. The original DOSBox-X
AWE32/EMU8000 device must initialize, then its built-in AWEUTIL /S executes
real emulated AWE register writes. We require the original EMU8000 trace.
This gate proves hardware I/O only, NOT playable Creative/AWE sound.
"""
import ctypes
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

REGEX = re.compile(r"^(\\d+) ([0-9A-Fa-f]{3,4}) ([0-9A-Fa-f]{4})$")


def original_guest_args(rom: Path):
    return [
        "AWE64DOSBoxRuntime", "-noconfig", "-silent", "-nogui", "-nomenu",
        "-set", "sblaster sbtype=sbawe",
        "-set", "sblaster awe32ram=28672",
        "-set", "sblaster awe32rom=" + str(rom),
        "-set", "midi mpu401=intelligent",
        "-set", "mixer rate=44100",
        "-c", "AWEUTIL /S", "-c", "exit",
    ]


def parse_writes(text):
    # Trace calls in ORIGINAL hardware/emu8000.cpp, not synthetic registers.
    writes = []
    for line in text.splitlines():
        m = REGEX.fullmatch(line.strip())
        if not m:
            continue
        frame, port, value = int(m[1]), int(m[2], 16), int(m[3], 16)
        if port in (0x620, 0x622, 0xA20, 0xA22, 0xE20, 0xE22):
            writes.append((frame, port, value))
    return writes


def child(dll_file: Path):
    if os.name != "nt":
        raise RuntimeError("Windows original DOSBox-X DLL required")
    with tempfile.TemporaryDirectory(prefix="awe64_zero_rom_probe_") as tmp:
        folder = Path(tmp)
        rom = folder / "awe32.raw"
        trace = folder / "emu8000_registers.txt"
        rom.write_bytes(bytes(1024 * 1024))
        # Tracing stays on the disposable CI worker; no ROM, SF2 or logs
        # containing private material are uploaded to public Actions.
        os.environ["EMU8K_TRACE"] = str(trace)
        os.environ["SDL_VIDEODRIVER"] = "dummy"
        os.environ.pop("SDL_AUDIODRIVER", None)
        with os.add_dll_directory(str(dll_file.parent)):
            dll = ctypes.WinDLL(str(dll_file))
            start = dll.AWE64_StartGuest
            start.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]
            start.restype = ctypes.c_int
            args = [item.encode("mbcs") for item in original_guest_args(rom)]
            argv = (ctypes.c_char_p * (len(args) + 1))(*args, None)
            result = start(len(args), argv)
        if result != 0:
            raise RuntimeError("Real DOSBox-X AWE32 guest returned " + str(result))
        if not trace.is_file():
            raise RuntimeError("No original EMU8000 hardware trace: device may not have initialized")
        records = parse_writes(trace.read_text(encoding="utf-8", errors="replace"))
        if len(records) < 16:
            raise RuntimeError("Real AWEUTIL /S generated insufficient actual EMU8000 writes: " +
                               str(len(records)))
        if len({p for _, p, _ in records}) < 2:
            raise RuntimeError("Original EMU8000 writes did not cover multiple hardware ports")
        print("ORIGINAL_AWE32_HARDWARE_WRITE_GATE_PASS", flush=True)
        print("EMU8000 register writes from real DOSBox-X: " + str(len(records)), flush=True)
        print("PROPRIETARY_ROM_USED=NO (synthetic zero-filled ROM)", flush=True)
        print("ORIGINAL_CREATIVE_SOUND_PROVED=NO", flush=True)


def parent(dll_file: Path):
    proc = subprocess.run(
        [sys.executable, str(Path(__file__).resolve()), "--child", str(dll_file)],
        capture_output=True, text=True, timeout=75, check=False)
    if proc.returncode or "ORIGINAL_AWE32_HARDWARE_WRITE_GATE_PASS" not in proc.stdout:
        print(proc.stdout[-3000:], file=sys.stderr)
        print(proc.stderr[-3000:], file=sys.stderr)
        raise SystemExit("Original DOSBox-X AWE32 hardware register gate FAILED")
    print(proc.stdout, end="")


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] not in ("--child", "--parent"):
        raise SystemExit("Usage: WindowsAweHardwareSmoke.py --parent|--child DLL")
    dll_path = Path(sys.argv[2]).resolve(strict=True)
    if sys.argv[1] == "--child":
        child(dll_path)
    else:
        parent(dll_path)
