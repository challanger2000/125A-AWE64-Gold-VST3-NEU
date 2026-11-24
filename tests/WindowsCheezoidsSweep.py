"""Private ephemeral comparison of the author's public Cheezoids SF2.
No audio, sample bytes, bank or traces are printed, retained or uploaded.
Compare all bank-zero programs using original Creative/DOSBox vs actual VST3.
"""
import argparse, ctypes, hashlib, math, os, re, subprocess, tempfile, threading, time, uuid, zipfile
from collections import deque
from pathlib import Path
from urllib.request import urlopen
from WindowsPrivateCreativeGuestSmoke import find_private_bundle, guest_arguments

ARCHIVE = "https://raw.githubusercontent.com/Analoguesque/freepatches/681907983ec024c1994f6a02615d650877913696/downloads/Analoguesque_SOUNDFONT_Banks.zip"
BLOB = "c12461b3e016f70b36e39eca2f78eac5685a0fbf"
OWNER_SHA256 = "5adad4b3b95a33ef64ae06dd1627038140435b395d183aab622efcb4c920359b"
OWNER_SIZE = 652206

def chunks(buf,start,end):
    while start+8<=end:
        n=int.from_bytes(buf[start+4:start+8],"little")
        stop=start+8+n
        if stop>end: raise ValueError("Invalid RIFF length")
        yield buf[start:start+4], start+8, stop
        start=stop+(n&1)
    if start!=end: raise ValueError("Invalid RIFF alignment")

def bank_info(raw):
    if raw[:4]!=b"RIFF" or raw[8:12]!=b"sfbk": raise ValueError("Not SF2")
    version=None; programs=[]
    for tag,a,b in chunks(raw,12,len(raw)):
        if tag!=b"LIST":continue
        for sub,c,d in chunks(raw,a+4,b):
            if raw[a:a+4]==b"INFO" and sub==b"ifil":
                version=(int.from_bytes(raw[c:c+2],"little"),int.from_bytes(raw[c+2:c+4],"little"),c)
            if raw[a:a+4]==b"pdta" and sub==b"phdr":
                if (d-c)%38:raise ValueError("Invalid phdr")
                programs=[int.from_bytes(raw[i+20:i+22],"little") for i in range(c,d-38,38)
                          if raw[i+22:i+24]==b"\0\0"]
    if sorted(programs)!=list(range(128)) or version is None or version[:2] not in ((2,0),(2,1)):
        raise ValueError("Unexpected Cheezoids layout/version")
    result=bytearray(raw)
    result[version[2]+2:version[2]+4]=b"\0\0"
    return bytes(result),version[:2]

def acquire(work, local=None):
    if local is not None:
        raw=local.read_bytes()
        if len(raw)!=OWNER_SIZE or hashlib.sha256(raw).hexdigest()!=OWNER_SHA256:
            raise ValueError("Wrong owner Cheezoids revision; no substitute")
        staged,version=bank_info(raw)
        source=work/"Cheezoids.sf2";source.write_bytes(raw)
        (work/"RAMTEST.SF2").write_bytes(staged)
        print("CHEEZ_EXACT_OWNER_BANK_SHA256="+OWNER_SHA256+"; 128 programs",flush=True)
        return source
    z=work/"author.zip"
    with urlopen(ARCHIVE,timeout=60) as response:
        data=response.read(12_000_000)
    if hashlib.sha1(("blob %d\0"%len(data)).encode()+data).hexdigest()!=BLOB:
        raise ValueError("Author archive Git blob mismatch")
    z.write_bytes(data)
    with zipfile.ZipFile(z) as archive:
        names=[n for n in archive.namelist() if Path(n).name.lower() in ("czoids.sf2","cheezoids.sf2")]
        if len(names)!=1:raise ValueError("No unique Cheezoids bank in author archive")
        raw=archive.read(names[0])
    staged,version=bank_info(raw)
    source=work/"Cheezoids.sf2";source.write_bytes(raw)
    (work/"RAMTEST.SF2").write_bytes(staged)
    print("CHEEZ_AUTHOR_BANK_SHA256="+hashlib.sha256(raw).hexdigest()+"; SF2="+str(version)+"; 128 programs",flush=True)
    return source

