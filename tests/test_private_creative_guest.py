"""Portable static and pure-function checks; never use Creative binaries in CI."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

module_path = Path(__file__).resolve().parent / "WindowsPrivateCreativeGuestSmoke.py"
spec = importlib.util.spec_from_file_location("creative_private_smoke", module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class CreativePrivateHarnessTest(unittest.TestCase):
    def test_exact_original_dos_command(self):
        cmd = module.guest_arguments(Path("X:/PRIVATE"))
        self.assertIn("midi mpu401=intelligent", cmd)
        self.assertIn("sblaster sbtype=sbawe", cmd)
        self.assertIn("sblaster awe32ram=28672", cmd)
        self.assertIn("DEMO.EXE /SBK:SYNTHGM.SF2", cmd)
        self.assertNotIn("midi mpu401=uart", cmd)
        self.assertFalse(any("86box" in x.lower() for x in cmd))

    def test_rejects_fake_originals_before_dll_load(self):
        with tempfile.TemporaryDirectory() as t:
            root = Path(t) / "AWE64GoldDOSBoxDev.vst3"
            b = root / "Contents" / "x86_64-win"
            b.mkdir(parents=True)
            (b / "AWE64DOSBoxRuntime.dll").write_bytes(b"X" * 2048)
            (b / "AWE64GoldDOSBoxDev.vst3").write_bytes(b"Y" * 2048)
            a = root / "Contents" / "Resources" / "PRIVATE"
            a.mkdir(parents=True)
            for n in ("awe32.raw", "DEMO.EXE", "SYNTHGM.SF2"):
                (a / n).write_bytes(b"not authentic")
            with self.assertRaises(ValueError):
                module.find_private_bundle(root)

    def test_trace_detects_writes_and_excludes_reads(self):
        with tempfile.TemporaryDirectory() as t:
            trace = Path(t) / "emu.trace"
            trace.write_text("R 123 620 0001\\n123 620 0001\\n124 A20 0400\\n")
            self.assertEqual(module.inspect_trace(trace), 2)


if __name__ == "__main__":
    unittest.main()
