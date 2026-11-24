#!/usr/bin/env python3
"""PRIVATE Windows-only integration gate for the original Creative DOS sound path.

Runs the exact original DOSBox-X-AWE32 DLL from a built development VST3 bundle
IN THIS TEST PROCESS with the owner's locally installed ROM, Creative DEMO.EXE
and SF2. Checks original EMU8000 register-write traces, authentic mixer PCM,
real host->MPU-401 MIDI, audio isolation and clean shutdown.

This is an intermediate DLL/guest gate, NOT a VST3-host/Studio-Pro test.
No original Creative bytes, traces, audio, or packed bundles are uploaded.
Only the isolated hash-verified archival-reference CI path may use public archival inputs.
"""
import argparse
from array import array
import math
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


def guest_arguments(assets: Path, ram_dir: Path | None = None):
    args = [
        "AWE64DOSBoxRuntime", "-noconfig", "-nogui", "-nomenu",
        "-set", "sblaster sbtype=sbawe",
        # This is a HARD original-EMU8000 sound test. Exclude other DOSBox
        # hardware voices so that mixer PCM cannot originate from an
        # independent PC-speaker or OPL emulation channel.
        "-set", "speaker pcspeaker=false",
        "-set", "sblaster oplmode=none",
        "-set", "sblaster awe32ram=28672",
        "-set", "sblaster awe32rom=" + str(assets / "awe32.raw"),
        "-set", "midi mpu401=intelligent",
        "-set", "mixer rate=44100",
        "-c", 'mount c "' + str(assets) + '"',
        "-c", "c:",
        "-c", "DEMO.EXE /SBK:SYNTHGM.SF2",
    ]
    if ram_dir is not None:
        args[-2:] = ["-c", 'mount d "' + str(ram_dir) + '"',
                     "-c", r"DEMO.EXE /SBK:D:\RAMTEST.SF2 > D:\RAMBOOT.LOG"]
    return args


def probe_midi_sequence():
    # Exactly the actual original Creative-DOS MIDI controller and note bytes.
    return (bytes((0xB0, 0, 0)), bytes((0xC0, 0)),
            bytes((0x90, 60, 112)))


EMU8000_PORTS = frozenset((0x620, 0x622, 0xA20, 0xA22, 0xE20, 0xE22))
EMU8000_TRACE_WORD = re.compile(r"^\d+ ([0-9A-Fa-f]{3,4}) [0-9A-Fa-f]{4}$")


