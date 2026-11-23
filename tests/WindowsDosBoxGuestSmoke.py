"""Windows-only proof that the ORIGINAL DOSBox-X main can enter and exit
inside the calling process, without starting a separate emulator executable.

This checks ONLY the DOSBox program bootstrap/lifecycle, using sb16 mode to
avoid requiring proprietary ROM/Creative files in CI. No AWE PCM is claimed.
"""
import ctypes
import os
import pathlib
import subprocess
import sys
import threading
import time

def child(dllpath):
    os.environ['SDL_VIDEODRIVER']='dummy'
    with os.add_dll_directory(str(dllpath.parent)):
        lib=ctypes.WinDLL(str(dllpath))
        fn=lib.AWE64_StartGuest
        fn.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
        fn.restype=ctypes.c_int
        # No original Creative software on public CI. Full PC emulator starts
        # in sb16 mode, executes DOSBox shell 'exit', and must return to Python.
        argv=[b'AWE64DOSBoxRuntime',b'-silent',b'-noconfig',
              b'-set',b'sblaster sbtype=sb16',b'-c',b'exit']
        native=(ctypes.c_char_p*(len(argv)+1))(*argv,None)
        code=fn(len(argv),native)
        print('ORIGINAL_DOSBOX_MAIN_RETURNED='+str(code),flush=True)
        if code!=0:raise SystemExit('DOSBox main returned a failure')
    print('ORIGINAL_DOSBOX_CALL_RETURNED',flush=True)

def parent(dllpath):
    result=subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),
                           '--child',str(dllpath)],capture_output=True,text=True,
                          timeout=35,check=False)
    print('CHILD EXIT:',result.returncode)
    print('CHILD STDOUT:',result.stdout[-5000:])
    print('CHILD STDERR:',result.stderr[-5000:])
    if result.returncode or 'ORIGINAL_DOSBOX_MAIN_RETURNED=0' not in result.stdout:
        raise SystemExit('In-process DOSBox guest bootstrap/exit smoke failed')
    if 'ORIGINAL_DOSBOX_CALL_RETURNED' not in result.stdout:
        raise SystemExit('In-process DOSBox call did not return to Python')


def stop_child(dllpath):
    # The actual VST3 host starts main() on a background worker thread.
    os.environ['SDL_VIDEODRIVER']='dummy'
    os.environ['SDL_AUDIODRIVER']='dummy'
    with os.add_dll_directory(str(dllpath.parent)):
        lib=ctypes.WinDLL(str(dllpath))
        start=lib.AWE64_StartGuest
        start.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
        start.restype=ctypes.c_int
        stop=lib.AWE64_RequestStop
        stop.argtypes=[]
        stop.restype=None
        check=lib.AWE64_IsInternalAudioOnly
        check.argtypes=[]
        check.restype=ctypes.c_int
        args=[b'AWE64DOSBoxRuntime',b'-noconfig',b'-nomenu',b'-noconsole',
              b'-set',b'sblaster sbtype=sb16']
        argv=(ctypes.c_char_p*(len(args)+1))(*args,None)
        result=[]
        def launch():
            result.append(start(len(args),argv))
        thread=threading.Thread(target=launch,daemon=True)
        thread.start()
        deadline=time.monotonic()+8.0
        while thread.is_alive() and check()!=1 and time.monotonic()<deadline:
            time.sleep(.05)
        if not thread.is_alive():
            raise SystemExit('DOSBox background worker exited before stop request')
        if check()!=1:
            raise SystemExit('Physical/audio backend not isolated: expected SDL dummy output')
        print('ORIGINAL_DOSBOX_INTERNAL_AUDIO_DRIVER=dummy',flush=True)
        stop()
        thread.join(18)
        if thread.is_alive():
            raise SystemExit('DOSBox background worker did not join after AWE64_RequestStop')
        if result!=[0]:
            raise SystemExit('DOSBox background worker failed: '+repr(result))
    print('ORIGINAL_DOSBOX_WORKER_STOPPED_AND_JOINED',flush=True)

def stop_parent(dllpath):
    result=subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),
                           '--stop-child',str(dllpath)],capture_output=True,
                          text=True,timeout=28,check=False)
    print('STOP CHILD EXIT:',result.returncode)
    print('STOP STDOUT:',result.stdout[-5000:])
    print('STOP STDERR:',result.stderr[-5000:])
    if result.returncode or 'ORIGINAL_DOSBOX_WORKER_STOPPED_AND_JOINED' not in result.stdout:
        raise SystemExit('Original DOSBox-X thread cancellation failed')

if __name__=='__main__':
    if os.name!='nt':raise SystemExit('Windows is required')
    if len(sys.argv)<3:
        raise SystemExit('Usage: WindowsDosBoxGuestSmoke.py --parent|--child DLL')
    dllpath=pathlib.Path(sys.argv[2]).resolve(strict=True)
    if sys.argv[1]=='--child':child(dllpath)
    elif sys.argv[1]=='--parent':parent(dllpath)
    elif sys.argv[1]=='--stop-child':stop_child(dllpath)
    elif sys.argv[1]=='--stop-parent':stop_parent(dllpath)
    else:raise SystemExit('Unknown mode')
