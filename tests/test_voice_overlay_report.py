"""Portable regression for numeric native output; synthetic evidence only."""
import unittest
from WindowsRunVst3HostPcmSmoke import validate_voice_overlay_report, validate_voice_profiles


class VoiceOverlayReportTest(unittest.TestCase):
    def report(self):
        return (
            ["FILTER_PRE_NOTE_SILENCE RMS=0.00018 WINDOWS=3 PASS"] * 9
            + [f"VOICE_PCM_RESULT ID={i} MID_DELTA=0.001 MAX_DELTA=0.002 "
               f"NEUTRAL_DELTA=0.0001 THRESHOLD=0.0002 PROGRAM={17 if 1105<=i<=1107 else 0} PASS"
               for i in range(1100,1110)]
            + ["REAL_VST3_VOICE_PCM_SUMMARY=10/10 NEUTRAL_RESTORE=PASS",
               "REAL_VST3_VOICE_CONTROLS_PCM_PASS"]
        )

    def test_native_crlf_and_lf(self):
        for sep in ("\r\n", "\n"):
            validate_voice_overlay_report(sep.join(self.report())+sep)

    def test_missing_duplicate_failed_wrong_id_and_summary(self):
        good = self.report()
        for bad in (
            good[:9]+good[10:],
            good[:10]+[good[9]]+good[10:],
            good[:9]+[good[9].removesuffix("PASS")+"FAIL"]+good[10:],
            good[:9]+[good[9].replace("1100","1110")]+good[10:],
            good[:-2]+["REAL_VST3_VOICE_PCM_SUMMARY=9/10 NEUTRAL_RESTORE=PASS",good[-1]],
            good[:-1],
            good+[good[-1]],
            good[1:],
            good[:1]+good,
            [good[0].removesuffix("PASS")+"FAIL"]+good[1:],
        ):
            with self.subTest(last=bad[-2:]):
                with self.assertRaises(RuntimeError):
                    validate_voice_overlay_report("\r\n".join(bad)+"\r\n")

    def test_false_pass_numeric_counterexamples(self):
        good = self.report()
        for index, old, new in (
            (9, "MID_DELTA=0.001", "MID_DELTA=0"),
            (9, "MAX_DELTA=0.002", "MAX_DELTA=0.0002"),
            (9, "NEUTRAL_DELTA=0.0001", "NEUTRAL_DELTA=0.002"),
            (9, "THRESHOLD=0.0002", "THRESHOLD=0"),
            (9, "MID_DELTA=0.001", "MID_DELTA=1e999"),
            (9, "MID_DELTA=0.001", "MID_DELTA=nan"),
            (9, "MID_DELTA=0.001", "MID_DELTA=-1"),
            (9, "MID_DELTA=0.001", "UNMEASURED"),
            (9, "PROGRAM=0", "PROGRAM=17"),
            (14, "PROGRAM=17", "PROGRAM=0"),
            (0, "RMS=0.00018", "RMS=0.02"),
            (0, "RMS=0.00018", "RMS=1e999"),
            (0, "WINDOWS=3", "WINDOWS=1"),
            (0, "WINDOWS=3", "WINDOWS=101"),
        ):
            with self.subTest(index=index, new=new):
                bad = good.copy()
                self.assertIn(old, bad[index])
                bad[index] = bad[index].replace(old, new)
                with self.assertRaises(RuntimeError):
                    validate_voice_overlay_report("\n".join(bad)+"\n")


class VoiceProfileTest(unittest.TestCase):
    def test_exact_profiles(self):
        def row(i,v,p):
            return f"VOICE_PROFILE ID={i} VALUE={v} PROGRAM={p} RMS="+",".join(["0.01"]*30)+" DIFF="+",".join(["0.002"]*30)+" HARMONICS="+",".join(["0.003"]*240)
        rows=[row(i,v,17 if 5<=i<=7 else 0) for i in range(10) for v in (.5,1.)]
        rows += [row(-1,0.,p) for p in (0,17) for _ in range(3)]
        validate_voice_profiles("\n".join(rows))
        for bad in (rows[:-1],rows+[rows[0]], [rows[0].replace("0.01","nan",1)]+rows[1:],
                    [rows[0].replace("PROGRAM=0","PROGRAM=17")]+rows[1:],
                    [rows[0].rsplit(",",1)[0]]+rows[1:]):
            with self.assertRaises(RuntimeError):
                validate_voice_profiles("\n".join(bad))

if __name__ == "__main__":
    unittest.main()
