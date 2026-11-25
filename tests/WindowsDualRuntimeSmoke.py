"""Research gate: two independently mapped original DOSBox DLLs in ONE process.

Self-authored MIDI/PIT guest, zero ROM, no Creative or owner material.
Not a VST3 or EMU8000 sound test. Parent bounds crash/hang risk.
No product changes or external runtime process proposed by this experiment.
"""
import ctypes as C
import hashlib
import faulthandler
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import threading
import time

from MakeMidiTriggeredDemoExe import build_midi_triggered_exe

MIDI = C.CFUNCTYPE(C.c_uint, C.c_void_p, C.POINTER(C.c_ubyte), C.c_uint)
PCM = C.CFUNCTYPE(None, C.c_void_p, C.POINTER(C.c_int16), C.c_uint)


def wait_for(predicate, seconds, label):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(.01)
    raise RuntimeError(label)


def signature_note(note, divisor):
    image = build_midi_triggered_exe()
    old = bytes.fromhex("3c3c")
    pit = bytes.fromhex("b8980a")
    if image.count(old) != 1 or image.count(pit) != 1:
        raise RuntimeError("Original synthetic fixture anchors changed")
    # Preserve the same guest and PIT writes; increase only the BIOS
    # hold interval (36 -> 126 ticks, ~7s). Enough time to measure the
    # surviving guest separately after stopping and unloading its peer.
    hold = bytes.fromhex("29da83fa24")
    if image.count(hold)!=1:
        raise RuntimeError("Synthetic hold anchor changed")
    return image.replace(old, bytes((0x3c,note))).replace(
        pit, bytes((0xb8,divisor&255,divisor>>8))).replace(
        hold, bytes.fromhex("29da83fa7e"))


class Runtime:
    def __init__(self, source, root, name, note, divisor):
        self.name = name
        self.folder = root / name
        self.folder.mkdir()
        dll = self.folder / ("AWE64Runtime_" + name + ".dll")
        shutil.copyfile(source,dll)
        if hashlib.sha256(dll.read_bytes()).digest()!=hashlib.sha256(source.read_bytes()).digest():
            raise RuntimeError("Runtime copy differs")
        (self.folder/"awe32.raw").write_bytes(bytes(1048576))
        (self.folder/"DEMO.EXE").write_bytes(signature_note(note,divisor))
        self.lib=C.WinDLL(str(dll),winmode=0x1100)
        self.start=self.lib.AWE64_StartGuest
        self.start.argtypes=[C.c_int,C.POINTER(C.c_char_p)]
        self.start.restype=C.c_int
        self.stop=self.lib.AWE64_RequestStop
        self.stop.argtypes=[]
        self.stop.restype=None
        self.ready=self.lib.AWE64_IsGuestMidiReady
        self.ready.argtypes=[]
        self.ready.restype=C.c_int
        self.midi=self.lib.AWE64_HostRegisterMidi
        self.midi.argtypes=[C.c_void_p,MIDI]
        self.midi.restype=None
        self.pcm=self.lib.AWE64_HostRegisterPCM
        self.pcm.argtypes=[C.c_void_p,PCM]
        self.pcm.restype=None
        self.trigger=False
        self.sent=0
        self.frames=0
        self.audible=0
        self.crossings=0
        self.prev=0
        self.result=[]

        @MIDI
        def feed(ctx, buffer, capacity):
            if not self.trigger or self.sent or capacity<3:
                return 0
            buffer[0],buffer[1],buffer[2]=0x90,note,100
            self.sent+=1
            return 3

        @PCM
        def receive(ctx,samples,count):
            self.frames+=count
            for i in range(count):
                x=int(samples[2*i])
                if abs(x)>128:
                    self.audible+=1
                    sign=1 if x>0 else -1
                    if self.prev and sign!=self.prev:
                        self.crossings+=1
                    self.prev=sign

        self.feed=feed
        self.receive=receive
        self.midi(None,self.feed)
        self.pcm(None,self.receive)
        args=["AWE64DOSBoxRuntime","-noconfig","-nogui","-nomenu",
              "-set","sblaster sbtype=sbawe","-set","sblaster awe32ram=28672",
              "-set","sblaster awe32rom="+str(self.folder/"awe32.raw"),
              "-set","midi mpu401=intelligent","-set","mixer rate=44100",
              "-c",'mount c "'+str(self.folder)+'"','-c',"c:","-c","DEMO.EXE"]
        self.args=[x.encode("mbcs") for x in args]
        self.argv=(C.c_char_p*(len(self.args)+1))(*self.args,None)
        def run():
            try:
                self.result.append(self.start(len(self.args),self.argv))
                print(f"DUAL_STAGE {self.name} MAIN_RETURN={self.result}",flush=True)
            except BaseException as error:
                print(f"DUAL_STAGE {self.name} MAIN_EXCEPTION={error!r}",flush=True)
                raise
        self.worker=threading.Thread(target=run,daemon=True,name="DOSBox-"+name)

    def stop_join(self):
        print(f"DUAL_STAGE {self.name} STOP_ENTER",flush=True)
        if self.worker.is_alive():
            self.stop()
            print(f"DUAL_STAGE {self.name} STOP_REQUEST_RETURNED",flush=True)
            self.worker.join(5)
        if self.worker.is_alive():
            faulthandler.dump_traceback()
            raise RuntimeError("Runtime shutdown hung")
        if self.result != [0]:
            raise RuntimeError("Runtime exit was not clean: "+repr(self.result))
        print(f"DUAL_STAGE {self.name} JOIN_PASS",flush=True)
        self.midi(None,MIDI())
        self.pcm(None,PCM())
        print(f"DUAL_STAGE {self.name} CALLBACKS_CLEARED",flush=True)

    def unload(self):
        print(f"DUAL_STAGE {self.name} UNLOAD_ENTER",flush=True)
        # Free only after the worker and callbacks have actually stopped.
        kernel=C.WinDLL("kernel32",use_last_error=True)
        free=kernel.FreeLibrary
        free.argtypes=[C.c_void_p]
        free.restype=C.c_int
        if not free(self.lib._handle):
            raise RuntimeError("Runtime FreeLibrary failed")
        print(f"DUAL_STAGE {self.name} UNLOAD_RETURNED",flush=True)
        mapped=C.c_void_p()
        get=kernel.GetModuleHandleExW
        get.argtypes=[C.c_uint,C.c_void_p,C.POINTER(C.c_void_p)]
        get.restype=C.c_int
        if get(6,C.c_void_p(self.lib._handle),C.byref(mapped)):
            raise RuntimeError("Runtime module remains mapped after FreeLibrary")
        print(f"DUAL_STAGE {self.name} UNMAP_PASS",flush=True)


