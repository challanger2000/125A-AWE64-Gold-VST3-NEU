"""Prepare one fresh, pinned, private DOSBox source tree. No owner media."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

PINNED = "f3a95b9f19a7f9863295e6481f271cf04733c635"

def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args],
                                   text=True).strip()

def prepare(source, private_root, patcher, pinned=PINNED):
    source = Path(source).resolve(strict=True)
    patcher = Path(patcher).resolve(strict=True)
    private_root = Path(private_root).absolute()
    if private_root.is_symlink() or (private_root.exists() and
        getattr(private_root.stat(follow_symlinks=False), "st_file_attributes", 0) & 0x400):
        raise ValueError("Refusing redirected PRIVATE root")
    private_root.mkdir(parents=True, exist_ok=True)
    if git(source, "rev-parse", "HEAD") != pinned:
        raise ValueError("Unexpected original DOSBox HEAD")
    # Never reuse a previous patched or built tree. Git clone reads committed
    # objects, not modified files in the owner's original checkout.
    run = Path(tempfile.mkdtemp(prefix="working-upstream-", dir=private_root))
    target = run / "dosbox-x-AWE32"
    subprocess.run(["git", "clone", "--local", "--no-hardlinks", "--no-checkout",
                    str(source), str(target)], check=True, stdout=sys.stderr)
    subprocess.run(["git", "-C", str(target), "checkout", "--detach", pinned],
                   check=True, stdout=sys.stderr)
    if git(target, "rev-parse", "HEAD") != pinned or git(
            target, "status", "--porcelain", "--untracked-files=all"):
        raise ValueError("Fresh original DOSBox checkout is not clean and pinned")
    before = hashlib.sha256(patcher.read_bytes()).hexdigest()
    subprocess.run([sys.executable, str(patcher), str(target)], check=True,
                   stdout=sys.stderr)
    if hashlib.sha256(patcher.read_bytes()).hexdigest() != before:
        raise ValueError("Source patch changed during preparation")
    diff = subprocess.check_output(["git", "-C", str(target), "diff", "--binary", "HEAD"])
    if not diff:
        raise ValueError("Current source patch made no tracked changes")
    (run / "source-provenance.json").write_text(json.dumps({
        "upstream_commit": pinned, "patcher_sha256": before,
        "patched_diff_sha256": hashlib.sha256(diff).hexdigest()
    }, indent=2) + "\n", encoding="utf-8")
    return target

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("private_root", type=Path)
    parser.add_argument("patcher", type=Path)
    args = parser.parse_args()
    print(prepare(args.source, args.private_root, args.patcher))
