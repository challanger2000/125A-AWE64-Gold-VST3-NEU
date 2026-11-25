"""Audit only: real pinned DOSBox DLL, synthetic missing-ROM/temp-file fault.
No Creative file or network download. -test suppresses ROM download question.
The expected fatal error dialog is dismissed only in our isolated child.
"""
import ctypes, os, subprocess, sys, tempfile, time
from ctypes import wintypes
from pathlib import Path

def child(dll_path, root, mode):
    # TEMP/TMP is set before this child starts, so the original Win32
    # GetTempPathA sees the isolated folder, not the host's temp folder.
    buf=ctypes.create_unicode_buffer(32768)
    n=ctypes.windll.kernel32.GetTempPathW(len(buf),buf)
    if not n or Path(buf.value).resolve()!=root.resolve():
        raise RuntimeError('Unexpected GetTempPathW '+buf.value)
    rom=root/'awe32.raw'
    if mode=='positive':
        rom.write_bytes(bytes(1024*1024))
    else:
        # The upstream fallback uses this fixed filename. A directory
        # at that filename makes fopen("wb") fail deterministically.
        (root/'dosbox-x-awe32-empty.rom').mkdir()
        if rom.exists():raise RuntimeError('Missing ROM fault invalid')
    os.environ['EMU8K_TRACE']=str(root/'trace.txt')
    args=['AWE64DOSBoxRuntime','-noconfig','-nogui','-nomenu',
          '-set','sdl waitonerror=false','-set','sblaster sbtype=sbawe',
          '-set','sblaster awe32ram=28672','-set','sblaster awe32rom='+str(rom),
          '-set','midi mpu401=intelligent','-set','mixer rate=44100',
          '-c','AWEUTIL /S','-c','exit']
    # Test mode suppresses the download only in the missing-ROM case.
    # The positive control is the normal previously proven AWEUTIL gate.
    if mode=='missing': args.insert(4,'-test')
    with os.add_dll_directory(str(dll_path.parent)):
        dll=ctypes.WinDLL(str(dll_path))
        fn=dll.AWE64_StartGuest
        fn.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_char_p)];fn.restype=ctypes.c_int
        enc=[a.encode('mbcs') for a in args]
        argv=(ctypes.c_char_p*(len(enc)+1))(*enc,None)
        print('AUDIT_GUEST_CALL_ENTER '+mode,flush=True)
        rc=fn(len(enc),argv)
        print('AUDIT_HOST_SURVIVED rc='+str(rc),flush=True)
    return rc

