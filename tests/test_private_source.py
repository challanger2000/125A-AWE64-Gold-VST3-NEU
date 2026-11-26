"""Execute the actual private source preparation against generated Git data."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("prepare", ROOT / "tools/prepare_private_source.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class FreshSourceTest(unittest.TestCase):
    def test_fresh_clone_and_patch_revision(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            original = base / "original"
            original.mkdir()
            def git(*args):
                return subprocess.check_output(["git", "-C", str(original), *args], text=True).strip()
            git("init")
            git("config", "user.email", "fixture@example.invalid")
            git("config", "user.name", "Fixture")
            tracked = original / "source.txt"
            tracked.write_text("original", encoding="utf-8")
            git("add", "source.txt")
            git("commit", "-m", "fixture")
            sha = git("rev-parse", "HEAD")
            tracked.write_text("owner edit", encoding="utf-8")
            private = base / "PRIVATE"
            patcher = base / "patch.py"
            def write_patch(value):
                patcher.write_text("from pathlib import Path\nimport sys\np=Path(sys.argv[1])/'source.txt'\nassert p.read_text()=='original'\np.write_text("+repr(value)+")\n", encoding="utf-8")
            write_patch("patched A")
            a = module.prepare(original, private, patcher, sha)
            self.assertEqual((a/"source.txt").read_text(), "patched A")
            (a/"source.txt").write_text("stale/tampered")
            write_patch("patched B")
            b = module.prepare(original, private, patcher, sha)
            self.assertNotEqual(a, b)
            self.assertEqual((b/"source.txt").read_text(), "patched B")
            self.assertEqual((a/"source.txt").read_text(), "stale/tampered")
            self.assertEqual(tracked.read_text(), "owner edit")
            record = json.loads((b.parent/"source-provenance.json").read_text())
            self.assertEqual(record["upstream_commit"], sha)
            self.assertEqual(record["patcher_sha256"], hashlib.sha256(patcher.read_bytes()).hexdigest())
            with self.assertRaises(ValueError):
                module.prepare(original, private, patcher, "0"*40)
            patcher.write_text("pass\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "no tracked changes"):
                module.prepare(original, private, patcher, sha)
            patcher.write_text("raise SystemExit(5)\n", encoding="utf-8")
            with self.assertRaises(subprocess.CalledProcessError):
                module.prepare(original, private, patcher, sha)

if __name__ == "__main__":
    unittest.main()