def inspect_trace(trace: Path) -> tuple[int, int]:
    """Return original EMU8000 register writes and distinct active ports.

    Original upstream format is '<frame> <hex-port> <hex-value>'. Read
    records begin with 'R ' and cannot count. Ignore all non-EMU ports:
    ordinary DOS mixer/MPU/PIT/OPL traffic is NOT an AWE32 sound proof.
    """
    if not trace.is_file():
        return 0, 0
    writes = 0
    ports: set[int] = set()
    with trace.open("r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            match = EMU8000_TRACE_WORD.fullmatch(raw.strip())
            if match:
                port = int(match.group(1), 16)
                if port in EMU8000_PORTS:
                    writes += 1
                    ports.add(port)
    return writes, len(ports)



def summarize_ram_trace(trace: Path):
    """Print aggregate DRAM write evidence; never print trace data or PCM.

    EMU8000 register 1/voice 26 (selector 0x3a), data ports A20/A22
    are SMLD/SMRD. Compare contiguous uploaded words with only our own
    generated PCM, ignoring other hardware initialization traffic.
    """
    from MakeRamSampleSf2 import build
    import struct
    data = build()
    start = data.index(b"smpl")
    size = struct.unpack_from("<I", data, start + 4)[0]
    samples = struct.unpack_from("<" + "H" * (size // 2), data, start + 8)
    expected = (samples[0:100], samples[4046:4106])
    matcher = re.compile(r"^(\d+) ([0-9A-Fa-f]{3,4}) ([0-9A-Fa-f]{4})$")
    selector = -1
    writes = nonzero = note_address_writes = 0
    window = deque(maxlen=100)
    matches = [0, 0]
    addresses = {}
    with trace.open(encoding="utf-8", errors="replace") as f:
        for raw in f:
            m = matcher.fullmatch(raw.strip())
            if not m:
                continue
            frame, port, word = int(m[1]), int(m[2], 16), int(m[3], 16)
            if port == 0xe22:
                selector = word & 255
            elif selector == 0x3a and port in (0xa20, 0xa22):
                writes += 1
                nonzero += word != 0
                window.append(word)
                for i, signature in enumerate(expected):
                    if len(window) >= len(signature) and word == signature[-1]:
                        if tuple(window)[-len(signature):] == signature:
                            matches[i] += 1
            elif 0 <= selector < 32 and port in (0xa20, 0xa22):
                if port == 0xa20:
                    addresses[selector] = word
                elif selector in addresses:
                    address = ((word & 255) << 16) | addresses[selector]
                    if frame > 176400 and address >= 0x200000:
                        note_address_writes += 1
    print(f"RAM_TRACE_DATA_WRITES={writes}; NONZERO={nonzero}; "
          f"MATCH_441={matches[0]}; MATCH_735={matches[1]}; "
          f"LATE_RAM_CCCA_WRITES={note_address_writes}")


# Same nominal schedule as native VST3 host. Wall-clock windows,
# not a sample-aligned or bit-identical oracle.
REFERENCE_BOUNDARIES = tuple(n * 512 / 44100 for n in (800, 880, 980, 1230))


class MixerWindowMetrics:
    def __init__(self):
        self.energy = [0] * 4
        self.samples = [0] * 4
        self.peak = [0] * 4

    def add(self, elapsed, values):
        phase = sum(elapsed >= t for t in REFERENCE_BOUNDARIES[:3])
        if not values:
            return
        self.energy[phase] += sum(v * v for v in values)
        self.samples[phase] += len(values)
        self.peak[phase] = max(self.peak[phase], max(map(abs, values)))

    def report(self):
        for i, name in enumerate(("WARMUP", "IDLE", "HELD_NOTE", "RELEASE")):
            if not self.samples[i]:
                raise RuntimeError("Missing direct mixer window: " + name)
            rms = math.sqrt(self.energy[i] / self.samples[i]) / 32768
            print(f"REFERENCE_{name}_RMS={rms:.9f}")
            print(f"REFERENCE_{name}_FRAMES={self.samples[i] // 2}")
            print(f"REFERENCE_{name}_PEAK={self.peak[i] / 32768:.9f}")


def effect_packets():
    return (bytes.fromhex("f041104212400130050af7"),
            bytes.fromhex("f0411042124001380700f7"))


def effect_trace_summary(trace):
    # The original Creative library uses these INIT1/INIT3 locations.
    # Compare the same late register signatures as the native VST3 gate.
    selected = -1
    counts = [0, 0]
    with trace.open(encoding="utf-8", errors="replace") as f:
        for raw in f:
            match = re.fullmatch(r"(\d+) ([0-9A-Fa-f]{3,4}) ([0-9A-Fa-f]{4})",raw.strip())
            if not match:
                continue
            frame, port, value = int(match[1]),int(match[2],16),int(match[3],16)
            if port == 0xe22:
                selected = value & 255
            elif frame > 176400 and port == 0xa20:
                counts[0] += selected == 0x43 and value == 0xb4ff
                counts[1] += selected == 0x69 and value == 0xe6c0
    print(f"DIRECT_EFFECT_TRACE_PLATE={counts[0]}; FEEDBACK={counts[1]}")
    return all(counts)


def run_child(bundle: Path, seconds: float, reference: bool = False,
              ram_program: int | None = None, parent_work: Path | None = None,
              effect_reference: bool = False):
    if os.name != "nt":
        raise RuntimeError("This test must run on Windows x64")
    dll_file, assets = find_private_bundle(bundle)
    # This environment applies only to the child tester. Never set SDL
    # environment variables in the actual DAW/VST3 product.
    os.environ["SDL_VIDEODRIVER"] = "dummy"
    # Original mixer must choose its own isolated internal SDL2 dummy driver.
    os.environ.pop("SDL_AUDIODRIVER", None)
    from contextlib import nullcontext
    # Windows DOS redirection retains a handle until process exit. Parent
    # owns this directory and deletes it only after the isolated child exits.
    with (nullcontext(str(parent_work)) if parent_work else tempfile.TemporaryDirectory(prefix="awe64_private_trace_")) as work:
        trace = Path(work) / "emu8k.trace"
        ram_dir = None
        if ram_program is not None:
            from MakeRamSampleSf2 import build
            ram_dir = Path(work)
            (ram_dir / "RAMTEST.SF2").write_bytes(build())
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
            note_delivered_to_mpu = [False]
            delivered = []
            metrics = MixerWindowMetrics()
            started = time.monotonic()

            @midi_type
            def read_midi(_context, dst, max_len):
                if not pending or max_len < len(pending[0]):
                    return 0
                message = pending.popleft()
                for i, byte in enumerate(message):
                    dst[i] = byte
                midi_pulled[0] += 1
                if reference:
                    delivered.append(message)
                if message[0] == 0x90:
                    note_delivered_to_mpu[0] = True
                return len(message)

            @pcm_type
            def read_pcm(_context, pcm, frames):
                if not pcm or frames == 0:
                    return
                pcm_frames[0] += int(frames)
                if reference:
                    values = array('h')
                    values.frombytes(ctypes.string_at(pcm, int(frames) * 4))
                    metrics.add(time.monotonic() - started, values)
                # For proof require a nonzero real 16-bit sample after a note
                # entered the emulated MPU queue; never generate test tones.
                if note_delivered_to_mpu[0]:
                    for i in range(int(frames) * 2):
                        val = abs(int(pcm[i]))
                        if val > peak_after_midi[0]:
                            peak_after_midi[0] = val

            register_midi(None, read_midi)
            register_pcm(None, read_pcm)
            args = [s.encode("mbcs") for s in guest_arguments(assets, ram_dir)]
            argv = (ctypes.c_char_p * (len(args) + 1))(*args, None)
            guest_result = []
            thread = threading.Thread(
                target=lambda: guest_result.append(start(len(args), argv)), daemon=True)
            if reference:
                pending.extend((bytes((0xb0, 0, 0)), bytes((0xc0, ram_program or 0))))
            started = time.monotonic()
            thread.start()
            last_note = 0.0
            isolated_seen = False
            sent_off = False
            sent_effects = False
            try:
                while time.monotonic() - started < seconds:
                    if not thread.is_alive():
                        raise RuntimeError("Original Creative guest or DOSBox returned before audible MIDI")
                    if isolated_audio() == 1:
                        isolated_seen = True
                    elapsed = time.monotonic() - started
                    # Repeat after startup because authentic SF2 loading can
                    # consume variable emulated DOS time. Use CC0, not CC32.
                    if reference:
                        if effect_reference and not sent_effects and elapsed >= REFERENCE_BOUNDARIES[0]:
                            pending.extend(effect_packets())
                            sent_effects = True
                        if elapsed >= REFERENCE_BOUNDARIES[1] and not notes_sent[0]:
                            pending.append(bytes((0x90, 60, 100)))
                            notes_sent[0] = True
                        if elapsed >= REFERENCE_BOUNDARIES[2] and not sent_off:
                            pending.append(bytes((0x80, 60, 0)))
                            sent_off = True
                        if elapsed >= REFERENCE_BOUNDARIES[3]:
                            break
                    elif elapsed >= 3 and elapsed - last_note >= 1.3:
                        pending.extend(probe_midi_sequence())
                        notes_sent[0] = True
                        last_note = elapsed
                    if not reference and peak_after_midi[0] >= 128 and midi_pulled[0] >= 3:
                        break
                    time.sleep(.05)
            finally:
                if notes_sent[0] and not sent_off:
                    pending.append(b"\x80\x3C\x00")
                stop()
                thread.join(timeout=18)
            if thread.is_alive():
                raise RuntimeError("Original DOSBox did not terminate its worker within 18 seconds")
            if not isolated_seen:
                raise RuntimeError("DOSBox mixer failed internal SDL2 dummy-only isolation")
            if guest_result != [0]:
                raise RuntimeError("DOSBox/Creative guest failed to exit normally: " + repr(guest_result))
            if ram_program is not None:
                print(f"DIRECT_RAM_PROGRAM={ram_program}")
                summarize_ram_trace(trace)
                boot = ram_dir / "RAMBOOT.LOG"
                if boot.exists():
                    log = boot.read_bytes().upper()
                    print("DIRECT_RAM_CREATIVE_ERROR=" + str(b"ERROR" in log))
            writes, used_ports = inspect_trace(trace)
            if writes < 16 or used_ports < 2:
                raise RuntimeError(
                    f"Insufficient authentic EMU8000 traffic after Creative boot: "
                    f"{writes} writes across {used_ports} actual EMU8000 ports")
            if midi_pulled[0] < 3:
                raise RuntimeError("Original emulated MPU-401 did not consume bank/program/note")
            if pcm_frames[0] == 0:
                raise RuntimeError("No original DOSBox PCM callback frames")
            if reference:
                expected = [bytes((0xb0, 0, 0)), bytes((0xc0, ram_program or 0)),
                            bytes((0x90, 60, 100)), bytes((0x80, 60, 0))]
                if effect_reference:
                    expected[2:2] = list(effect_packets())
                if delivered != expected:
                    raise RuntimeError('Direct mixer did not deliver complete MIDI sequence')
                if effect_reference:
                    print("DIRECT_EFFECT_SYSEX_DELIVERED_PASS")
                    if not effect_trace_summary(trace):
                        raise RuntimeError("Original direct guest effect registers missing")
                metrics.report()
                print('DIRECT_DOSBOX_MIXER_MEASURED; NOT_A_MUSICAL_PASS')
                return
            if peak_after_midi[0] < 128:
                raise RuntimeError("Creative + original ROM/SF2 produced no detectable note PCM")
            print("PRIVATE_CREATIVE_DOS_AWE_PCM_PASS")
            print("Original EMU8000 register writes:", writes)
            print("Distinct original EMU8000 I/O ports:", used_ports)
            print("PC speaker and OPL were disabled during this original-sound probe")
            print("Original mixer stereo PCM frames:", pcm_frames[0])
            print("Observed note-on PCM peak (16-bit):", peak_after_midi[0])
            print("MPU-401 MIDI packets delivered:", midi_pulled[0])
            print("Original DOSBox SDL2 hardware audio isolation: PASS")
            print("Original DOSBox worker stop and join: PASS")
            print("NOTE: This was the runtime DLL, NOT a complete VST3-host test.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path)
    parser.add_argument("--trace-summary", type=Path)
    parser.add_argument("--ram-program", type=int, choices=(0, 17))
    parser.add_argument("--effect-reference", action="store_true")
    parser.add_argument("--seconds", type=float, default=30.)
    parser.add_argument("--child", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--parent-work", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--reference", action="store_true", help="Measure direct mixer without VST3")
    opt = parser.parse_args()
    if opt.trace_summary is not None:
        summarize_ram_trace(opt.trace_summary)
        return
    if opt.bundle is None:
        parser.error("--bundle required")
    if opt.effect_reference and (not opt.reference or opt.ram_program is not None):
        parser.error("--effect-reference requires --reference and no RAM fixture")
    if opt.ram_program is not None and not opt.reference:
        parser.error("--ram-program requires --reference")
    if not (5 <= opt.seconds <= 120):
        parser.error("--seconds must be 5..120")
    if opt.child:
        run_child(opt.bundle, opt.seconds, opt.reference, opt.ram_program, opt.parent_work, opt.effect_reference)
        return
    # A native crash or hang must not terminate the controlling test process.
    command = [sys.executable, str(Path(__file__).resolve()), "--child",
               "--bundle", str(opt.bundle), "--seconds", str(opt.seconds)]
    if opt.reference:
        if opt.seconds < 20:
            parser.error('--reference requires at least 20 seconds')
        command.append('--reference')
    if opt.effect_reference:
        command.append("--effect-reference")
    if opt.ram_program is not None:
        command.extend(('--ram-program', str(opt.ram_program)))
    with tempfile.TemporaryDirectory(prefix="awe64_private_trace_") as work:
        command.extend(("--parent-work", work))
        completed = subprocess.run(
            command, capture_output=True, text=True, check=False,
            timeout=opt.seconds + 25)
    marker = ("DIRECT_DOSBOX_MIXER_MEASURED; NOT_A_MUSICAL_PASS" if opt.reference
              else "PRIVATE_CREATIVE_DOS_AWE_PCM_PASS")
    if completed.returncode != 0 or marker not in completed.stdout:
        print(completed.stdout[-3000:], file=sys.stderr)
        print(completed.stderr[-3000:], file=sys.stderr)
        raise SystemExit("PRIVATE_CREATIVE_DOS_AWE_PCM_FAIL")
    print(completed.stdout, end="")


if __name__ == "__main__":
    main()
