"""No proprietary Creative data is used by these portable staging tests."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

source=Path(__file__).resolve().parent/"prepare_private_vst3.py"
spec=importlib.util.spec_from_file_location("awe64_private_stage",source)
stage=importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage)

def blob(data):
    return hashlib.sha1(("blob "+str(len(data))+"\0").encode()+data).hexdigest()
def fake(path,data):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(data)

class PrivateStagingTests(unittest.TestCase):
    def test_hashing_and_wrong_originals_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/"AWE32.raw"
            fake(file,b"synthetic not a Creative ROM")
            self.assertEqual(stage.file_hash(file,"git-blob"),blob(file.read_bytes()))
            self.assertEqual(stage.file_hash(file,"sha256"),hashlib.sha256(file.read_bytes()).hexdigest())
            with self.assertRaises(ValueError):
                stage.verify(file,("sha256","0"*64,None))
            with self.assertRaises(ValueError):
                stage.verify(file,("git-blob",blob(file.read_bytes()),1_048_576))
    def test_private_package_with_only_synthetic_fixture(self):
        root=source.parents[1]
        with tempfile.TemporaryDirectory(dir=root/"PRIVATE" if (root/"PRIVATE").exists() else None) as _:
            pass
        private=root/"PRIVATE"
        private.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=private) as work:
            tmp=Path(work)
            bundle=tmp/"AWE64GoldDOSBoxDev.vst3"
            artifact=bundle/"Contents"/"Resources"/"125A_P165_Original_Faceplate_1000x708.png"
            fake(artifact,b"original-GUI-placeholder-test")
            fake(bundle/"Contents"/"x86_64-win"/"AWE64GoldDOSBoxDev.vst3",b"A"*2048)
            fake(bundle/"Contents"/"x86_64-win"/"AWE64DOSBoxRuntime.dll",b"B"*2048)
            inputs=tmp/"source-license-fixtures"
            names={"awe32.raw":b"ROMTEST","DEMO.EXE":b"DEMOTEST","SYNTHGM.SF2":b"SF2TEST"}
            for name,data in names.items():fake(inputs/name,data)
            fake_specs={name:("git-blob",blob(data),len(data)) for name,data in names.items()}
            dest=tmp/"private-build"/"AWE64GoldDOSBoxDev.vst3"
            with patch.object(stage,"REQUIRED",fake_specs),patch.object(stage,"FACEPLATE",
                ("git-blob",blob(artifact.read_bytes()),None)):
                result=stage.assemble(bundle,inputs,dest)
                self.assertEqual(result,dest)
                for name,data in names.items():
                    self.assertEqual((dest/"Contents"/"Resources"/"PRIVATE"/name).read_bytes(),data)
                with self.assertRaises(FileExistsError):
                    stage.assemble(bundle,inputs,dest)
                with self.assertRaises(ValueError):
                    stage.assemble(bundle,inputs,Path(work).parents[1]/"outside.vst3")
if __name__=="__main__":
    unittest.main()
