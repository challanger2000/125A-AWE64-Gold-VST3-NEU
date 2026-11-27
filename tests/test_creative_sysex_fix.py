"""Public synthetic test for the one-byte private research repair."""
import unittest
from pathlib import Path
import tempfile
from unittest.mock import patch
import WindowsPrivateCreativeGuestSmoke as harness
from WindowsPrivateCreativeGuestSmoke import repair_sysex_compare, corrected_sysex_copy

class CreativeSysexFixTest(unittest.TestCase):
    def test_exactly_one_byte_and_input_unchanged(self):
        src = bytearray(40197)
        offset = 21834 + 0x35
        # Synthetic representation of the instruction sequence only.
        seq = bytes.fromhex("2e8a04988e460a8bd903df2af6268a173bd0")
        src[offset:offset+len(seq)] = seq
        source = bytes(src)
        fixed = repair_sysex_compare(source)
        self.assertEqual(len(source),len(fixed))
        self.assertEqual([i for i,(a,b) in enumerate(zip(source,fixed)) if a!=b],[offset+3])
        self.assertEqual(source[offset+3],0x98)
        self.assertEqual(fixed[offset+3],0x90)
        with self.assertRaises(ValueError):
            repair_sysex_compare(fixed)
        with self.assertRaises(ValueError):
            corrected_sysex_copy(source)

    def test_corrected_guest_command_targets_staged_image(self):
        with tempfile.TemporaryDirectory(prefix="AWE test ") as tmp:
            root = Path(tmp)
            assets = root / "ROM"
            assets.mkdir()
            work = root / "work dir"
            work.mkdir()
            original = b"synthetic original"
            fixed = b"synthetic corrected"
            (assets / "Driver.exe").write_bytes(original)
            initial = harness.guest_arguments(assets)
            with patch.object(harness, "corrected_sysex_copy", return_value=fixed) as fix:
                command = harness.stage_corrected_sysex_guest(assets, work, initial)
            fix.assert_called_once_with(original)
            self.assertEqual(command[-4:], ["-c", 'mount d "' + str(work) + '"',
                            "-c", r"D:\FXDEMO.EXE /SBK:C:\SYNTHGM.SF2"])
            guest_name = command[-1].split()[0][3:]
            self.assertEqual((work / guest_name).read_bytes(), fixed)
            self.assertEqual((assets / "Driver.exe").read_bytes(), original)
            self.assertEqual(sorted(p.name for p in work.iterdir()), ["FXDEMO.EXE"])

if __name__ == "__main__":
    unittest.main()