def parent(dll):
    if sys.platform!='win32':raise RuntimeError('Windows required')
    u=ctypes.WinDLL('user32',use_last_error=True)
    callback=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
    u.EnumWindows.argtypes=[callback,wintypes.LPARAM];u.EnumWindows.restype=wintypes.BOOL
    u.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
    u.GetWindowThreadProcessId.restype=wintypes.DWORD
    u.GetWindowTextW.argtypes=[wintypes.HWND,wintypes.LPWSTR,ctypes.c_int]
    u.GetWindowTextW.restype=ctypes.c_int
    u.PostMessageW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM]
    u.PostMessageW.restype=wintypes.BOOL
    u.GetDlgItem.argtypes=[wintypes.HWND,ctypes.c_int];u.GetDlgItem.restype=wintypes.HWND
    u.EnumChildWindows.argtypes=[wintypes.HWND,callback,wintypes.LPARAM];u.EnumChildWindows.restype=wintypes.BOOL
    u.SendMessageTimeoutW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM,wintypes.UINT,wintypes.UINT,ctypes.POINTER(ctypes.c_size_t)]
    u.SendMessageTimeoutW.restype=wintypes.LPARAM
    with tempfile.TemporaryDirectory(prefix='awe64_fatal_audit_') as folder:
        root=Path(folder)
        outcomes={}
        for mode in ('positive','missing'):
            case=root/mode;case.mkdir()
            env=dict(os.environ);env.update(TEMP=str(case),TMP=str(case),SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
            with (case/'stdout.txt').open('w+b') as out,(case/'stderr.txt').open('w+b') as err:
                p=subprocess.Popen([sys.executable,str(Path(__file__).resolve()),'--child',str(dll),str(case),mode],
                                   cwd=case,env=env,stdout=out,stderr=err)
                seen=set();dialog_text=[]
                @callback
                def enumerate_dialog(hwnd,_):
                    pid=wintypes.DWORD()
                    u.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
                    if pid.value!=p.pid:return True
                    title=ctypes.create_unicode_buffer(256);u.GetWindowTextW(hwnd,title,len(title))
                    if title.value=='E_Exit':
                        if hwnd not in seen:
                            seen.add(hwnd);print('AUDIT_FATAL_DIALOG_OBSERVED child='+str(p.pid),flush=True)
                            @callback
                            def read_child(control,_):
                                b=ctypes.create_unicode_buffer(4096);result=ctypes.c_size_t()
                                u.SendMessageTimeoutW(control,0x000d,len(b),ctypes.addressof(b),2,500,ctypes.byref(result))
                                if b.value:
                                    dialog_text.append(b.value);print('AUDIT_FATAL_DIALOG_TEXT '+b.value,flush=True)
                                return True
                            u.EnumChildWindows(hwnd,read_child,0)
                        button=u.GetDlgItem(hwnd,1)
                        if button:u.PostMessageW(button,0x00f5,0,0) # BM_CLICK on IDOK only
                        u.PostMessageW(hwnd,0x0111,1,button or 0) # WM_COMMAND, button HWND
                    return True
                deadline=time.monotonic()+35
                while p.poll() is None and time.monotonic()<deadline:
                    u.EnumWindows(enumerate_dialog,0);time.sleep(.1)
                timed=p.poll() is None
                if timed:p.kill()
                rc=p.wait()
                out.seek(0);err.seek(0)
                logs=out.read().decode('utf-8','replace')+'\n'+err.read().decode('utf-8','replace')
                # Synthetic only; preserve exact diagnostic lines, not giant logs.
                print('AUDIT_CASE',mode,'exit='+str(rc),'timeout='+str(timed),'fatal_dialog='+str(bool(seen)),flush=True)
                logs+='\n'+'\n'.join(dialog_text)
                for line in logs.splitlines():
                    if 'AUDIT_' in line or 'E_Exit' in line or 'cannot create a temporary empty ROM' in line:
                        print(line,flush=True)
                outcomes[mode]=(rc,timed,bool(seen),logs,case)
        rc,timed,dialog,logs,case=outcomes['positive']
        trace=case/'trace.txt'
        writes=sum(1 for line in trace.read_text(errors='replace').splitlines() if line and line[0].isdigit()) if trace.is_file() else 0
        if timed or rc or 'AUDIT_HOST_SURVIVED rc=0' not in logs or writes<16:
            print('AUDIT_FATAL_POSITIVE_CONTROL NOT_VERIFIED writes='+str(writes));return 2
        print('AUDIT_FATAL_POSITIVE_CONTROL PASS writes='+str(writes),flush=True)
        rc,timed,dialog,logs,case=outcomes['missing']
        expected='EMU8000: cannot create a temporary empty ROM' in logs
        if not timed and rc==1 and expected and 'AUDIT_HOST_SURVIVED' not in logs:
            print('AUDIT_A22_REAL_DLL_HOST_TERMINATED FAIL',flush=True);return 1
        if timed and expected and dialog:
            print('AUDIT_A22_REAL_DLL_MODAL_HANG FAIL (process exit not observed)',flush=True);return 1
        if not timed and 'AUDIT_HOST_SURVIVED' in logs:
            print('AUDIT_A22_REAL_DLL NOT_VERIFIED (fatal path not independently observed)',flush=True);return 2
        print('AUDIT_A22_REAL_DLL NOT_VERIFIED (unexpected failure)',flush=True);return 2

if __name__=='__main__':
    if len(sys.argv)==5 and sys.argv[1]=='--child':
        sys.exit(child(Path(sys.argv[2]),Path(sys.argv[3]),sys.argv[4]))
    if len(sys.argv)!=2:raise SystemExit('usage: audit_dosbox_fatal.py DLL')
    sys.exit(parent(Path(sys.argv[1]).resolve(strict=True)))
