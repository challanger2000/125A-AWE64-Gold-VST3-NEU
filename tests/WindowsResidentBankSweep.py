"""Private native A/B/A fixture with same program, different sample identity.
The caller supplies locally generated exact resident guest and DOS extender.
This script never uploads, installs into the user's VST path or changes SF2s.
"""
import argparse
import hashlib
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import uuid
from MakeRamSampleSf2 import build
from WindowsPrivateCreativeGuestSmoke import find_private_bundle

RESIDENT = "fd0c30c0aa8c6651a5e08c715189bcd9966be713f60790159ef332a8060772f5"
EXTENDER = "b8265123ac8a189637448618409ef3ecd2e9f3e1a47062c685a02240f688dec1"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_report(stdout, resident):
    lines = stdout.splitlines()
    if lines.count("REAL_VST3_RESIDENT_SWITCH_PCM_STATE_PASS") != 1:
        raise ValueError("Missing complete native A/B/A evidence")
    if lines.count("AWE64_RESIDENT_BANK_COMMIT") != (3 if resident else 0):
        raise ValueError("Resident path not actually exercised or unexpected fallback")
    tones = []
    changes = []
    for line in lines:
        if line.startswith("RESIDENT_TONE "):
            m = re.fullmatch(r"RESIDENT_TONE HZ=(\S+) RMS=(\S+) IDLE=(\S+) RATIO=(\S+)", line)
            if not m:
                raise ValueError("Malformed tone evidence")
            hz, rms, idle, ratio = map(float, m.groups())
            if not all(math.isfinite(x) for x in (hz,rms,idle,ratio)) or not (
                    0 <= idle < .0005 and rms > .002 and rms > idle * 5 and ratio > 20):
                raise ValueError("Tone signature or silence evidence failed")
            tones.append(hz)
        elif line.startswith("RESIDENT_CHANGE "):
            m = re.fullmatch(r"RESIDENT_CHANGE INDEX=(\d+) MS=(\S+) STATE=MATCH",line)
            if not m:
                raise ValueError("State mismatch")
            ms=float(m[2])
            if not math.isfinite(ms) or ms <= 0:
                raise ValueError("Invalid measured change time")
            changes.append((int(m[1]),ms))
    if tones != [441.,735.,441.] or [i for i,_ in changes] != [1,2]:
        raise ValueError("Wrong or incomplete bank change sequence")
    return [ms for _,ms in changes]


def run(bundle, exe, a, b, engine_sha, resident):
    _, assets = find_private_bundle(bundle)
    plugin=bundle/"Contents/x86_64-win/125A AWE64 Gold.vst3"
    boot="B"+uuid.uuid4().hex[:7].upper()+".LOG"
    log=assets/boot
    if log.exists():
        raise ValueError("Existing boot log")
    env={k:v for k,v in os.environ.items() if not k.startswith("AWE64_TEST_") and k!="EMU8K_TRACE"}
    env.update(AWE64_ORIGINAL_CREATIVE_ONLY="1",SDL_AUDIODRIVER="dummy",
               AWE64_TEST_EXTERNAL_SF2=str(a),AWE64_TEST_RAM_TONE_HZ="441",
               AWE64_TEST_HOST_RATE="44100",AWE64_TEST_RESIDENT_SWITCH=str(b),
               AWE64_CREATIVE_BOOT_LOG=boot)
    if resident:
        env.update(AWE64_TEST_RESIDENT_BANK="1",AWE64_TEST_RESIDENT_ENGINE_SHA256=engine_sha)
    proc=subprocess.run([str(exe),str(plugin),"--original","60"],env=env,
                        capture_output=True,text=True,encoding="utf-8",errors="replace",timeout=180)
    for line in proc.stdout.splitlines():
        if line.startswith(("RESIDENT_","AWE64_RESIDENT_BANK_COMMIT","REAL_VST3_RESIDENT_")):
            print(line)
    if proc.returncode or not log.exists() or b"ERROR:" in log.read_bytes().upper():
        raise RuntimeError("Native bank change test failed; exit="+str(proc.returncode))
    return validate_report(proc.stdout,resident)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--bundle",type=Path,required=True)
    p.add_argument("--exe",type=Path,required=True)
    p.add_argument("--resident",type=Path,required=True)
    p.add_argument("--dos4gw",type=Path,required=True)
    p.add_argument("--engine-sha",required=True,help="Exact Engine.dll SHA256 from verified private build manifest")
    a=p.parse_args()
    bundle=a.bundle.resolve(strict=True);exe=a.exe.resolve(strict=True)
    engine,_=find_private_bundle(bundle)
    if sha(engine)!=a.engine_sha or sha(a.resident)!=RESIDENT or sha(a.dos4gw)!=EXTENDER:
        raise ValueError("Resident/engine/extender identity mismatch")
    with tempfile.TemporaryDirectory(prefix="awe64-resident-private-") as work:
        work=Path(work)
        isolated=work/bundle.name
        shutil.copytree(bundle,isolated)
        _,assets=find_private_bundle(isolated)
        for src,name,expected in ((a.resident,"Resident.exe",RESIDENT),(a.dos4gw,"DOS4GW.EXE",EXTENDER)):
            dest=assets/name
            if dest.exists() and sha(dest)!=expected:
                raise ValueError("Conflicting private guest asset")
            if not dest.exists():
                shutil.copyfile(src,dest)
            if sha(dest)!=expected:
                raise ValueError("Staged guest asset mismatch")
        first,second=work/"A 441.sf2",work/"B 735.sf2"
        first.write_bytes(build());second.write_bytes(build(resident_b=True))
        before=sha(first),sha(second)
        old=run(isolated,exe,first,second,a.engine_sha,False)
        new=run(isolated,exe,first,second,a.engine_sha,True)
        if (sha(first),sha(second))!=before or sha(engine)!=a.engine_sha:
            raise RuntimeError("Original inputs modified")
        print("RESIDENT_A_B_TIMES_MS baseline="+str(old)+" resident="+str(new))
        # Measure actual speed, never infer it from the path marker.
        if not all(n < o for n,o in zip(new,old)):
            raise RuntimeError("Resident small-bank speed gain not established")
        print("RESIDENT_PRIVATE_SMALL_BANK_GAIN_OBSERVED; other banks/rollback/DAW still unverified")


if __name__=="__main__":
    main()
