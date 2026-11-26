"""Windows/portable adversarial package boundary checks, SYNTHETIC DATA ONLY."""
import hashlib, shutil, subprocess, sys, tempfile
from pathlib import Path
from unittest.mock import patch
import prepare_private_vst3 as stage

root=Path(stage.__file__).resolve().parents[1]
private=root/'PRIVATE'
private.mkdir(exist_ok=True)
failures=[]
with tempfile.TemporaryDirectory(dir=root,prefix='audit_fake_') as folder:
    tmp=Path(folder)
    source=tmp/'125A AWE64 Gold.vst3'
    bin_dir=source/'Contents'/'x86_64-win';bin_dir.mkdir(parents=True)
    (bin_dir/'125A AWE64 Gold.vst3').write_bytes(b'A'*2048)
    (bin_dir/'Engine.dll').write_bytes(b'B'*2048)
    art=source/'Contents'/'Resources'/'125A_P165_Original_Faceplate_1000x708.png'
    art.parent.mkdir(parents=True);art.write_bytes(b'SYNTHETIC_GUI')
    assets=tmp/'fake-assets';assets.mkdir()
    specs={}
    for name in stage.REQUIRED:
        data=('FAKE_AUDIT_ONLY_'+name).encode()
        (assets/name).write_bytes(data)
        specs[name]=('sha256',hashlib.sha256(data).hexdigest(),len(data))
    art_spec=('sha256',hashlib.sha256(art.read_bytes()).hexdigest(),None)
    inside=private/(tmp.name+'.vst3')
    link=private/(tmp.name+'_junction')
    def verify_payload(dest):
        for name in specs:
            if (dest/'Contents'/'Resources'/'ROM'/name).read_bytes()!=(assets/name).read_bytes():
                raise RuntimeError('Payload mismatch: '+name)
    def probe(label,destination):
        try:stage.assemble(source,assets,destination)
        except ValueError:
            print(label,'PASS rejected',flush=True);return
        verify_payload(destination)
        if destination.resolve().is_relative_to(private.resolve()):
            raise RuntimeError('Fixture did not escape private directory')
        print(label,'FAIL accepted outside PRIVATE',flush=True);failures.append(label)
    try:
        with patch.object(stage,'REQUIRED',specs),patch.object(stage,'FACEPLATE',art_spec):
            stage.assemble(source,assets,inside);verify_payload(inside)
            if not inside.resolve().is_relative_to(private.resolve()):raise RuntimeError('positive control')
            print('AUDIT_PACKAGE_POSITIVE_CONTROL PASS',flush=True)
            probe('AUDIT_PACKAGE_DIRECT_OUTSIDE',tmp/'direct.vst3')
            probe('AUDIT_A18_DOTDOT',private/'..'/tmp.name/'dotdot.vst3')
            if sys.platform=='win32':
                subprocess.run(['cmd.exe','/d','/c','mklink','/J',str(link),str(tmp)],check=True)
            else:
                link.symlink_to(tmp,target_is_directory=True)
            if link.resolve()!=tmp.resolve():raise RuntimeError('junction fixture invalid')
            probe('AUDIT_A18_JUNCTION_PARENT',link/'linked.vst3')
    finally:
        if link.exists() or link.is_symlink():
            if sys.platform=='win32':link.rmdir()
            else:link.unlink()
        shutil.rmtree(inside,ignore_errors=True)
    # Disposable repository only, no staging/pushing in actual product repo.
    gitroot=tmp/'git-fixture';gitroot.mkdir()
    shutil.copyfile(root/'.gitignore',gitroot/'.gitignore')
    subprocess.run(['git','init','-q',str(gitroot)],check=True)
    for prefix in ('PRIVATE','example.vst3/Contents/Resources/ROM','build-vst3/VST3/Release/125A AWE64 Gold.vst3/Contents/Resources/ROM'):
        for name in specs:
            path=gitroot/prefix/name;path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(b'FAKE_AUDIT_ONLY')
            status=subprocess.run(['git','-C',str(gitroot),'check-ignore','-q','--',str(path.relative_to(gitroot))]).returncode
            if status not in (0,1):raise RuntimeError('git check-ignore infrastructure')
            label='AUDIT_A21_GITIGNORE_'+prefix.replace('/','_')+'_'+name
            print(label,'PASS' if status==0 else 'FAIL unignored',flush=True)
            if status:failures.append(label)
print('AUDIT_NATIVE_PACKAGE_EXECUTED failures='+str(len(failures)),flush=True)
sys.exit(bool(failures))
