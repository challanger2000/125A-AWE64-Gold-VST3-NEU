"""Private native VST3 screening. Reports measurements, not sound approval.
Never writes or uploads source SF2 files or raw audio. One host per bank.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import uuid
from OwnerPresetPlan import plan


def expected_probes(bank, mode, programs=None):
    return [(p['program'],k,v) for p in bank['presets'] if p['status']=='PLANNED'
            and (programs is None or p['program'] in programs) for k,v in p[mode]]


def parse_report(stdout, probes):
    lines=stdout.splitlines()
    if lines.count('OWNER_VST3_SWEEP_COMPLETE')!=1:
        raise ValueError('missing or ambiguous native completion')
    expected=set(probes)
    if not expected or len(expected)!=len(probes):
        raise ValueError('invalid expected probe list')
    result={}
    for line in lines:
        if not line.startswith(('OWNER_PROBE ', 'OWNER_PROBE_CONTAMINATED ')):
            continue
        fields=line.split()
        p=tuple(map(int,fields[1:4]))
        if p not in expected or p in result:
            raise ValueError('duplicate or unexpected probe')
        if fields[0]=='OWNER_PROBE_CONTAMINATED':
            if len(fields)!=4:
                raise ValueError('malformed contamination row')
            result[p]={'status':'CONTAMINATED'}
            continue
        if len(fields)!=13:
            raise ValueError('malformed PCM row')
        idle,rms,ac,peak,step=map(float,fields[4:9])
        jumps=int(fields[9]);tail=float(fields[10]);held,release=map(int,fields[11:13])
        if (not all(math.isfinite(x) and x>=0 for x in (idle,rms,ac,peak,step,tail))
                or idle>.00025 or max(rms,ac)>peak*1.00001+1e-10
                or step>2*peak*1.00001+1e-10 or held!=260*512 or release!=86*512
                or not 0<=jumps<held or (jumps and step<.15)):
            raise ValueError('invalid PCM evidence')
        status=('SILENCE_CANDIDATE' if ac<max(.0005,4*idle) else
                'LARGE_STEP_REVIEW' if jumps else 'SIGNAL_OBSERVED')
        result[p]={'status':status,'idle':idle,'rms':rms,'ac':ac,'peak':peak,
                   'max_step':step,'jumps':jumps,'tail_rms':tail,'held_frames':held,'tail_frames':release}
    if set(result)!=expected:
        raise ValueError('incomplete native probe coverage')
    return [{'program':p[0],'key':p[1],'velocity':p[2],**result[p]} for p in probes]


def run_bank(bundle, exe, source, sha, probes):
    from WindowsPrivateCreativeGuestSmoke import find_private_bundle
    _,assets=find_private_bundle(bundle)
    plugin=bundle/'Contents/x86_64-win/125A AWE64 Gold.vst3'
    boot='B'+uuid.uuid4().hex[:7].upper()+'.LOG'
    log=assets/boot
    if log.exists():
        raise ValueError('stale boot log')
    if hashlib.sha256(source.read_bytes()).hexdigest()!=sha:
        raise ValueError('source bank changed before test')
    env={k:v for k,v in os.environ.items() if not k.startswith('AWE64_TEST_') and k!='EMU8K_TRACE'}
    env.update(AWE64_TEST_OWNER_SWEEP='1',AWE64_TEST_HOST_RATE='44100',
               AWE64_TEST_EXTERNAL_SF2=str(source),AWE64_ORIGINAL_CREATIVE_ONLY='1',
               SDL_AUDIODRIVER='dummy',AWE64_CREATIVE_BOOT_LOG=boot)
    payload=str(len(probes))+'\n'+''.join('%d %d %d\n'%p for p in probes)
    try:
        proc=subprocess.run([str(exe),str(plugin),'--original','60'],env=env,input=payload,
                            text=True,encoding='utf-8',errors='replace',capture_output=True,
                            timeout=150+10*len(probes))
        if proc.returncode or not log.exists() or b'ERROR:' in log.read_bytes().upper():
            raise RuntimeError('native bank host/boot failed; exit='+str(proc.returncode))
        return parse_report(proc.stdout,probes)
    finally:
        if hashlib.sha256(source.read_bytes()).hexdigest()!=sha:
            raise RuntimeError('source bank modified')


def compare_rows(reference, actual):
    def key(r):return r['program'],r['key'],r['velocity']
    base={key(r):r for r in reference}
    if len(base)!=len(reference) or set(base)!={key(r) for r in actual} or len(actual)!=len(base):
        raise ValueError('reference coverage mismatch')
    result=[]
    for row in actual:
        old=base[key(row)]
        if 'ac' not in row or 'ac' not in old:
            label='INCONCLUSIVE'
        elif old['status']=='SILENCE_CANDIDATE' and row['status']!='SILENCE_CANDIDATE':
            label='SIGNAL_RECOVERED_CANDIDATE'
        elif old['status']!='SILENCE_CANDIDATE' and row['status']=='SILENCE_CANDIDATE':
            label='LOST_SIGNAL_REVIEW'
        elif old['status']=='SILENCE_CANDIDATE':
            label='BOTH_SILENT_NOT_VERIFIED'
        elif abs(math.log10((row['ac']+1e-9)/(old['ac']+1e-9)))>1:
            label='LEVEL_CHANGE_REVIEW'
        elif row['jumps']>old['jumps']:
            label='ADDITIONAL_LARGE_STEPS_REVIEW'
        else:
            label='NO_SCREENING_DISCREPANCY'
        result.append({**row,'reference':old,'comparison':label})
    return result


def binary_identity(bundle,exe):
    binaries=bundle/'Contents/x86_64-win'
    return {name:hashlib.sha256(path.read_bytes()).hexdigest() for name,path in
            [('vst3',binaries/'125A AWE64 Gold.vst3'),('engine',binaries/'Engine.dll'),('host',exe)]}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--banks',type=Path,required=True)
    ap.add_argument('--bundle',type=Path,required=True)
    ap.add_argument('--exe',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--reference-bundle',type=Path)
    ap.add_argument('--bank',action='append',help='exact SF2 filename; repeat to select multiple')
    ap.add_argument('--program',type=int,action='append')
    ap.add_argument('--mode',choices=('screen','probes'),default='screen')
    args=ap.parse_args()
    banks=plan(args.banks)
    if args.bank:
        if not set(args.bank)<={b['file'] for b in banks}:
            ap.error('unknown bank filename')
        banks=[b for b in banks if b['file'] in args.bank]
    if args.program and not all(0<=p<=127 for p in args.program):
        ap.error('invalid program')
    args.bundle=args.bundle.resolve(strict=True);args.exe=args.exe.resolve(strict=True)
    identity=binary_identity(args.bundle,args.exe)
    baseline_identity=None
    if args.reference_bundle:
        args.reference_bundle=args.reference_bundle.resolve(strict=True)
        if args.reference_bundle==args.bundle:
            ap.error('reference and candidate must be distinct bundles')
        baseline_identity=binary_identity(args.reference_bundle,args.exe)
    args.output.mkdir(parents=True,exist_ok=False)
    failed=False
    for n,bank in enumerate(banks):
        source=(args.banks/bank['file']).resolve(strict=True)
        probes=expected_probes(bank,args.mode,args.program)
        report={'inventory':bank,'mode':args.mode,'planned':len(probes),'rows':[], 'status':'NOT_RUN','candidate_identity':identity,'reference_identity':baseline_identity}
        try:
            if not probes:
                raise ValueError('no addressable selected probes')
            report['rows']=run_bank(args.bundle.resolve(strict=True),args.exe.resolve(strict=True),source,bank['sha256'],probes)
            report['status']='SCREENING_MEASURED_NOT_SOUND_APPROVAL'
            if args.reference_bundle:
                baseline=run_bank(args.reference_bundle.resolve(strict=True),args.exe.resolve(strict=True),source,bank['sha256'],probes)
                report['rows']=compare_rows(baseline,report['rows'])
        except (ValueError,RuntimeError,OSError,subprocess.SubprocessError) as e:
            failed=True;report['status']='INCOMPLETE';report['error']=str(e)
        with (args.output/f'{n:02d}.json').open('x',encoding='utf-8') as fp:
            json.dump(report,fp,indent=2,ensure_ascii=False,allow_nan=False)
        print(bank['file'],report['status'],len(report['rows']),flush=True)
    return 1 if failed else 0


if __name__=='__main__':
    raise SystemExit(main())
