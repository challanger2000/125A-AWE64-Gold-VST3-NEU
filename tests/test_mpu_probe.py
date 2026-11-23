import unittest
from MakeMpuCom import build_probe
class MpuComTests(unittest.TestCase):
    def test_no_rom_or_creative_assets(self):
        b=build_probe()
        self.assertLess(len(b),512)
        self.assertIn(b'MIDI.BIN\x00',b)
        self.assertEqual(b[-3:],bytes([0,0,0]))
        self.assertEqual(b[:2],bytes([0x0e,0x1f]))
        self.assertIn(bytes([0xba,0x31,0x03]),b)
        self.assertIn(bytes([0xba,0x30,0x03]),b)
        self.assertIn(bytes([0xb0,0x3f,0xee]),b)
        self.assertIn(bytes([0xb8,0x00,0x4c,0xcd,0x21]),b)
        self.assertNotIn(b"DEMO.EXE",b)
if __name__=="__main__":
    unittest.main()