def reference(bundle,work):
    dllpath,assets=find_private_bundle(bundle)
    os.environ["SDL_VIDEODRIVER"]="dummy"
    os.environ.pop("EMU8K_TRACE",None)
    with os.add_dll_directory(str(dllpath.parent)):
        dll=ctypes.WinDLL(str(dllpath))
        midi=ctypes.CFUNCTYPE(ctypes.c_uint,ctypes.c_void_p,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_uint)
        pcm=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.POINTER(ctypes.c_int16),ctypes.c_uint)
        dll.AWE64_HostRegisterMidi.argtypes=[ctypes.c_void_p,midi]
        dll.AWE64_HostRegisterPCM.argtypes=[ctypes.c_void_p,pcm]
        dll.AWE64_StartGuest.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
        dll.AWE64_SetAbsoluteSends.argtypes=[ctypes.c_uint]
        dll.AWE64_IsGuestMidiReady.restype=ctypes.c_int
        pending=deque(); measurement=None; energy=[0.,0.]; counts=[0,0]
        @midi
        def pull(ctx,buf,cap):
            if not pending or len(pending[0])>cap:return 0
            msg=pending.popleft()
            for i,b in enumerate(msg):buf[i]=b
            return len(msg)
        @pcm
        def receive(ctx,buf,n):
            if measurement is None:return
            t=time.monotonic()-measurement
            k=0 if .20<=t<.60 else 1 if 1.10<=t<1.60 else -1
            if k<0:return
            energy[k]+=sum((buf[i]/32768.)**2 for i in range(n*2))
            counts[k]+=n*2
        dll.AWE64_HostRegisterMidi(None,pull)
        dll.AWE64_HostRegisterPCM(None,receive)
        args=[a.encode("mbcs") for a in guest_arguments(assets,work)]
        argv=(ctypes.c_char_p*(len(args)+1))(*args,None)
        exits=[]
        thread=threading.Thread(target=lambda:exits.append(dll.AWE64_StartGuest(len(args),argv)),daemon=True)
        thread.start()
        results={}
        try:
            deadline=time.monotonic()+30
            while not dll.AWE64_IsGuestMidiReady():
                if not thread.is_alive() or time.monotonic()>deadline:raise RuntimeError("Original bank load/ready failure")
                time.sleep(.05)
            dll.AWE64_SetAbsoluteSends(0)
            pending.append(bytes((0xb0,0,0)))
            time.sleep(2)
            for p in range(128):
                if not thread.is_alive():raise RuntimeError("Original guest exited during sweep")
                pending.append(bytes((0xc0,p)))
                time.sleep(22*512/44100)
                energy[:]=[0.,0.];counts[:]=[0,0]
                measurement=time.monotonic()
                pending.append(bytes((0x90,60,100)))
                time.sleep(151*512/44100)
                measurement=None
                pending.append(bytes((0x80,60,0)))
                time.sleep(44*512/44100)
                if not all(counts):raise RuntimeError("Missing direct PCM windows")
                results[p]=tuple(math.sqrt(e/n) for e,n in zip(energy,counts))
            if pending:raise RuntimeError("Unconsumed original MIDI")
        finally:
            dll.AWE64_RequestStop()
            thread.join(20)
        if thread.is_alive() or exits!=[0]:raise RuntimeError("Original sweep shutdown failed")
        return results

