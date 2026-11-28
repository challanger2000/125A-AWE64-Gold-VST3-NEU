import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from OwnerPresetCoverage import collect
from WindowsOwnerPresetSweep import parse_report


class CoverageTest(unittest.TestCase):
    def test_completed_identity_and_no_duplicate_count(self):
        banks = [{'file':'test.sf2','sha256':'source','presets':[
            {'program':0,'status':'PLANNED','screen':[(48,65),(60,100)]},
            {'program':1,'status':'NOT_ADDRESSABLE','screen':[(60,100)]}]}]
        identity = {'vst3':'vst','engine':'eng','host':'host'}
        row = 'OWNER_PROBE 0 48 65 0.0001 0.03 0.05 0.1 0.04 0 0.001 133120 44032'
        log = row+'\nOWNER_PROBE_CONTAMINATED 0 60 100\nOWNER_VST3_SWEEP_COMPLETE\n'
        probes = [(0,48,65),(0,60,100)]
        report = {'bank':'test.sf2','source_sha':'source','identity':identity,'exit':0,
                  'boot_exists':True,'boot_error':False,'probes':probes,
                  'log_sha':hashlib.sha256(log.encode()).hexdigest(),'rows':parse_report(log,probes)}
        with tempfile.TemporaryDirectory() as tmp:
            rp,lp = Path(tmp)/'r.json',Path(tmp)/'r.log'
            def run(obj,text=log,twice=False):
                rp.write_text(json.dumps(obj));lp.write_text(text)
                return collect(banks,identity,[(rp,lp)]*(2 if twice else 1))
            good=run(report,twice=True)
            self.assertEqual((good['planned'],good['measured'],good['signal'],good['complete_banks']), (2,1,1,0))
            self.assertEqual(good['banks'][0]['remaining'],[[0,60,100]])
            for k,v in [('exit',1),('exit',False),('boot_exists',False),('boot_error',True),('source_sha','wrong'),('identity',{}),('log_sha','wrong'),('rows',[]),('probes',[(0,48,65),(1,60,100)])]:
                with self.subTest(field=k,value=v):
                    bad=copy.deepcopy(report);bad[k]=v
                    result=run(bad)
                    self.assertEqual(result['measured'],0)
                    self.assertEqual(len(result['rejected_reports']),1)
            self.assertEqual(run(report,log.replace('0.03','0.04'))['measured'],0)
            # Even a log with a correctly updated digest cannot fake completion.
            missing=log.replace('OWNER_VST3_SWEEP_COMPLETE\n','')
            bad=copy.deepcopy(report);bad['log_sha']=hashlib.sha256(missing.encode()).hexdigest()
            self.assertEqual(run(bad,missing)['measured'],0)
            second='OWNER_PROBE 0 60 100 0.0001 0.02 0.04 0.2 0.19 1 0.001 133120 44032\nOWNER_VST3_SWEEP_COMPLETE\n'
            other=copy.deepcopy(report);other['probes']=[(0,60,100)];other['rows']=parse_report(second,other['probes']);other['log_sha']=hashlib.sha256(second.encode()).hexdigest()
            run(report)
            rp2,lp2=Path(tmp)/'s.json',Path(tmp)/'s.log';rp2.write_text(json.dumps(other));lp2.write_text(second)
            both=collect(banks,identity,[(rp,lp),(rp2,lp2)])
            self.assertEqual((both['measured'],both['signal'],both['complete_banks']),(2,2,1))
            self.assertEqual(both['banks'][0]['review'][0]['statuses'],['CONTAMINATED','LARGE_STEP_REVIEW'])


if __name__=='__main__':unittest.main()
