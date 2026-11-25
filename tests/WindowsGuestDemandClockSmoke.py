#!/usr/bin/env python3
"""Real Windows DLL/Creative RAM guest clock gate, not VST3 host acceptance.
The parent is bounded; no trace, audio, guest bytes or native guest logs are published.
Requires the baseline test helpers in tests/ and the augmented runtime DLL.
"""
import argparse
from array import array
import ctypes as C
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time

RATE=44100
TICK_MAX=45

def analyze(samples, deliveries):
    # Independent PCM oracle. Compare two held notes using equal windows;
    # release and idle windows are well away from the note transitions.
    def rms(start,end):
        part=samples[2*start:2*end]
        if len(part)!=2*(end-start):raise RuntimeError('Incomplete PCM window')
        return (sum(v*v for v in part)/len(part))**.5
    if len(deliveries)!=6:raise RuntimeError('Missing or duplicate MIDI packets')
    for target,actual in deliveries:
        if not target<=actual<=target+2*TICK_MAX:
            raise RuntimeError('Guest MIDI deadline violated')
    a=rms(8000,14000);b=rms(52100,58100);idle=rms(37000,40000)
    if min(a,b)<100 or idle>min(a,b)*.05 or not .85<=a/b<=1.15:
        raise RuntimeError('Held-note PCM / release oracle failed')
    # Find the independent rising edge on each note, not the packet callback.
    def onset(start,end,threshold):
        for i in range(start,end):
            if max(abs(samples[2*i]),abs(samples[2*i+1]))>=threshold:return i
        raise RuntimeError('PCM onset missing')
    threshold=min(a,b)*.2
    x=onset(4000,8000,threshold);y=onset(48100,52100,threshold)
    if abs((y-x)-RATE)>2*TICK_MAX:raise RuntimeError('Relative PCM onset interval wrong')
    return x,y

