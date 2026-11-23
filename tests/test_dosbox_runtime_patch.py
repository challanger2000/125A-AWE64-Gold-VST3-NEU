import importlib.util
from pathlib import Path
import unittest
p=Path(__file__).resolve().parents[1]/"integration"/"patch_dosbox_runtime.py"
spec=importlib.util.spec_from_file_location("awe64_dosbox_patcher",p)
patch=importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)

class DosBoxPatchTests(unittest.TestCase):
    def test_emu_port_hw(self):
        s='#include <string>\nclass EMU8000_Device {\nIO_WriteHandleObject WriteHandler[EMU_PORT_GROUPS];\n\tchan = MixerChan.Install(emu8000_callback, EMU_RATE, EMU8000_MIXER_CHANNEL);'
        r=patch.patch_emu(s)
        self.assertIn('AweHostRead.Install(0xBEE0u',r)
        self.assertIn('AweHostWrite.Install(0xBEF0u',r)
        self.assertIn('AWE64_HostRegisterIO',r)
        with self.assertRaises(ValueError):patch.patch_emu(r)
    def test_true_pcm_tap(self):
        s='#define MIXER_SSIZE 4\n    }\n}\n\nstd::string mixerinfo() {'
        r=patch.patch_mixer(s)
        self.assertIn('reinterpret_cast<const int16_t*>(stream)',r)
        self.assertIn('awe64_pcm_receiver.load',r)
        with self.assertRaises(ValueError):patch.patch_mixer(r)
    def test_guest_entry(self):
        s='int main(int argc, char* argv[]) SDL_MAIN_NOEXCEPT {}'
        r=patch.patch_main(s)
        self.assertIn('AWE64_StartGuest(',r)
        self.assertIn('return main(argc,argv);',r)
    def test_native_win_x64_dll(self):
        cond="'$(Configuration)|$(Platform)'=='Release|x64'"
        s=f'  <PropertyGroup Condition="{cond}" Label="Configuration">\n    <ConfigurationType>Application</ConfigurationType>\n  </PropertyGroup>\n  <PropertyGroup Condition="{cond}">\n    <OutDir>x</OutDir>\n  </PropertyGroup>\n  <ItemDefinitionGroup Condition="{cond}">\n    <ClCompile><PreprocessorDefinitions>WIN32</PreprocessorDefinitions></ClCompile>\n  </ItemDefinitionGroup>'
        r=patch.patch_project(s)
        self.assertIn('<ConfigurationType>DynamicLibrary</ConfigurationType>',r)
        self.assertIn('AWE64_VST3_RUNTIME;WIN32',r)
        with self.assertRaises(ValueError):patch.patch_project(r)
if __name__=="__main__":
    unittest.main()
