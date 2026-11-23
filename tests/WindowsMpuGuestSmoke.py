"""Windows integration test: original DOSBox-X guest, actual 16-bit hardware
MPU UART port I/O, and the real host callback, without copyrighted assets.
"""
import ctypes
import os
import pathlib
import subprocess
import sys
import tempfile
from MakeMpuCom import build_probe

def run_child(dllpath):
    os.environ['SDL_VIDEODRIVER']="dummy"
    with tempfile.TemporaryDirectory(prefix="awe64-original-mpu-") as tmp:
        folder=pathlib.Path(tmp)
        (folder/"MPUPROBE.COM").write_bytes(build_probe())
        with os.add_dll_directory(str(dllpath.parent)):
            dll=ctypes.WinDLL(str(dllpath))
            register=dll.AWE64_HostRegisterMidi
            fn=ctypes.CFUNCTYPE(ctypes.c_uint,ctypes.c_void_p,
                                ctypes.POINTER(ctypes.c_ubyte),ctypes.c_uint)
            register.argtypes=[ctypes.c_void_p,fn]
            register.restype=None
            called=[]
            expected=b"\x90\x3c\x64"  # NoteOn ch0 note60 velocity100
            @fn
            def feed(ctx,buffer,limit):
                if limit<3 or called:return 0
                for i,x in enumerate(expected):buffer[i]=x
                called.append(1)
                return len(expected)
            register(None,feed)
            try:
                start=dll.AWE64_StartGuest
                start.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
                start.restype=ctypes.c_int
                args=["AWE64DOSBoxRuntime","-noconfig","-nogui","-nomenu",
                      "-silent","-set","sblaster sbtype=sb16",
                      "-set","midi mpu401=intelligent","-c",
                      'mount c "'+str(folder)+'"','-c',"c:","-c","MPUPROBE.COM",
                      "-c","exit"]
                a=[x.encode("utf-8") for x in args]
                argv=(ctypes.c_char_p*(len(a)+1))(*a,None)
                result=start(len(a),argv)
                output=folder/"MIDI.BIN"
                if result!=0:raise RuntimeError("Original DOSBox main exit "+str(result))
                if not output.is_file():raise RuntimeError("Real DOS COM produced no MIDI.BIN")
                actual=output.read_bytes()
                if actual!=expected:
                    raise RuntimeError("MPU401 guest received "+actual.hex()+
                                       ", expected "+expected.hex())
                if len(called)!=1:raise RuntimeError("Real host MIDI pull was not consumed exactly once")
                print("ORIGINAL_MPU401_16BIT_GUEST_MIDI=903c64",flush=True)
            finally:
                register(None,fn())  # deregister without leaving Python callback alive

def run_parent(dllpath):
    result=subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),
                          "--child",str(dllpath)],capture_output=True,text=True,
                          timeout=40,check=False)
    print("CHILD RETURN:",result.returncode)
    print("CHILD STDOUT:",result.stdout[-4500:])
    print("CHILD STDERR:",result.stderr[-4500:])
    if result.returncode or "ORIGINAL_MPU401_16BIT_GUEST_MIDI=903c64" not in result.stdout:
        raise SystemExit("Real 16-bit MPU401 MIDI-to-DOS regression FAILED")

if __name__=="__main__":
    if os.name!="nt":raise SystemExit("Windows-only DOSBox native DLL integration test")
    if len(sys.argv)!=3:raise SystemExit("Usage: WindowsMpuGuestSmoke.py --parent|--child DLL")
    path=pathlib.Path(sys.argv[2]).resolve(strict=True)
    if sys.argv[1]=="--parent":run_parent(path)
    elif sys.argv[1]=="--child":run_child(path)
    else:raise SystemExit("Invalid mode")
