"""Audit owner-only Windows build-and-original-Creative-sound runner.

This unit test never touches Windows binaries or owner ROM/Creative files.
The actual private execution remains a separate locally run acceptance.
"""
from pathlib import Path
import unittest


class PrivateBuildRunnerContract(unittest.TestCase):
    def test_pinned_hardware_and_private_asset_gate(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / "tools" / "Build-And-Test-Private-AWE64.ps1").read_text(
            encoding="utf-8")
        for token in (
            "81d0fde73917b57a1716e610dec2605c36def555",
            "4e143b94f758734f594ded78f4e5115635975c16fa37fabeae0baa4938ca710e",
            "v3.7.12_build_20",
            "AWE64GoldDOSBoxDev",
            "awe64_vst3_host_pcm_smoke",
            "AWE64DOSBoxRuntime.dll",
            "125A AWE64 Gold.vst3",
            "Engine.dll",
            "Run-Private-Creative-AWE-Test.ps1",
            "Release SDL2",
            "Windows x64",
            "Original P165 VST3",
            "PRIVATE ORIGINAL CREATIVE AWE64 VST3 SOUND ACCEPTANCE PASS",
        ):
            self.assertIn(token, source)
        # The actual native host, the original Creative program and user's
        # ROM must run privately on the owner's real Windows x64 machine.
        self.assertIn("powershell.exe -NoProfile -NonInteractive", source)
        self.assertIn("-Bundle $bundle -Rom $romPath -Seconds $Seconds", source)
        self.assertIn("PRIVATE", source)
        self.assertIn("Get-FileHash -Algorithm SHA256", source)
        # Original pinned owner-submodule must never be patched or rebuilt.
        self.assertIn("tools/prepare_private_source.py", source)
        self.assertIn("Join-Path $runRoot 'build-vst3'", source)
        self.assertIn("build-provenance.json", source)
        self.assertIn("source-provenance.json", source)
        self.assertNotIn("AWE64_HostRegisterPCM", source)

    def test_no_auto_overwrite_or_public_rom_upload(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / "tools" / "Build-And-Test-Private-AWE64.ps1").read_text(
            encoding="utf-8")
        lower = source.lower()
        for forbidden in (
            "git push", "git add", "git reset --hard", "git clean",
            "upload-artifact", "upload release", "gh release create",
            "86box", "qemu", "win95", "synthesizer replacement",
        ):
            self.assertNotIn(forbidden, lower)
        self.assertIn("Staged runtime SHA-256 differs from freshly built DLL.", source)
        self.assertIn("Get-FileHash -Algorithm SHA256 -LiteralPath $romPath", source)
        self.assertIn("no release", lower)

    def test_public_ci_parses_scripts_but_never_invokes_private_originals(self):
        root = Path(__file__).resolve().parents[1]
        public = (root / ".github" / "workflows" /
                  "audio-bridge-tests.yml").read_text(encoding="utf-8")
        self.assertIn("test_private_build_runner.py", public)
        self.assertIn("Build-And-Test-Private-AWE64.ps1", public)
        self.assertIn("ParseFile", public)
        self.assertNotIn("powershell.exe -File tools/Build-And-Test-Private-AWE64.ps1",
                         public)


if __name__ == "__main__":
    unittest.main()