def child(bundle,work):
    if os.name!='nt':raise RuntimeError('Windows required')
    from WindowsPrivateCreativeGuestSmoke import find_private_bundle,guest_arguments
    from MakeRamSampleSf2 import build
    dll_path,assets=find_private_bundle(bundle)
    (work/'RAMTEST.SF2').write_bytes(build())
    os.environ.pop('SDL_AUDIODRIVER',None)
    os.environ.pop('EMU8K_TRACE',None)
    with os.add_dll_directory(str(dll_path.parent)):
        dll=C.WinDLL(str(dll_path))
        def fn(name,args,ret):
            f=getattr(dll,name);f.argtypes=args;f.restype=ret;return f
        ready=fn('AWE64_IsGuestMidiReady',[],C.c_int)
        fault=fn('AWE64_GuestClockFault',[],C.c_int)
        internal=fn('AWE64_IsInternalAudioOnly',[],C.c_int)
        start=fn('AWE64_StartGuest',[C.c_int,C.POINTER(C.c_char_p)],C.c_int)
        stop=fn('AWE64_RequestStop',[],None)
        clock_mode=fn('AWE64_SetGuestClockPCM',[C.c_uint],None)
        clock_type=C.CFUNCTYPE(C.c_int,C.c_void_p)
        midi_type=C.CFUNCTYPE(C.c_uint,C.c_void_p,C.POINTER(C.c_ubyte),C.c_uint)
        pcm_type=C.CFUNCTYPE(None,C.c_void_p,C.POINTER(C.c_int16),C.c_uint)
        clock_register=fn('AWE64_HostRegisterClock',[C.c_void_p,clock_type],None)
        midi_register=fn('AWE64_HostRegisterMidi',[C.c_void_p,midi_type],None)
        pcm_register=fn('AWE64_HostRegisterPCM',[C.c_void_p,pcm_type],None)
        pcm=array('h'); deliveries=[]; horizon=0; index=0; callback_error=[]
        # Each NoteOn is separated by exactly one second of native frame time.
        events=[(0,bytes((0xb0,0,0))),(0,bytes((0xc0,0))),
                (4410,bytes((0x90,60,100))),(17640,bytes((0x80,60,0))),
                (48510,bytes((0x90,60,100))),(61740,bytes((0x80,60,0)))]
        @clock_type
        def tick(_):
            return 0 if not ready() else (1 if len(pcm)//2<horizon else 2)
        @midi_type
        def midi(_,out,capacity):
            nonlocal index
            if not ready() or index>=len(events):return 0
            target,msg=events[index];actual=len(pcm)//2
            if actual<target or capacity<len(msg):return 0
            for j,v in enumerate(msg):out[j]=v
            deliveries.append((target,actual));index+=1
            return len(msg)
        @pcm_type
        def receive(_,src,frames):
            if ready() and src and frames:
                try:
                    if len(pcm)//2+frames>100000:raise RuntimeError('Guest exceeded hard PCM bound')
                    pcm.frombytes(C.string_at(src,frames*4))
                except Exception as exc:callback_error.append(str(exc))
        clock_mode(1);clock_register(None,tick);midi_register(None,midi);pcm_register(None,receive)
        args=[s.encode('mbcs') for s in guest_arguments(assets,work)]
        argv=(C.c_char_p*(len(args)+1))(*args,None)
        result=[];worker=threading.Thread(target=lambda:result.append(start(len(args),argv)),daemon=True);worker.start()
        def await_condition(check,timeout):
            deadline=time.monotonic()+timeout
            while not check():
                if not worker.is_alive() or callback_error or fault():raise RuntimeError('Guest clock/worker failed')
                if time.monotonic()>=deadline:raise RuntimeError('Guest clock deadline timeout')
                time.sleep(.005)
        def booted():
            p=work/'RAMBOOT.LOG'
            try:s=p.read_text(encoding='ascii',errors='replace')
            except OSError:return False
            if 'ERROR:' in s:raise RuntimeError('Creative bank boot failed')
            return 'Press <Esc> to stop' in s and ready() and internal()==1
        try:
            await_condition(booted,20)
            horizon=384;await_condition(lambda:len(pcm)//2>=horizon,5)
            before=len(pcm)//2;time.sleep(.1)
            if len(pcm)//2!=before or not horizon<=before<horizon+TICK_MAX:
                raise RuntimeError('Guest advanced without host demand')
            # Advance the actual guest virtual time without pacing by wall clock.
            for target in (12000,32000,60000,88200):
                horizon=target;await_condition(lambda:len(pcm)//2>=horizon,20)
                before=len(pcm)//2;time.sleep(.03)
                if len(pcm)//2!=before or not horizon<=before<horizon+TICK_MAX:
                    raise RuntimeError('Guest demand boundary violated')
            if callback_error or fault():raise RuntimeError('Guest PCM fault')
            onset=analyze(pcm,deliveries)
        finally:
            stop();worker.join(5)
            if worker.is_alive():raise RuntimeError('Guest did not stop at demand hold')
        if result!=[0]:raise RuntimeError('Guest shutdown unsuccessful')
        print(f'NATIVE_GUEST_DEMAND_CLOCK_PASS FRAMES={len(pcm)//2} ONSET_DELTA={onset[1]-onset[0]} PACKETS={len(deliveries)}',flush=True)

def main():
    p=argparse.ArgumentParser();p.add_argument('--bundle',type=Path,required=True);p.add_argument('--child-work',type=Path);a=p.parse_args()
    if a.child_work is not None:child(a.bundle,a.child_work);return
    with tempfile.TemporaryDirectory(prefix='awe64_clock_') as w:
        try:proc=subprocess.run([sys.executable,__file__,'--bundle',str(a.bundle.resolve()),'--child-work',w],capture_output=True,text=True,timeout=120,errors='replace')
        except subprocess.TimeoutExpired:raise SystemExit('Native guest clock test timed out')
        matches=[x for x in proc.stdout.splitlines() if x.startswith('NATIVE_GUEST_DEMAND_CLOCK_PASS ')]
        if proc.returncode or len(matches)!=1:raise SystemExit(f'Native guest clock test FAILED; exit={proc.returncode}')
        print(matches[0])
if __name__=='__main__':main()
