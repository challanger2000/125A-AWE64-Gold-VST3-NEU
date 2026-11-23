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

def child(dllpath):
    os.environ['SDL_VIDEODRIVER']='dummy'
    os.environ['SDL_AUDIODRIVER']='dummy'
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
    print('ORIGINAL_DOSBOX_DLL_UNLOADED',flush=True)

def parent(dllpath):
    result=subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),
                           '--child',str(dllpath)],capture_output=True,text=True,
                          timeout=35,check=False)
    print('CHILD EXIT:',result.returncode)
    print('CHILD STDOUT:',result.stdout[-5000:])
    print('CHILD STDERR:',result.stderr[-5000:])
    if result.returncode or 'ORIGINAL_DOSBOX_MAIN_RETURNED=0' not in result.stdout:
        raise SystemExit('In-process DOSBox guest bootstrap/exit smoke failed')
    if 'ORIGINAL_DOSBOX_DLL_UNLOADED' not in result.stdout:
        raise SystemExit('In-process DOSBox unloading not confirmed')

if __name__=='__main__':
    if os.name!='nt':raise SystemExit('Windows is required')
    if len(sys.argv)<3:
        raise SystemExit('Usage: WindowsDosBoxGuestSmoke.py --parent|--child DLL')
    dllpath=pathlib.Path(sys.argv[2]).resolve(strict=True)
    if sys.argv[1]=='--child':child(dllpath)
    elif sys.argv[1]=='--parent':parent(dllpath)
    else:raise SystemExit('Unknown mode')
