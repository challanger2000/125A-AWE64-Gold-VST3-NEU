"""Adversarial audit of the actual pinned Python oracles and portable SF2 parser.
No original assets or modified product modules. Exit 1 = demonstrated oracle/preflight defect.
"""
import contextlib,io,math,os,re,runpy,struct,subprocess,sys,tempfile
from pathlib import Path
from unittest.mock import patch
from array import array
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import WindowsCheezoidsSweep as cheez
import WindowsGuestDemandClockSmoke as demand
import WindowsRunVst3HostPcmSmoke as smoke
failures=[]
def verdict(name,ok,detail):
    print(name, 'PASS' if ok else 'FAIL',detail,flush=True)
    if not ok:failures.append(name)
def accepted(call):
    try:
        with contextlib.redirect_stdout(io.StringIO()):call()
        return True
    except RuntimeError:return False
# Positive and obvious invalid all-silent reference/actual.
tones={i:(.05,.04) for i in range(128)}
silent={i:(0.,0.) for i in range(128)}
assert accepted(lambda:cheez.compare(tones,tones))
assert not accepted(lambda:cheez.compare(tones,silent))
verdict('A16_CHEEZOIDS_SILENT_REFERENCE',not accepted(lambda:cheez.compare(silent,silent)),'both 128-program sets silent')
# Same onset positions, genuine bipolar sinusoidal control vs DC plateau.
def samples(dc=False):
    a=array('h',[0])*176400
    for start in (4410,48510):
        for i in range(start,start+13230):
            v=1000 if dc else round(1000*math.sin(2*math.pi*441*(i-start)/44100))
            a[2*i]=a[2*i+1]=v
    return a
targets=[0,0,4410,17640,48510,61740]
deliveries=[(x,x) for x in targets]
assert accepted(lambda:demand.analyze(samples(),deliveries))
assert not accepted(lambda:demand.analyze(samples(),[(x,x+200) for x in targets]))
verdict('A23_DC_AUDIO_ORACLE',not accepted(lambda:demand.analyze(samples(True),deliveries)),'DC instead of 441 Hz')
verdict('A23_DUPLICATE_EVENT_ORACLE',not accepted(lambda:demand.analyze(samples(),[(0,0)]*6)),'six copies of same event')
# Execute the original native-host report consumer with a deterministic
# mock subprocess result; this tests the report oracle, not native PCM.
common="""REAL_VST3_TRANSPORT_LATENCY_PCM_PASS; QUEUE GATE SEPARATE
PERF_CPU_COUNTER_POSITIVE_CONTROL_PASS
PERF_CPU_MEASURE_METHOD=ETW_SCHEDULED_QPC; CI GUEST SCHEDULED TIME
REAL_VST3_PERFORMANCE_MEASUREMENT_PASS; CI CPU NOT OWNER CPU
PRIVATE_CREATIVE_VST3_HOST_NONZERO_PCM_OBSERVED
VST3_HOST_NOTE_ON_EVENTS_SENT=12
VST3_HOST_NOTE_OFF_EVENTS_SENT=12
PERF_QUEUE_BLOCKS=100 AUDIO_DROPPED_FRAMES=0 MIDI_DROPPED_EVENTS=0 UNDERRUN_FRAMES=0
"""
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'dummy.exe';p.touch()
    def consumer(report):
        with patch.dict(os.environ,{'AWE64_TEST_PERFORMANCE':'1'},clear=True),patch.object(smoke,'run_private_creative_host',return_value=subprocess.CompletedProcess([],0,report,'')):
            return accepted(lambda:smoke.run_native_host(p,p,True))
    positive=common+'PERF_HOST_RATE=44100 PERF_BLOCK=128 LOGICAL_CPUS=4\nREAL_VST3_SUSTAIN_PCM_PASS\nPERF_CALLBACK_MAX_MS=0.1 CALLBACK_DEADLINE_OVERRUNS=0 HOST_WAKE_MAX_LATE_MS=1\nPERF_MIDI_TO_PCM_MEDIAN_MS=17 P95_MS=18 MAX_MS=19\nPERF_PROCESS_ONE_CORE_PERCENT=10 TOTAL_MACHINE_PERCENT=2.5\nPERF_32_NOTE_ONE_CORE_PERCENT=20 TOTAL_MACHINE_PERCENT=5\n'
    assert consumer(positive)
    assert not consumer(positive.replace('AUDIO_DROPPED_FRAMES=0','AUDIO_DROPPED_FRAMES=1'))
    near=positive.replace('P95_MS=18 MAX_MS=19','P95_MS=20.2292 MAX_MS=20.2292')
    verdict('A15_LATENCY_TOLERANCE_POSITIVE',consumer(near),'owner-accepted minor target deviation')
    verdict('A15_LATENCY_CEILING_NEGATIVE',not consumer(near.replace('20.2292','21.01')),'above 21 ms ceiling')
    verdict('A15_DEADLINE_OVERRUN_ORACLE',not consumer(positive.replace('OVERRUNS=0','OVERRUNS=99')),'99 overruns')
    verdict('A15_MISSING_NUMERIC_EVIDENCE',not consumer(common),'no rate, p95, CPU values, sustain, deadline evidence')
workflow=(Path(__file__).resolve().parents[2]/'.github/workflows/windows-dosbox-runtime.yml').read_text()
match=re.search(r'-match \'(\^Result: [^\']+)\'',workflow)
assert match and re.fullmatch(match[1],'Result: 47 tests passed, 0 tests failed')
assert not re.fullmatch(match[1],'Result: 47 tests passed, 1 tests failed')
verdict('A20_VALIDATOR_COVERAGE_ORACLE',not re.fullmatch(match[1],'Result: 1 tests passed, 0 tests failed'),'1/47 accepted by actual regex')
# Real preflight executable from the existing portable CMake target.
probe=Path(sys.argv[1]).resolve(strict=True)
with tempfile.TemporaryDirectory() as d:
    old=sys.argv;sys.argv=['MakeAuditSf2Cases.py',d]
    fixture=runpy.run_path(str(Path(__file__).resolve().parents[1]/'MakeAuditSf2Cases.py'))
    sys.argv=old
    fixture['write']('self_linked_stereo',lambda l:fixture['mutate'](l[b'pdta'],b'shdr',44,'<H',2))
    def probe_rc(expect,*paths):
        return subprocess.run([str(probe),expect,*[str(Path(d)/p) for p in paths]],capture_output=True,text=True)
    assert probe_rc('accept','valid.sf2').returncode==0
    assert probe_rc('reject','reject_sample_loop_end_oob.sf2').returncode==0
    p=probe_rc('reject','self_linked_stereo.sf2')
    print(p.stdout,p.stderr,sep='\n',flush=True)
    verdict('A26_SF2_SELF_LINKED_STEREO',p.returncode==0,'right sample links to itself instead of reciprocal left')
print('ADVERSARIAL_AUDIT_FAILURES',failures,flush=True)
sys.exit(bool(failures))
