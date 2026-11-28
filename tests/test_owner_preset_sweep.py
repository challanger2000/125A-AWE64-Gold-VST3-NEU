import unittest
from WindowsOwnerPresetSweep import parse_report, compare_rows


class SweepTest(unittest.TestCase):
    def test_numeric_report(self):
        good='OWNER_PROBE 2 48 65 0.0001 0.03 0.05 0.1 0.04 0 0.001 133120 44032'
        rows=parse_report(good+'\nOWNER_VST3_SWEEP_COMPLETE',[(2,48,65)])
        self.assertEqual(rows[0]['status'],'SIGNAL_OBSERVED')
        for old,new in (('0.0001','0.001'),('0.05','nan'),('0.05','0.5'),('0.04 0','0.04 1'),('133120','0'),('2 48 65','2 49 65')):
            with self.subTest(new=new),self.assertRaises(ValueError):
                parse_report(good.replace(old,new,1)+'\nOWNER_VST3_SWEEP_COMPLETE',[(2,48,65)])
        for bad in (good,good+'\n'+good+'\nOWNER_VST3_SWEEP_COMPLETE','OWNER_VST3_SWEEP_COMPLETE'):
            with self.assertRaises(ValueError):parse_report(bad,[(2,48,65)])
    def test_silence_and_contaminated(self):
        text='OWNER_PROBE 2 48 65 0.0001 0.001 0.0001 0.001 0 0 0 133120 44032\nOWNER_PROBE_CONTAMINATED 2 60 100\nOWNER_VST3_SWEEP_COMPLETE'
        rows=parse_report(text,[(2,48,65),(2,60,100)])
        self.assertEqual([r['status'] for r in rows],['SILENCE_CANDIDATE','CONTAMINATED'])
        self.assertEqual([r['comparison'] for r in compare_rows(rows,rows)],['BOTH_SILENT_NOT_VERIFIED','INCONCLUSIVE'])
        with self.assertRaises(ValueError):compare_rows(rows,rows[:1])


if __name__=='__main__':
    unittest.main()
