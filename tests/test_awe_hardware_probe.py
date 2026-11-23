"""Proprietary-asset-free checks for the original EMU8000 Windows gate."""
import importlib.util
from pathlib import Path
import unittest

path = Path(__file__).resolve().parent / "WindowsAweHardwareSmoke.py"
spec = importlib.util.spec_from_file_location("original_awe_hardware", path)
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class TestOriginalAweHardwareProbe(unittest.TestCase):
    def test_uses_original_awe32_and_builtin_aweutil(self):
        args = probe.original_guest_args(Path("C:/PROBE/awe32.raw"))
        self.assertIn("sblaster sbtype=sbawe", args)
        self.assertIn("sblaster awe32ram=28672", args)
        self.assertIn("sblaster awe32rom=C:/PROBE/awe32.raw", args)
        self.assertIn("midi mpu401=intelligent", args)
        self.assertIn("AWEUTIL /S", args)
        self.assertEqual(args[-1], "exit")
        self.assertFalse(any("86box" in x.lower() for x in args))
        self.assertFalse(any("DEMO.EXE" in x for x in args))

    def test_trace_only_original_awe_hardware_word_writes(self):
        trace = ("# Original trace\\nR 123 620 0001\\n"
                 "123 620 0001\\n124 A20 0400\\n"
                 "125 E20 FFFF\\nR 126 A20 0002\\n"
                 "130 330 1234\\ninvalid\\n")
        records = probe.parse_writes(trace)
        self.assertEqual(records, [
            (123, 0x620, 0x0001),
            (124, 0xA20, 0x0400),
            (125, 0xE20, 0xFFFF),
        ])


if __name__ == "__main__":
    unittest.main()
