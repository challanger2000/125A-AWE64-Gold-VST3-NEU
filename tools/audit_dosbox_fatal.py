"""Native regression: exact original DLL rejects invalid explicit AWE ROM
before original main(); the same loaded DLL can then boot a valid fake ROM.
No Creative files, downloads or production audio claims.
"""
import ctypes
import faulthandler
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REJECTED=-12564
ARG_REJECTED=-12565
SDL_INIT_REJECTED=-12566
ARG_CASES={
    'printconf':['-printconf'],
    'resetconf':['-resetconf'],
    'resetmapper':['-resetmapper'],
    'editconf':['-editconf'],
    'unknown-machine':['-set','dosbox machine=invalid'],
    'unsafe-mixer':['-set','mixer blocksize=999999'],
    'option-as-command':['-c','-printconf'],
    'missing-command':['-c'],
    'no-noconfig':[],
}
MODES=('positive','missing','short','oversize','empty','duplicate','absent','directory','sdl-init','host-policy','config-isolation',*ARG_CASES)

def child(dll_path,root,mode):
    faulthandler.enable()
    print('AUDIT_CHILD_BEGIN '+mode,flush=True)
    rom=root/'valid.raw'
    rom.write_bytes(bytes(1024*1024))
    trace=root/'trace.txt'
    os.environ['EMU8K_TRACE']=str(trace)
    def args(path):
        return ['AWE64DOSBoxRuntime','-noconfig','-nogui','-nomenu',
                '-set','sdl waitonerror=false','-set','sblaster sbtype=sbawe',
                '-set','sblaster awe32ram=28672','-set','sblaster awe32rom='+str(path),
                '-set','midi mpu401=intelligent','-set','mixer rate=44100',
                '-c','AWEUTIL /S','-c','exit']
    # Real Windows policy, in a disposable test child only.
    if mode=='host-policy':
        from ctypes import wintypes
        k=ctypes.WinDLL('kernel32',use_last_error=True)
        k.GetCurrentProcess.restype=wintypes.HANDLE
        k.GetPriorityClass.argtypes=[wintypes.HANDLE]
        k.GetPriorityClass.restype=wintypes.DWORD
        k.SetPriorityClass.argtypes=[wintypes.HANDLE,wintypes.DWORD]
        k.SetPriorityClass.restype=wintypes.BOOL
        proc=k.GetCurrentProcess()
        if not k.SetPriorityClass(proc,0x4000):
            raise ctypes.WinError(ctypes.get_last_error())
        sh=ctypes.WinDLL('shcore')
        sh.GetProcessDpiAwareness.argtypes=[wintypes.HANDLE,ctypes.POINTER(ctypes.c_int)]
        sh.GetProcessDpiAwareness.restype=ctypes.c_long
        u=ctypes.WinDLL('user32',use_last_error=True)
        class Sticky(ctypes.Structure):
            _fields_=[('size',wintypes.UINT),('flags',wintypes.DWORD)]
        u.SystemParametersInfoW.argtypes=[wintypes.UINT,wintypes.UINT,ctypes.c_void_p,wintypes.UINT]
        u.SystemParametersInfoW.restype=wintypes.BOOL
        def policy():
            dpi=ctypes.c_int(-1)
            if sh.GetProcessDpiAwareness(proc,ctypes.byref(dpi))!=0:
                raise RuntimeError('GetProcessDpiAwareness failed')
            sticky=Sticky(ctypes.sizeof(Sticky),0)
            if not u.SystemParametersInfoW(0x003a,ctypes.sizeof(sticky),ctypes.byref(sticky),0):
                raise ctypes.WinError(ctypes.get_last_error())
            return (int(k.GetPriorityClass(proc)),dpi.value,int(sticky.flags))
        before=policy()
        if before[0]!=0x4000:raise RuntimeError('priority precondition failed')
        print('AUDIT_HOST_POLICY_BEFORE '+str(before),flush=True)
    if mode=='config-isolation':
        # A standalone config would change the entire process CWD and
        # disable sound. The input is disposable, not an installed config.
        target=root/'wrong-directory'
        target.mkdir()
        config=root/'dosbox-x.conf'
        poison=('[dosbox]\nworking directory option=force\n'
                'working directory default='+str(target)+'\n'
                '[mixer]\nnosound=true\n'
                '[autoexec]\nexit\n').encode('ascii')
        config.write_bytes(poison)
        cwd=Path.cwd()
        if cwd!=root:raise RuntimeError('config isolation CWD precondition')
    with os.add_dll_directory(str(dll_path.parent)):
        dll=ctypes.WinDLL(str(dll_path))
        start=dll.AWE64_StartGuest
        start.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
        start.restype=ctypes.c_int
        def invoke(values):
            encoded=[v.encode('mbcs') for v in values]
            argv=(ctypes.c_char_p*(len(encoded)+1))(*encoded,None)
            return start(len(encoded),argv)
        if mode=='sdl-init':
            print('AUDIT_SDL_INIT_CALL_BEGIN',flush=True)
            rc=invoke(args(rom)+['-awe64-audit-sdl-init'])
            print('AUDIT_SDL_INIT_REJECTION rc='+str(rc),flush=True)
            if rc!=SDL_INIT_REJECTED or trace.exists():return 1
            # This was an actual original main invocation. Dispose that
            # image and start a fresh copy, as VST3 does per activation.
            free=ctypes.WinDLL('kernel32',use_last_error=True).FreeLibrary
            free.argtypes=[ctypes.c_void_p];free.restype=ctypes.c_int
            if not free(dll._handle):return 1
            print('AUDIT_SDL_INIT_DLL_RELEASED',flush=True)
            fresh=root/'fresh-runtime.dll'
            shutil.copyfile(dll_path,fresh)
            dll=ctypes.WinDLL(str(fresh))
            start=dll.AWE64_StartGuest
            start.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
            start.restype=ctypes.c_int
        elif mode in ARG_CASES:
            values=args(rom)+ARG_CASES[mode]
            if mode=='no-noconfig':values.remove('-noconfig')
            rc=invoke(values)
            print('AUDIT_ARG_ENTRY_REJECTION mode='+mode+' rc='+str(rc),flush=True)
            if rc!=ARG_REJECTED or trace.exists():return 1
        elif mode not in ('positive','host-policy','config-isolation'):
            # -test prevents a ROM download prompt if a regression ever
            # accidentally re-enters the upstream standalone fallback.
            invalid=root/'invalid.raw'
            if mode=='short':invalid.write_bytes(b'X')
            elif mode=='oversize':invalid.write_bytes(bytes(1048577))
            elif mode=='directory':invalid.mkdir()
            values=args('' if mode=='empty' else invalid)
            if mode=='duplicate':
                values=args(rom)+['-set','sblaster awe32rom='+str(rom)]
            elif mode=='absent':
                i=next(i for i,v in enumerate(values) if v.startswith('sblaster awe32rom='))
                del values[i-1:i+1]
            values.insert(4,'-test')
            rc=invoke(values)
            print('AUDIT_ROM_ENTRY_REJECTION mode='+mode+' rc='+str(rc),flush=True)
            if rc!=REJECTED or trace.exists():return 1
        rc=invoke(args(rom))
        if mode=='config-isolation':
            unchanged=Path.cwd()==cwd and config.read_bytes()==poison
            print('AUDIT_CONFIG_ISOLATION_UNCHANGED='+str(int(unchanged)),flush=True)
            if not unchanged:return 1
        if mode=='host-policy':
            after=policy()
            print('AUDIT_HOST_POLICY_AFTER '+str(after),flush=True)
            print('AUDIT_HOST_POLICY_UNCHANGED='+str(int(before==after)),flush=True)
            if before!=after:return 1
        writes=sum(1 for line in trace.read_text(errors='replace').splitlines()
                   if line and line[0].isdigit()) if trace.is_file() else 0
        print('AUDIT_ROM_ENTRY_HOST_SURVIVED mode='+mode+' rc='+str(rc)+' writes='+str(writes),flush=True)
        return 0 if rc==0 and writes>=16 else 1

