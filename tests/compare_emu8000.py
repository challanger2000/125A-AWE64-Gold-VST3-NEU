#!/usr/bin/env python3
"""Compare patched original EMU8000 kernels on Linux using synthetic inputs."""
import argparse
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import re
import shlex
from statistics import median
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CORE = "src/hardware/emu8k"
REFERENCE = "609ab17525a986c2c4a55106724e87b3ec098764"
OLD_UPSTREAM = "f3a95b9f19a7f9863295e6481f271cf04733c635"
NEW_UPSTREAM = "81d0fde73917b57a1716e610dec2605c36def555"
COMPILER_FLAGS = ["-std=gnu17", "-O2", "-fwrapv", "-ffp-contract=off", "-DAWE64_VST3_RUNTIME"]


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args])


def require_equal(label, reference, actual):
    if reference != actual:
        raise RuntimeError(f"{label}: PCM differs")


def extract(upstream, revision, dest):
    dest.mkdir()
    archive = git(upstream, "archive", revision, CORE)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tree:
        tree.extractall(dest, filter="data")
    return dest / CORE


def compile_hosts(directory, upstream, cc, sanitize=False):
    old_core = extract(upstream, OLD_UPSTREAM, directory / "old")
    new_core = extract(upstream, NEW_UPSTREAM, directory / "new")
    old_patcher = {"__name__": "reference_patcher"}
    exec(git(ROOT, "show", f"{REFERENCE}:integration/patch_dosbox_runtime.py"), old_patcher)
    spec = importlib.util.spec_from_file_location(
        "candidate_patcher", ROOT / "integration/patch_dosbox_runtime.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    old_c = old_core / "snd_emu8k.c"
    original = old_c.read_text()
    old_c.write_text(old_patcher["patch_emu_c_init"](original))
    new_c = new_core / "snd_emu8k.c"
    new_c.write_text(module.patch_emu_c_init(new_c.read_text()))
    new_header = new_core / "include/86box/snd_emu8k.h"
    new_header.write_text(module.patch_emu_header(new_header.read_text()))

    # Inject known register patterns from the frozen reference. Recognition
    # of the requested values and the PCM itself are verified independently.
    pattern = (r"static const [^\n]+ \b(?:emu8k_rv_slots|emu8k_rv_parm|"
               r"emu8k_eq_bass_parm|emu8k_eq_treble_parm)\[[^\n]+\n.*?\n\};")
    register_patterns = re.findall(pattern, original, re.DOTALL)
    if len(register_patterns) != 4:
        raise RuntimeError("Reference effect register fixture is incomplete")
    (directory / "emu8k_test_effect_registers.h").write_text("\n".join(register_patterns) + "\n")
    for index, core in enumerate((old_core, new_core)):
        flags = [*COMPILER_FLAGS]
        if index:
            flags.append("-DAWE64_CANDIDATE")
        if sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-fno-omit-frame-pointer", "-no-pie"]
        subprocess.run([*cc, *flags, "-I", str(core / "include"), "-I", str(directory),
                        str(core / "snd_emu8k.c"), str(ROOT / "tests/Emu8000ReferenceRender.c"),
                        "-Wl,--wrap=calloc,--wrap=free,--wrap=fclose", "-lm",
                        "-o", str(directory / f"render-{index}")], check=True)


def compare(directory, repeats, seconds):
    env = {key: value for key, value in os.environ.items() if not key.startswith("EMU8K_")}

    def run(index, *args):
        return subprocess.check_output([str(directory / f"render-{index}"), *map(str, args)],
                                       env=env, text=True, timeout=60)

    for index in (0, 1):
        print(run(index, "lifetime").strip())

    # Each entry records voices, filter/attack mode, chunk size and effect preset.
    cases = [(v, mode, 44, -1) for v, mode in
             [(0, 0), (1, 0), (1, 1), (8, 2), (32, 2), (32, 3),
              (1, 4), (8, 4), (32, 4), (8, 5), (32, 5)]]
    cases += [(8, 4, 44, effect) for effect in range(8)]
    cases += [(8, 6, chunk, -1) for chunk in (1, 44, 512)]
    report = []
    port_reference = None
    for voices, mode, chunk, effect in cases:
        times = [[], []]
        expected_pcm = None
        label = f"voices={voices} mode={mode} chunk={chunk} effect={effect}"
        for repetition in range(repeats):
            order = (1, 0) if repetition % 2 else (0, 1)
            for index in order:
                pcm = directory / f"render-{index}.pcm"
                log = run(index, voices, mode, chunk, seconds, effect, 0, 0,
                          pcm if repetition == 0 else "-")
                times[index].append(float(log.split()[0].split("=")[1]))
            if repetition == 0:
                expected_pcm = (directory / "render-0.pcm").read_bytes()
                require_equal(label, expected_pcm, (directory / "render-1.pcm").read_bytes())
        if mode == 6:
            if port_reference is None:
                port_reference = expected_pcm
            require_equal(f"exact register schedule with chunk={chunk}", port_reference, expected_pcm)
        if mode == 5:
            fallback = directory / "fallback.pcm"
            run(1, voices, mode, chunk, seconds, effect, 1, 0, fallback)
            require_equal("optional cache allocation fails", expected_pcm, fallback.read_bytes())
        reference, candidate = map(median, times)
        print(f"BIT_EXACT {label} reference_ms={reference:.3f} candidate_ms={candidate:.3f}")
        report.append({"voices": voices, "mode": mode, "chunk": chunk, "effect": effect,
                       "sha256": hashlib.sha256(expected_pcm).hexdigest(),
                       "reference_ms": times[0], "candidate_ms": times[1]})

    negative = directory / "one-frame-late.pcm"
    run(0, 8, 6, 44, seconds, -1, 0, 1, negative)
    if negative.read_bytes() == port_reference:
        raise RuntimeError("Port timing negative control did not alter PCM")
    print("NEGATIVE_CONTROL_PASS shifting IFATN writes by one frame changes PCM")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream", type=Path, help="local Git checkout with both upstream commits")
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"))
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--seconds", type=int, default=5)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    if args.repeats < 1 or not 2 <= args.seconds <= 60:
        parser.error("requires at least one repeat and 2–60 seconds per render")
    with tempfile.TemporaryDirectory(prefix="awe64-chip-reference-") as temp:
        directory = Path(temp)
        compile_hosts(directory, args.upstream.resolve(strict=True), shlex.split(args.cc), args.sanitize)
        cases = compare(directory, args.repeats, args.seconds)
    if args.output:
        args.output.write_text(json.dumps({"reference_product": REFERENCE,
                                           "reference_upstream": OLD_UPSTREAM,
                                           "candidate_upstream": NEW_UPSTREAM,
                                           "seconds": args.seconds,
                                           "compiler_flags": COMPILER_FLAGS,
                                           "sanitized": args.sanitize,
                                           "cases": cases}, indent=2) + "\n")
    print(f"EMU8000_REFERENCE_PASS {len(cases)} raw stereo PCM cases")


if __name__ == "__main__":
    main()
