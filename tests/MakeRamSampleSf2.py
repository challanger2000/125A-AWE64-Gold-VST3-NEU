#!/usr/bin/env python3
"""Self-authored SoundFont 2.00 test fixture, no ROM or third-party samples.

Sparse bank zero: program 0 is 441 Hz, program 17 is 735 Hz, both rooted
at MIDI note 60. The two signatures must come from uploaded EMU8000 RAM.
"""
import argparse
import math
import struct
from pathlib import Path


def chunk(tag, data):
    assert len(tag) == 4
    return tag + struct.pack("<I", len(data)) + data + b"\0" * (len(data) % 2)


def list_chunk(kind, *parts):
    return chunk(b"LIST", kind + b"".join(parts))


def name(value):
    return value.encode("ascii").ljust(20, b"\0")


def phdr(label, program, bag):
    return struct.pack("<20sHHHIII", name(label), program, 0, bag, 0, 0, 0)


def inst(label, bag):
    return struct.pack("<20sH", name(label), bag)


def gen(operator, amount):
    return struct.pack("<HH", operator, amount & 0xffff)


def shdr(label, start, end, ls, le, rate, pitch, sample_type):
    return struct.pack("<20sIIIIIBbHH", name(label), start, end, ls, le,
                       rate, pitch, 0, 0, sample_type)


def build(wet=False):
    samples = []
    headers = []
    for label, period, count in (("RAM 441 Hz", 100, 4000),
                                 ("RAM 735 Hz", 60, 3600)):
        start = len(samples)
        samples.extend(round(14000 * math.sin(2 * math.pi * n / period))
                       for n in range(count))
        headers.append(shdr(label, start, start + count, start + period,
                            start + count - period, 44100, 60, 1))
        samples.extend([0] * 46)
    pcm = struct.pack("<" + "h" * len(samples), *samples)
    info = list_chunk(b"INFO", chunk(b"ifil", struct.pack("<HH", 2, 0)),
                      chunk(b"isng", b"EMU8000\0"),
                      chunk(b"INAM", b"125A RAM sample test\0\0"))
    sdta = list_chunk(b"sdta", chunk(b"smpl", pcm))
    # Deliberately high embedded sends to test whether the original CC91/93
    # zero setting truly suppresses all hardware sends, not merely an offset.
    send = gen(15,1000) + gen(16,1000) if wet else b""
    instruments = 5 if wet else 3
    pdta = list_chunk(
        b"pdta",
        chunk(b"phdr", phdr("RAM 441 Hz", 0, 0) +
              phdr("RAM 735 Hz", 17, 1) + phdr("EOP", 0, 2)),
        chunk(b"pbag", struct.pack("<6H", 0, 0, 1, 0, 2, 0)),
        chunk(b"pmod", bytes(10)),
        chunk(b"pgen", gen(41, 0) + gen(41, 1) + gen(0, 0)),
        chunk(b"inst", inst("RAM 441 Hz", 0) + inst("RAM 735 Hz", 1) + inst("EOI", 2)),
        chunk(b"ibag", struct.pack("<6H", 0, 0, instruments, 0, instruments*2, 0)),
        chunk(b"imod", bytes(10)),
        chunk(b"igen", send + gen(38, -2400) + gen(54, 1) + gen(53, 0) +
              send + gen(38, -2400) + gen(54, 1) + gen(53, 1) + gen(0, 0)),
        chunk(b"shdr", b"".join(headers) +
              shdr("EOS", len(samples), len(samples), len(samples),
                   len(samples), 0, 0, 0)))
    return chunk(b"RIFF", b"sfbk" + info + sdta + pdta)


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    with args.output.open("xb") as fp:
        fp.write(build())
