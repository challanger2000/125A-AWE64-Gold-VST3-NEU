import pathlib,unittest,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"integration"))
import patch_dosbox_runtime as patch
class BootPolicy(unittest.TestCase):
 def test_clock_restore_precedes_ready_and_midi(self):
  self.assertIn("AWE64_RawGuestMidiReady()&&AWE64_BootClockSettled()",patch.MPU_BRIDGE)
  src="static Bitu Normal_Loop(void) {\n                GFX_Events();\n                if (DOSBox_Paused() == false && ticksRemain > 0) {"
  generated=patch.patch_dosbox_clock(src)
  self.assertLess(generated.index("AWE64_RestoreBootClock();"),generated.index("const int hostTick="))
  self.assertLess(generated.index("CPU_CycleMax=3000;"),generated.index("awe64_boot_clock_pending.store(false"))
  self.assertIn("if(previous==24000)CPU_CycleLeft/=8;",generated)
 def test_host_timeouts_and_authentic_only(self):
  h=(ROOT/"src/DosBoxRuntimeHost.hpp").read_text()
  self.assertIn("boostBankLoad_=authenticGuest_&&",h)
  self.assertIn("bootTimeoutSeconds_=boostBankLoad_?180u:30u;",h)
  self.assertEqual(h.count("std::chrono::seconds(bootTimeoutSeconds_)"),2)
  self.assertIn("std::chrono::seconds(engine.bootTimeoutSeconds())",(ROOT/"src/DosBoxVst3Shell.cpp").read_text())
  self.assertIn('config -set \\"cpu cycles=fixed 24000\\"',h)
if __name__=="__main__":unittest.main()
