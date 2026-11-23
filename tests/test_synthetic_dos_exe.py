"""Public synthetic fixtures for actual Windows-VST3 host PCM verification.
NO actual Creative DEMO.EXE/SF2 or copyrighted AWE ROM is ever used in CI.
"""
import struct
import tempfile
from pathlib import Path
import unittest

from MakeSyntheticDemoExe import build_test_mz_exe, stage_fixture


class SyntheticDosExeTests(unittest.TestCase):
    def test_true_mz_exe_with_original_pc_speaker_io(self):
        binary = build_test_mz_exe()
        self.assertEqual(binary[:2], b"MZ")
        self.assertEqual(len(binary), 202)
        magic, last, pages, reloc, hdrpara, minalloc, maxalloc, ss, sp = (
            struct.unpack_from("<9H", binary, 0))
        self.assertEqual(magic, 0x5a4d)
        self.assertEqual(last, len(binary))
        self.assertEqual(pages, 1)
        self.assertEqual(reloc, 0)
        self.assertEqual(hdrpara, 4)
        self.assertGreaterEqual(minalloc, 0x1000)
        self.assertEqual(ss, 0)
        self.assertEqual(sp, 0xfffe)
        code = binary[hdrpara * 16:]
        self.assertIn(bytes((0xe6, 0x43)), code)
        self.assertIn(bytes((0xe6, 0x61)), code)
        self.assertIn(bytes((0x83, 0xfa, 0x24)), code)
        self.assertIn(bytes((0xb8, 0, 0x4c, 0xcd, 0x21)), code)
        # The DOS speaker stays OFF until real MIDI has been read on 330h:
        # UART 3Fh + FE ACK + status90/note60/velocity100, then speaker I/O.
        self.assertIn(bytes((0xBA, 0x31, 0x03, 0xB0, 0x3F, 0xEE)), code)
        self.assertIn(bytes((0x3C, 0xFE)), code)
        self.assertIn(bytes((0x3C, 0x90)), code)
        self.assertIn(bytes((0x3C, 0x3C)), code)
        self.assertIn(bytes((0x3C, 0x64)), code)
        self.assertIn(bytes((0xBA, 0x30, 0x03, 0xEC)), code)
        self.assertIn(bytes((0xF9, 0x5B, 0xC3)), code)
        self.assertNotIn(b"Creative", binary)

    def test_ci_script_does_not_overwrite_powershell_builtin_host_variable(self):
        import re
        workflow = (Path(__file__).resolve().parents[1] /
                    ".github" / "workflows" / "windows-dosbox-runtime.yml"
                    ).read_text(encoding="utf-8")
        # PowerShell $Host is a READ-ONLY automatic variable, including
        # lower-case $host. The prior Windows run failed BEFORE VST3 testing.
        self.assertIsNone(re.search(r"\$host\s*=", workflow, re.IGNORECASE))
        self.assertIn("--exe $hostTestExe --plugin $plugin", workflow)
        self.assertIn("actions/cache/restore@v4", workflow)
        self.assertIn("actions/cache/save@v4", workflow)

    def test_stage_fixture_rejects_existing_owner_data(self):
        with tempfile.TemporaryDirectory() as work:
            bundle = Path(work) / "AWE64GoldDOSBoxDev.vst3"
            native = bundle / "Contents" / "x86_64-win"
            native.mkdir(parents=True)
            (native / "AWE64GoldDOSBoxDev.vst3").write_bytes(b"fixture VST3 stub")
            (native / "AWE64DOSBoxRuntime.dll").write_bytes(b"fixture DLL stub")
            stage_fixture(bundle)
            private = bundle / "Contents" / "Resources" / "PRIVATE"
            self.assertEqual((private / "awe32.raw").stat().st_size, 1048576)
            self.assertEqual((private / "DEMO.EXE").read_bytes(), build_test_mz_exe())
            with self.assertRaises(FileExistsError):
                stage_fixture(bundle)
            self.assertTrue((private / "SYNTHGM.SF2").is_file())


if __name__ == "__main__":
    unittest.main()
