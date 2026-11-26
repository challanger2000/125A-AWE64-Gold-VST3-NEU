"""Portable tests for ORIGINAL Creative-DOS startup diagnostics only.

Uses disposable GENERATED text files and Python child processes. No ROM,
SoundFont, Creative executable, or genuine AWE64 PCM is ever touched.
"""
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


source = Path(__file__).resolve().parent / "WindowsRunVst3HostPcmSmoke.py"
spec = importlib.util.spec_from_file_location("owner_awe_host_runner", source)
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class PrivateCreativeBootDiagnosticTests(unittest.TestCase):
    @staticmethod
    def make_bundle(tmp):
        plugin = (Path(tmp) / "AWE64GoldDOSBoxDev.vst3" / "Contents" /
                  "x86_64-win" / "AWE64GoldDOSBoxDev.vst3")
        plugin.parent.mkdir(parents=True)
        plugin.write_bytes(b"generated stand-in, not a VST3")
        log = (plugin.parent.parent / "Resources" / "PRIVATE" /
               "B12AB34C.LOG")
        log.parent.mkdir(parents=True)
        return plugin, log

    def test_fails_immediately_on_real_creative_error_message(self):
        with tempfile.TemporaryDirectory() as tmp:
            plugin, log = self.make_bundle(tmp)
            code = ("from pathlib import Path; import time; "
                    + f"Path({str(log)!r}).write_text('ERROR:  SB AWE32 not detected\\n'); "
                    + "time.sleep(4)")
            with patch.dict(os.environ, {"AWE64_CREATIVE_BOOT_LOG": log.name}):
                with self.assertRaisesRegex(
                        RuntimeError, "SB AWE32 not detected"):
                    runner.run_private_creative_host(
                        [sys.executable, "-c", code], plugin, 15)

    def test_does_not_accept_creative_guest_without_boot_log(self):
        with tempfile.TemporaryDirectory() as tmp:
            plugin, log = self.make_bundle(tmp)
            with patch.dict(os.environ, {"AWE64_CREATIVE_BOOT_LOG": log.name}):
                with self.assertRaisesRegex(RuntimeError, "never created"):
                    runner.run_private_creative_host(
                        [sys.executable, "-c", "print('guest exited')"], plugin, 15)

    def test_refuses_stale_or_non_dos_filename(self):
        with tempfile.TemporaryDirectory() as tmp:
            plugin, log = self.make_bundle(tmp)
            log.write_text("ERROR: stale synthetic startup\n")
            with patch.dict(os.environ, {"AWE64_CREATIVE_BOOT_LOG": log.name}):
                with self.assertRaisesRegex(RuntimeError, "stale log"):
                    runner.run_private_creative_host([], plugin, 15)
            with patch.dict(os.environ,
                            {"AWE64_CREATIVE_BOOT_LOG": "../../private"}):
                with self.assertRaisesRegex(RuntimeError, "fresh 8.3 DOS"):
                    runner.run_private_creative_host([], plugin, 15)

    def test_successful_child_return_is_not_a_creative_audio_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            plugin, log = self.make_bundle(tmp)
            code = ("from pathlib import Path; "
                    + f"Path({str(log)!r}).write_text('Creative DOS banner\\n'); "
                    + "print('process returned')")
            with patch.dict(os.environ, {"AWE64_CREATIVE_BOOT_LOG": log.name}):
                output = runner.run_private_creative_host(
                    [sys.executable, "-c", code], plugin, 15)
                self.assertEqual(output.returncode, 0)
                self.assertIn("process returned", output.stdout)
                self.assertNotIn("PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED",
                                 output.stdout)


class DualCreativeRunnerTests(unittest.TestCase):
    def test_normal_native_stderr_is_not_failure(self):
        code = ("import os,sys; print('LOG: Early LOG Init complete', file=sys.stderr); "
                "print('DUAL_VST3_DIRECTION='+('SECOND_TO_FIRST' if os.environ['AWE64_TEST_DUAL_REVERSE']=='1' else 'FIRST_TO_SECOND')); "
                "print('REAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS')")
        self.assertTrue(runner.run_dual_creative_command([sys.executable, "-c", code]))

    def test_reverse_direction_is_required_in_second_process(self):
        code = ("print('DUAL_VST3_DIRECTION=FIRST_TO_SECOND'); "
                "print('REAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS')")
        with self.assertRaisesRegex(RuntimeError, "SECOND_TO_FIRST"):
            runner.run_dual_creative_command([sys.executable, "-c", code])

    def test_exit_and_exact_unique_marker_are_required(self):
        for code in (
            "print('unrelated output')",
            "print('REAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS extra')",
            "print('REAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS\\nREAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS')",
            "import sys; print('REAL_VST3_DUAL_CREATIVE_PCM_LIFECYCLE_PASS'); sys.exit(1)",
        ):
            with self.subTest(code=code):
                with self.assertRaises(RuntimeError):
                    runner.run_dual_creative_command([sys.executable, "-c", code])


if __name__ == "__main__":
    unittest.main()
