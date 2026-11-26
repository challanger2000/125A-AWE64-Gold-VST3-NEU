"""Native regression: exact original DLL rejects invalid explicit AWE ROM
before original main(); the same loaded DLL can then boot a valid fake ROM.
No Creative files, downloads or production audio claims.
"""
import ctypes
import os
import subprocess
import sys
import tempfile
from pathlib import Path

REJECTED=-12564
ARG_REJECTED=-12565
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
MODES=('positive','missing','short','oversize','empty','duplicate','absent','directory',*ARG_CASES)

def child(dll_path,root,mode):
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
    with os.add_dll_directory(str(dll_path.parent)):
        dll=ctypes.WinDLL(str(dll_path))
        start=dll.AWE64_StartGuest
        start.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)]
        start.restype=ctypes.c_int
        def invoke(values):
            encoded=[v.encode('mbcs') for v in values]
            argv=(ctypes.c_char_p*(len(encoded)+1))(*encoded,None)
            return start(len(encoded),argv)
        if mode in ARG_CASES:
            values=args(rom)+ARG_CASES[mode]
            if mode=='no-noconfig':values.remove('-noconfig')
            rc=invoke(values)
            print('AUDIT_ARG_ENTRY_REJECTION mode='+mode+' rc='+str(rc),flush=True)
            if rc!=ARG_REJECTED or trace.exists():return 1
        elif mode!='positive':
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
                    'AWE64_EMBEDDED_ARG_PREFLIGHT_REJECTED' in line):
                    print(line,flush=True)
            ok=(p.returncode==0 and ('AUDIT_ROM_ENTRY_HOST_SURVIVED mode='+mode+' rc=0 writes=') in logs)
            if mode in ARG_CASES:
                ok=ok and ('AUDIT_ARG_ENTRY_REJECTION mode='+mode+' rc='+str(ARG_REJECTED)) in logs
                ok=ok and logs.count('AWE64_EMBEDDED_ARG_PREFLIGHT_REJECTED')==1
            elif mode!='positive':
                ok=ok and ('AUDIT_ROM_ENTRY_REJECTION mode='+mode+' rc='+str(REJECTED)) in logs
                ok=ok and logs.count('AWE64_EMBEDDED_ROM_PREFLIGHT_REJECTED')==1
            if not ok:failures.append(mode)
            print('AUDIT_ROM_ENTRY_CASE '+mode+' '+('PASS' if ok else 'FAIL'),flush=True)
    print('AUDIT_ROM_ENTRY_FAILURES='+str(failures),flush=True)
    return int(bool(failures))

if __name__=='__main__':
    if len(sys.argv)==5 and sys.argv[1]=='--child':
        sys.exit(child(Path(sys.argv[2]),Path(sys.argv[3]),sys.argv[4]))
    if len(sys.argv)!=2:raise SystemExit('usage: audit_dosbox_fatal.py DLL')
    sys.exit(parent(Path(sys.argv[1]).resolve(strict=True)))