def vst3(bundle,exe,source,work):
    plugin=bundle/"Contents/x86_64-win/AWE64GoldDOSBoxDev.vst3"
    env=dict(os.environ,AWE64_TEST_CHEEZOIDS_SWEEP="1",AWE64_TEST_EXTERNAL_SF2=str(source),
             AWE64_ORIGINAL_CREATIVE_ONLY="1",SDL_AUDIODRIVER="dummy",
             AWE64_CREATIVE_BOOT_LOG="B"+uuid.uuid4().hex[:7].upper()+".LOG")
    for key in ("AWE64_TEST_RAM_TONE_HZ","AWE64_TEST_SEND_PAIR","AWE64_TEST_EFFECT_TYPES","EMU8K_TRACE"):
        env.pop(key,None)
    proc=subprocess.run([str(exe),str(plugin),"--original","60"],env=env,capture_output=True,text=True,timeout=440)
    if proc.returncode or "CHEEZ_VST3_SWEEP_COMPLETE" not in proc.stdout:
        raise RuntimeError("Native Cheezoids sweep failed: "+str(proc.returncode))
    log=plugin.parent.parent/"Resources/PRIVATE"/env["AWE64_CREATIVE_BOOT_LOG"]
    if not log.exists() or b"ERROR:" in log.read_bytes().upper():raise RuntimeError("Creative bank boot failed")
    rows=re.findall(r"^CHEEZ_VST3 (\d+) ([\d.eE+-]+) ([\d.eE+-]+)$",proc.stdout,re.M)
    if len(rows)!=128 or {int(p) for p,_,_ in rows}!=set(range(128)):
        raise RuntimeError("Missing native program measurements")
    return {int(p):(float(a),float(b)) for p,a,b in rows}

def compare(ref,actual):
    # Do not require arbitrary one-shot presets to sustain. Compare each
    # held note against the same original driver. Flag >20 dB discrepancies.
    ref_bleep=[]; vst_bleep=[]; lost=[]; mismatch=[]
    for p,(a,b) in ref.items():
        x,y=actual[p]
        if a>=.0005 and b<a*.1:ref_bleep.append(p)
        if x>=.0005 and y<x*.1:vst_bleep.append(p)
        if b>=.0005 and y<b*.1:lost.append(p)
        if max(a,b,x,y)>=.0005 and abs(math.log10((max(x,y)+1e-7)/(max(a,b)+1e-7)))>1:
            mismatch.append(p)
    print("CHEEZ_ORIGINAL_SHORT_DECAY="+str(ref_bleep))
    print("CHEEZ_VST3_SHORT_DECAY="+str(vst_bleep))
    print("CHEEZ_VST3_LOST_SUSTAIN="+str(lost)+"; PEAK_WINDOW_MISMATCH="+str(mismatch))
    if lost or mismatch:raise RuntimeError("Cheezoids original/VST3 envelope mismatch")
    if len(ref_bleep)>=64:raise RuntimeError("Majority short in original Creative reference too; investigate SF2 loop/envelope support")
    print("CHEEZ_ALL_128_ENVELOPE_COMPARISON_PASS; not hardware identity or subjective sound approval")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--bundle",type=Path,required=True)
    p.add_argument("--exe",type=Path,required=True)
    p.add_argument("--owner-bank",type=Path,help="Private exact uploaded Cheezoids revision; never commit/upload")
    a=p.parse_args()
    # Direct original DLL holds open DOS handles until child exits. Run
    # reference in separate child to protect native VST3 globals and cleanup.
    # Entry below uses parent subprocess dispatch to avoid loading both
    # runtimes in the same Windows process.
    import sys,json
    if os.environ.get("CHEEZ_REFERENCE_WORK"):
        work=Path(os.environ["CHEEZ_REFERENCE_WORK"])
        (work/"reference.json").write_text(json.dumps(reference(a.bundle,work)))
    else:
        with tempfile.TemporaryDirectory(prefix="awe64-cheez-") as tmp:
            work=Path(tmp)
            source=acquire(work,a.owner_bank)
            exact=hashlib.sha256(source.read_bytes()).hexdigest()==OWNER_SHA256
            print("CHEEZ_EXACT_REQUESTED_REVISION="+str(exact),flush=True)
            before=hashlib.sha256(source.read_bytes()).digest()
            env=dict(os.environ,CHEEZ_REFERENCE_WORK=str(work))
            subprocess.run([sys.executable,__file__,"--bundle",str(a.bundle),"--exe",str(a.exe)],env=env,timeout=440,check=True)
            ref={int(k):v for k,v in json.loads((work/"reference.json").read_text()).items()}
            actual=vst3(a.bundle,a.exe,source,work)
            if hashlib.sha256(source.read_bytes()).digest()!=before:raise RuntimeError("Source bank modified")
            compare(ref,actual)
            if not exact:
                raise RuntimeError("Public author revision differs from requested owner bank; this is not its acceptance")
            print("CHEEZ_EXACT_OWNER_REVISION_GATE_PASS")