def parent(dll):
    if sys.platform!='win32':raise RuntimeError('Windows required')
    failures=[]
    with tempfile.TemporaryDirectory(prefix='awe64_rom_entry_') as folder:
        for mode in MODES:
            case=Path(folder)/mode;case.mkdir()
            env=dict(os.environ)
            env.update(TEMP=str(case),TMP=str(case),APPDATA=str(case),LOCALAPPDATA=str(case),
                       USERPROFILE=str(case),SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
            try:
                p=subprocess.run([sys.executable,str(Path(__file__).resolve()),'--child',str(dll),str(case),mode],
                    cwd=case,env=env,capture_output=True,text=True,errors='replace',timeout=30)
            except subprocess.TimeoutExpired:
                print('AUDIT_ROM_ENTRY_TIMEOUT '+mode,flush=True)
                failures.append(mode);continue
            logs=p.stdout+'\n'+p.stderr
            for line in logs.splitlines():
                if ('AUDIT_ROM_ENTRY_' in line or 'AUDIT_ARG_ENTRY_' in line or
                    'AWE64_EMBEDDED_ROM_PREFLIGHT_REJECTED' in line or
                    'AWE64_EMBEDDED_ARG_PREFLIGHT_REJECTED' in line or
                    'AUDIT_SDL_INIT_' in line or 'AWE64_EMBEDDED_SDL_' in line or
                    'AUDIT_HOST_POLICY_' in line or 'AUDIT_CONFIG_ISOLATION_' in line):
                    print(line,flush=True)
            ok=(p.returncode==0 and ('AUDIT_ROM_ENTRY_HOST_SURVIVED mode='+mode+' rc=0 writes=') in logs)
            if mode=='sdl-init':
                ok=ok and 'AUDIT_SDL_INIT_REJECTION rc=-12566' in logs
                ok=ok and logs.count('AWE64_EMBEDDED_SDL_INIT_FAILED')==1
                ok=ok and logs.count('AWE64_EMBEDDED_SDL_CLEANUP_RETURN')==1
                ok=ok and logs.count('AUDIT_SDL_INIT_DLL_RELEASED')==1
            elif mode in ARG_CASES:
                ok=ok and ('AUDIT_ARG_ENTRY_REJECTION mode='+mode+' rc='+str(ARG_REJECTED)) in logs
                ok=ok and logs.count('AWE64_EMBEDDED_ARG_PREFLIGHT_REJECTED')==1
            elif mode=='host-policy':
                ok=ok and logs.count('AUDIT_HOST_POLICY_UNCHANGED=1')==1
            elif mode=='config-isolation':
                ok=ok and logs.count('AUDIT_CONFIG_ISOLATION_UNCHANGED=1')==1
                # Actual original AWEUTIL trace gate also required.
                ok=ok and 'CONFIG: Loaded config file:' not in logs
            elif mode!='positive':
                ok=ok and ('AUDIT_ROM_ENTRY_REJECTION mode='+mode+' rc='+str(REJECTED)) in logs
                ok=ok and logs.count('AWE64_EMBEDDED_ROM_PREFLIGHT_REJECTED')==1
            if not ok:
                failures.append(mode)
                print('AUDIT_CHILD_FAILED mode='+mode+' rc='+str(p.returncode)+
                      ' hex='+hex(p.returncode & 0xffffffff),flush=True)
                # Synthetic fixture only: retain actual failure context.
                print(logs[-16000:],flush=True)
            print('AUDIT_ROM_ENTRY_CASE '+mode+' '+('PASS' if ok else 'FAIL'),flush=True)
    print('AUDIT_ROM_ENTRY_FAILURES='+str(failures),flush=True)
    return int(bool(failures))

if __name__=='__main__':
    if len(sys.argv)==5 and sys.argv[1]=='--child':
        sys.exit(child(Path(sys.argv[2]),Path(sys.argv[3]),sys.argv[4]))
    if len(sys.argv)!=2:raise SystemExit('usage: audit_dosbox_fatal.py DLL')
    sys.exit(parent(Path(sys.argv[1]).resolve(strict=True)))
