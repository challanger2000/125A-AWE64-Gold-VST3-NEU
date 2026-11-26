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

    def test_private_original_creative_mode_never_runs_in_public_ci(self):
        root = Path(__file__).resolve().parents[1]
        native = (root / "tests" / "WindowsVst3HostPcmSmoke.cpp").read_text(
            encoding="utf-8")
        launcher = (root / "tests" / "WindowsRunVst3HostPcmSmoke.py").read_text(
            encoding="utf-8")
        private = (root / "tools" / "Run-Private-Creative-AWE-Test.ps1").read_text(
            encoding="utf-8")
        public = (root / ".github" / "workflows" /
                  "windows-dosbox-runtime.yml").read_text(encoding="utf-8")
        self.assertIn("PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED", native)
        self.assertIn("VST3_HOST_NOTE_OFF_EVENTS_SENT", native)
        self.assertIn("originalSeconds", native)
        self.assertIn("minimumPeak=originalMode ? 0.002f : 0.01f", native)
        self.assertIn('command.extend(["--original", str(seconds)])', launcher)
        self.assertIn("PRIVATE_CREATIVE_VST3_EMU8000_PCM_PASS", private)
        self.assertIn("--original --seconds $Seconds", private)
        self.assertIn("expectedDemoBlob", private)
        self.assertIn("expectedSf2", private)
        self.assertIn("expectedRom", private)
        self.assertIn("ORIGINAL_CREATIVE_EMU8000_KLANG_BEWIESEN=NEIN", native)
        # Only SHA-verified private owner script is allowed to run this mode.
        self.assertNotIn("--original --seconds", public)
        self.assertNotIn("Run-Private-Creative-AWE-Test.ps1 -Bundle", public)

    def test_real_creative_only_mode_excludes_pc_speaker_and_opl_in_vst3(self):
        root = Path(__file__).resolve().parents[1]
        runtime = (root / "src" / "DosBoxRuntimeHost.hpp").read_text(
            encoding="utf-8")
        native = (root / "tests" / "WindowsVst3HostPcmSmoke.cpp").read_text(
            encoding="utf-8")
        private = (root / "tools" / "Run-Private-Creative-AWE-Test.ps1").read_text(
            encoding="utf-8")
        public = (root / ".github" / "workflows" /
                  "windows-dosbox-runtime.yml").read_text(encoding="utf-8")
        # Synthetic public MIDI->PC-speaker->VST3 audio fixture must
        # continue working; disabled outputs appear only after the explicit
        # owner-only variable gate, not in normal DOSBox command arguments.
        initial = runtime.split("argvStorage_={", 1)[1].split("};", 1)[0]
        self.assertNotIn("speaker pcspeaker=false", initial)
        self.assertNotIn("sblaster oplmode=none", initial)
        self.assertIn('std::getenv("AWE64_ORIGINAL_CREATIVE_ONLY")', runtime)
        self.assertIn('std::strcmp(creativeOnly,"1")==0', runtime)
        self.assertIn('argvStorage_.push_back("speaker pcspeaker=false")', runtime)
        self.assertIn('argvStorage_.push_back("sblaster oplmode=none")', runtime)
        # No native owner-mode PCM proof without both alternate DOS audio
        # generators blocked, plus the isolated dummy SDL2 output.
        self.assertIn('std::getenv("AWE64_ORIGINAL_CREATIVE_ONLY")', native)
        self.assertIn('std::getenv("SDL_AUDIODRIVER")', native)
        self.assertIn('std::strcmp(sdl,"dummy")!=0', native)
        self.assertIn('std::strcmp(originalOnly,"1")!=0', native)
        for key, value in (("AWE64_ORIGINAL_CREATIVE_ONLY", "1"),
                           ("SDL_AUDIODRIVER", "dummy")):
            self.assertIn(
                f"[Environment]::SetEnvironmentVariable('{key}', '{value}', 'Process')",
                private)
        self.assertIn("finally {", private)
        self.assertIn("$previousOriginal", private)
        self.assertIn("$previousSdl", private)
        # GitHub public CI still uses only synthetic DOS-PC-speaker audio.
        self.assertNotIn("AWE64_ORIGINAL_CREATIVE_ONLY", public)
        # Actual Windows original-EMU8K DEV bundle must also run a STRICT
        # negative integration test on the public generated fake MZ guest.
        self.assertIn("WindowsVst3CreativeOnlyNegative.py", public)
        anti_spoof = (root / "tests" / "WindowsVst3CreativeOnlyNegative.py").read_text(
            encoding="utf-8")
        self.assertIn('env["AWE64_ORIGINAL_CREATIVE_ONLY"] = "1"', anti_spoof)
        self.assertIn('env["SDL_AUDIODRIVER"] = "dummy"', anti_spoof)
        self.assertIn("Real VST3 host process() emitted only silence", anti_spoof)
        self.assertIn('"peak=0, nonzero=0"', anti_spoof)
        self.assertIn('"--negative"', anti_spoof)
        self.assertIn("if(exclusionMode){", native)
        self.assertIn("if(peak!=0.f || nonzero!=0)", native)
        self.assertIn("!originalMode && !exclusionMode", native)
        self.assertIn("PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED", anti_spoof)

    def test_stage_fixture_rejects_existing_owner_data(self):
        with tempfile.TemporaryDirectory() as work:
            bundle = Path(work) / "125A AWE64 Gold.vst3"
            native = bundle / "Contents" / "x86_64-win"
            native.mkdir(parents=True)
            (native / "125A AWE64 Gold.vst3").write_bytes(b"fixture VST3 stub")
            (native / "Engine.dll").write_bytes(b"fixture DLL stub")
            stage_fixture(bundle)
            private = bundle / "Contents" / "Resources" / "ROM"
            self.assertEqual((private / "awe32.raw").stat().st_size, 1048576)
            self.assertEqual((private / "Driver.exe").read_bytes(), build_test_mz_exe())
            with self.assertRaises(FileExistsError):
                stage_fixture(bundle)
            self.assertTrue((private / "SYNTHGM.SF2").is_file())


if __name__ == "__main__":
    unittest.main()
