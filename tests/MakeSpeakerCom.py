"""Independent 16-bit DOS PC-speaker probe, no Creative proprietary bytes.

Runs OUTs to original emulated PIT channel 2 (ports 43h/42h) and PC speaker
gate (61h), holds ~0.5s via BIOS timer INT 1Ah and disables speaker before
exiting through DOS INT 21h. Audio MUST be produced by original DOSBox-X
hardware/mixer; there is no test tone synthesized by the Python host.

This is a stereo PCM bridge gate, NOT proof of EMU8000 or Creative sounds.
"""
from pathlib import Path


def build_probe():
    b = bytearray()

    def put(*bytes_):
        b.extend(bytes_)

    # Real-mode COM: CS == DS; no generated audio samples in test host.
    put(0x0e, 0x1f)             # push cs; pop ds
    put(0xb0, 0xb6, 0xe6, 0x43)  # PIT control 0xb6 -> OUT 43h
    put(0xba, 0x42, 0x00)       # mov dx,42h
    put(0xb8, 0x98, 0x0a)       # PIT divisor 0x0A98 ~= 440 Hz
    put(0xee, 0x88, 0xe0, 0xee) # write divisor LSB then MSB
    put(0xe4, 0x61, 0x0c, 0x03, 0xe6, 0x61) # enable speaker gate
    put(0xb4, 0x00, 0xcd, 0x1a, 0x89, 0xd3) # BIOS tick -> BX
    wait = len(b)
    put(0x53, 0xb4, 0x00, 0xcd, 0x1a, 0x5b) # preserve BX; INT 1Ah
    put(0x29, 0xda, 0x83, 0xfa, 0x09)      # ticks elapsed >=9 (~0.49 s)?
    put(0x72, 0x00)             # JB back to wait
    delta = wait - len(b)
    if not -128 <= delta <= 127:
        raise AssertionError("8-bit jump offset out of range")
    b[-1] = delta & 255
    put(0xe4, 0x61, 0x24, 0xfc, 0xe6, 0x61) # disable gate, even on return
    put(0xb8, 0x00, 0x4c, 0xcd, 0x21)      # DOS exit code 0
    if len(b) > 128:
        raise AssertionError("unexpectedly large 16-bit speaker program")
    return bytes(b)


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        raise SystemExit("Usage: MakeSpeakerCom.py SPEAKER.COM")
    Path(sys.argv[1]).write_bytes(build_probe())
    print("Original emulated PIT/PC-speaker DOS probe generated; no Creative assets")
