import unittest
from WindowsResidentBankSweep import validate_report
class ResidentReportTest(unittest.TestCase):
    def test_exact_native_path_and_pcm_sequence(self):
        rows=["AWE64_RESIDENT_BANK_COMMIT"]*3+[
            "RESIDENT_TONE HZ=441 RMS=.02 IDLE=.0001 RATIO=120",
            "RESIDENT_CHANGE INDEX=1 MS=75 STATE=MATCH",
            "RESIDENT_TONE HZ=735 RMS=.03 IDLE=.0001 RATIO=100",
            "RESIDENT_CHANGE INDEX=2 MS=48 STATE=MATCH",
            "RESIDENT_TONE HZ=441 RMS=.02 IDLE=.0001 RATIO=120",
            "REAL_VST3_RESIDENT_SWITCH_PCM_STATE_PASS"]
        self.assertEqual(validate_report("\n".join(rows),True),[75.,48.])
        self.assertEqual(validate_report("\n".join(rows[3:]),False),[75.,48.])
        for bad in (rows[1:],rows[:-1],rows[:5]+[rows[5].replace("735","441")]+rows[6:],
                    rows[:3]+[rows[3].replace(".02","0")]+rows[4:],
                    rows[:4]+[rows[4].replace("MATCH","MISMATCH")]+rows[5:]):
            with self.assertRaises(ValueError):
                validate_report("\n".join(bad),True)
if __name__=="__main__":
    unittest.main()