def run_child(source):
    if os.name!="nt":
        raise RuntimeError("Windows only")
    os.environ["SDL_VIDEODRIVER"]="dummy"
    os.environ["SDL_AUDIODRIVER"]="dummy"
    with tempfile.TemporaryDirectory(prefix="awe64-dual-runtime-") as tmp:
        root=Path(tmp)
        a=Runtime(source,root,"A",60,2712)
        b=Runtime(source,root,"B",67,1620)
        if a.lib._handle==b.lib._handle:
            raise RuntimeError("Windows mapped one shared module")
        print("DUAL_DISTINCT_MODULES_PASS",flush=True)
        a.worker.start()
        wait_for(lambda:a.ready() and a.frames>1024,5,"First guest never ready")
        b.worker.start()
        wait_for(lambda:b.ready() and b.frames>1024,5,"Second guest never ready")
        if not a.worker.is_alive() or not b.worker.is_alive():
            raise RuntimeError("Both guests must be concurrently alive")
        if a.audible or b.audible:
            raise RuntimeError("No-note negative control not silent")
        a.trigger=True
        wait_for(lambda:a.audible>4096,.8,"Guest A did not respond")
        if b.audible or b.sent:
            raise RuntimeError("Guest A MIDI/audio leaked into B")
        b.trigger=True
        wait_for(lambda:b.audible>4096,.8,"Guest B did not respond")
        if a.sent!=1 or b.sent!=1:
            raise RuntimeError("MIDI callback routing failed")
        print("DUAL_RUNTIME_MIDI_PCM_ISOLATION_PASS",flush=True)
        # A's stop flag, SDL shutdown and unload must not stop B.
        a.stop_join()
        before=b.audible
        wait_for(lambda:b.audible>before+2048,1.0,
                 "Stopping A interrupted B audio")
        if not b.worker.is_alive():
            raise RuntimeError("Stopping A killed guest B")
        print("DUAL_SURVIVOR_AFTER_JOIN_PASS",flush=True)
        a.unload()
        before=b.audible
        wait_for(lambda:b.audible>before+2048,1.0,
                 "Unloading A interrupted B audio")
        if not b.worker.is_alive():
            raise RuntimeError("Unloading A killed guest B")
        print("DUAL_SURVIVOR_AFTER_UNLOAD_PASS",flush=True)
        b.stop_join()
        b.unload()
        print("DUAL_RUNTIME_INDEPENDENT_SHUTDOWN_PASS",flush=True)
        print("DUAL_RUNTIME_SYNTHETIC_RESEARCH_PASS; NOT_VST3_OR_CREATIVE",flush=True)


if __name__=="__main__":
    path=Path(sys.argv[2]).resolve(strict=True)
    if sys.argv[1]=="--child":
        faulthandler.enable(all_threads=True)
        faulthandler.dump_traceback_later(25)
        try:
            run_child(path)
        finally:
            faulthandler.cancel_dump_traceback_later()
    elif sys.argv[1]=="--parent":
        p=subprocess.run([sys.executable,"-u","-X","faulthandler",str(Path(__file__).resolve()),"--child",str(path)],
                         capture_output=True,text=True,timeout=35)
        print(f"DUAL_CHILD_EXIT=0x{p.returncode&0xffffffff:08x}",flush=True)
        print(p.stdout[-10000:],end="",flush=True)
        print(p.stderr[-14000:],end="",file=sys.stderr,flush=True)
        if p.returncode or "DUAL_RUNTIME_SYNTHETIC_RESEARCH_PASS; NOT_VST3_OR_CREATIVE" not in p.stdout:
            raise SystemExit("Dual DLL research failed; existing single-instance product unchanged")
    else:
        raise SystemExit("Expected --parent or --child")
