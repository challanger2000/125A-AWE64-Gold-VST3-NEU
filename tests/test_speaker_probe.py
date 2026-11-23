"""Asset-free regression for original DOS hardware -> mixer PCM probe."""
from pathlib import Path
import unittest

from MakeSpeakerCom import build_probe
from WindowsSpeakerPcmSmoke import guest_arguments


class TestOriginalPcmProbe(unittest.TestCase):
    def test_actual_dos_com_emits_pit_and_speaker_port_io(self):
        probe = build_probe()
        self.assertEqual(len(probe), 52)
        self.assertTrue(probe.startswith(bytes((0x0e, 0x1f))))
        # OUT to PIT control, PIT2 low/high, speaker gate on/off.
        self.assertIn(bytes((0xb0, 0xb6, 0xe6, 0x43)), probe)
        self.assertIn(bytes((0xba, 0x42, 0x00, 0xb8, 0x98, 0x0a,
                             0xee, 0x88, 0xe0, 0xee)), probe)
        self.assertIn(bytes((0xe4, 0x61, 0x0c, 0x03, 0xe6, 0x61)), probe)
        self.assertIn(bytes((0xe4, 0x61, 0x24, 0xfc, 0xe6, 0x61)), probe)
        self.assertIn(bytes((0xb4, 0x00, 0xcd, 0x1a)), probe)
        self.assertEqual(probe[-5:], bytes((0xb8, 0, 0x4c, 0xcd, 0x21)))
        self.assertNotIn(b"DEMO.EXE", probe)
        self.assertNotIn(b"SYNTHGM", probe)

    def test_dos_bios_timer_jump_targets_wait_label(self):
        code = build_probe()
        branch = code.index(bytes((0x72, 0xf3)))
        destination = branch + 2 - 13
        self.assertEqual(code[destination:destination + 3], bytes((0x53, 0xb4, 0)))
        self.assertLess(branch + 2, len(code))

    def test_pcm_harness_matches_real_awe32_config_and_is_headless(self):
        arguments = guest_arguments(Path("C:/TEST_DIR"))
        self.assertIn("sblaster sbtype=sbawe", arguments)
        self.assertIn("sblaster awe32ram=28672", arguments)
        self.assertIn("sblaster awe32rom=" + str(Path("C:/TEST_DIR/awe32.raw")),
                      arguments)
        self.assertNotIn("-silent", arguments)
        self.assertIn("-nogui", arguments)
        self.assertIn("-nomenu", arguments)
        self.assertIn("SPEAKER.COM", arguments)
        self.assertEqual(arguments[-1], "exit")
        self.assertNotIn("DEMO.EXE", " ".join(arguments))


if __name__ == "__main__":
    unittest.main()
