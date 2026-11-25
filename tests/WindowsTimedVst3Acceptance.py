"""Dedicated bounded Windows actual VST3 gate. Never publishes private assets.
Uses baseline self-authored RAM fixture and verified Creative bundle preparation.
Not executed in the Linux remediation environment.
"""
import argparse,os,re,subprocess,tempfile
from pathlib import Path
from MakeRamSampleSf2 import build
from WindowsPrivateCreativeGuestSmoke import find_private_bundle

def main():
    p=argparse.ArgumentParser();p.add_argument('--bundle',type=Path,required=True);p.add_argument('--host-exe',type=Path,required=True);a=p.parse_args()
    if os.name!='nt':raise RuntimeError('Windows x64 required')
    _,assets=find_private_bundle(a.bundle)
    plugin=a.bundle.resolve()/'Contents'/'x86_64-win'/'AWE64GoldDOSBoxDev.vst3'
    with tempfile.TemporaryDirectory(prefix='awe64_timed_native_') as d:
        fixture=Path(d)/'RAMTEST.SF2';fixture.write_bytes(build())
        env=os.environ.copy()
        for key in list(env):
            if key.startswith('AWE64_TEST_') or key in ('EMU8K_TRACE','AWE64_CREATIVE_BOOT_LOG'):env.pop(key,None)
        env.update(AWE64_TEST_HOST_DEMAND_CLOCK='1',AWE64_TEST_TIMED_HOST='1',AWE64_TEST_RAM_TONE_HZ='441',AWE64_TEST_EXTERNAL_SF2=str(fixture),AWE64_ORIGINAL_CREATIVE_ONLY='1',SDL_AUDIODRIVER='dummy')
        failures=[]
        for rate in (44100,48000,96000):
            env['AWE64_TEST_HOST_RATE']=str(rate)
            try:
                p1=subprocess.run([str(a.host_exe.resolve()),str(plugin),'--original','60'],env=env,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=120)
            except subprocess.TimeoutExpired:
                failures.append(rate)
                print(f'Native timed VST3 {rate} Hz: TIMEOUT',flush=True)
                continue
            markers=re.findall(r'^TIMED_HOST_RATE=(\d+) LATENCY=(\d+) EARLY=(\d+) LATE=(\d+) DELTA=(-?\d+) TOLERANCE=(\d+) RESUMED=(\d+)\s*$',p1.stdout,re.M)
            if p1.returncode or p1.stdout.splitlines().count('REAL_VST3_TIMED_OFFLINE_PCM_PASS')!=1 or p1.stdout.splitlines().count('REAL_VST3_TIMED_PAUSE_RESUME_PASS')!=1 or len(markers)!=1:
                failures.append(rate)
                # Only allow fixed, test-owned diagnostics. Never publish raw
                # DOS/Creative output, absolute paths, traces or private data.
                reasons=[line for line in p1.stderr.splitlines()
                         if re.fullmatch(r'SYNTHETIC_VST3_HOST_PCM_FAILURE: Timed host [A-Za-z0-9 /-]+',line)]
                print(f'Native timed VST3 {rate} Hz: FAIL exit={p1.returncode}',flush=True)
                for reason in reasons:print(reason,flush=True)
                continue
            measured,latency,early,late,delta,tol,resumed=map(int,markers[0])
            if measured!=rate or delta!=late-early or abs(delta-384)>tol or tol!=int((rate*2+999)//1000) or abs(resumed-early)>tol:
                failures.append(rate)
                print(f'Native timed VST3 {rate} Hz: independent timing oracle FAIL',flush=True)
                continue
            print(f'Native VST3 {rate} Hz: reported {latency} samples, offset delta {delta} samples, expected 384; resumed onset {resumed}, baseline {early}')
        if failures:raise RuntimeError(f'Native timed VST3 failed at rates {failures}')
    print('TIMED_VST3_NATIVE_THREE_RATE_PASS')
if __name__=='__main__':main()
