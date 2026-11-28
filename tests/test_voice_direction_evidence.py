import copy
import unittest
from VoiceDirectionEvidence import SPECS, directions, validate_direction_report


def fixture():
    def profile(rms=.01, bright=.4):
        return [[rms]*30,[.001]*30,([.003]*3+[.003*bright*(3/5)**.5]*5)*30]
    result={(-1,0.,p):[profile(),profile(),profile()] for p in (0,17)}
    for i,(first,last,sign) in enumerate(SPECS):
        p=17 if 5<=i<=7 else 0
        result[i,.5,p]=[profile()]
        result[i,1.,p]=[profile(.005 if sign<0 else .02,.8)]
    return result


class DirectionEvidenceTest(unittest.TestCase):
    def test_control_direction_and_negatives(self):
        original=fixture()
        self.assertTrue(all(x[-1] for x in directions(original)))
        for i in range(10):
            p=17 if 5<=i<=7 else 0
            a,b=(i,.5,p),(i,1.,p)
            with self.subTest(id=i):
                swapped=copy.deepcopy(original);swapped[a],swapped[b]=swapped[b],swapped[a]
                self.assertFalse(directions(swapped)[i][-1])
                flat=copy.deepcopy(original);flat[b]=flat[a]
                self.assertFalse(directions(flat)[i][-1])
        weak=fixture();weak[8,1.,0][0][2]=[0.]*240
        with self.assertRaises(RuntimeError):directions(weak)
        noisy=fixture();noisy[-1,0.,0][2][0]=[.03]*30
        self.assertFalse(directions(noisy)[0][-1])

    def test_capture_gates(self):
        lines=['REAL_VST3_VOICE_PROFILE_CAPTURE_COMPLETE','REAL_VST3_VOICE_PCM_SUMMARY=7/10 NEUTRAL_RESTORE=PASS','FILTER_PCM_REFERENCE PEAK=0.02 LATE_HELD=0.01 RELEASE_450MS=0.005 VALID=PASS']+['FILTER_PRE_NOTE_SILENCE RMS=0.0001 WINDOWS=3 PASS']*9
        self.assertEqual(len(validate_direction_report('\n'.join(lines),fixture())),10)
        for bad in (lines[1:],lines[:-1],lines+[lines[0]],lines[:2]+[lines[2].replace('0.005','0.00001')]+lines[3:]):
            with self.assertRaises(RuntimeError):validate_direction_report('\n'.join(bad),fixture())

if __name__=='__main__':unittest.main()
