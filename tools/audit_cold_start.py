"""Cold-start experiment, synthetic assets only. Preserve every failed trial.
Compare child stdout inheritance (historical direct-like execution) and pipes
(historical successful runtime-audit launcher). Not a Creative sound test.
"""
import argparse
import json
import os
import pathlib
import subprocess
import time

def main():
    p=argparse.ArgumentParser()
    for name in ('exe','plugin','bank','report'):
        p.add_argument('--'+name,required=True,type=pathlib.Path)
    p.add_argument('--variant',required=True,choices=('historical','current'))
    a=p.parse_args()
    env=dict(os.environ)
    env['AWE64_TEST_PERFORMANCE']='1'
    env['SDL_AUDIODRIVER']='dummy'
    for k in ('AWE64_TEST_HOST_DEMAND_CLOCK','AWE64_ORIGINAL_CREATIVE_ONLY',
              'AWE64_AUDIT_SHIM_MODE','AWE64_AUDIT_SHIM_DIR'):
        env.pop(k,None)
    cmd=[str(a.exe.resolve(strict=True)),str(a.plugin.resolve(strict=True)),
         'early',str(a.bank.resolve(strict=True))]
    rows=[]
    # Interleave six fresh independent children per launch method.
    # Alternate which method goes first to avoid systematic warmup bias.
    for pair in range(6):
        methods=('inherited','captured') if pair%2==0 else ('captured','inherited')
        for method in methods:
            start=time.monotonic()
            print('COLD_START_TRIAL_BEGIN',a.variant,pair,method,flush=True)
            row={'pair':pair,'method':method,'variant':a.variant}
            try:
                result=subprocess.run(cmd,env=env,timeout=75,
                    capture_output=(method=='captured'),text=True,errors='replace')
                row['returncode']=result.returncode
                if method=='captured':
                    row['stdout']=result.stdout
                    row['stderr']=result.stderr
                    print(result.stdout,flush=True)
                    print(result.stderr,flush=True)
                    row['pass']=result.returncode==0 and result.stdout.count('AUDIT_A09_EARLY_NOTE PASS')==1 and result.stdout.count('AUDIT_NATIVE_RUNTIME_EXECUTED mode=early failures=0 nonfinite_or_unwritten=0')==1
                else:
                    # Inherited streams remain in CI job log. The common
                    # native host exits nonzero on any failed verdict/cleanup.
                    row['pass']=result.returncode==0
            except subprocess.TimeoutExpired as e:
                row['pass']=False
                row['timeout']=True
                row['stdout']=e.stdout.decode(errors='replace') if isinstance(e.stdout,bytes) else e.stdout
                row['stderr']=e.stderr.decode(errors='replace') if isinstance(e.stderr,bytes) else e.stderr
                print('COLD_START_CHILD_TIMEOUT',row,flush=True)
            row['elapsed_ms']=round((time.monotonic()-start)*1000)
            rows.append(row)
            a.report.write_text(json.dumps(rows,indent=2),encoding='utf-8')
            print('COLD_START_TRIAL_END',a.variant,pair,method,'PASS' if row['pass'] else 'FAIL',
                  'elapsed_ms='+str(row['elapsed_ms']),flush=True)
    counts={m:{'pass':sum(r['pass'] for r in rows if r['method']==m),
               'total':sum(r['method']==m for r in rows)}
            for m in ('captured','inherited')}
    print('COLD_START_COUNTS',json.dumps(counts),flush=True)
    return int(not all(r['pass'] for r in rows))

if __name__=='__main__':
    raise SystemExit(main())
