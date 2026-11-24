import importlib.util
from pathlib import Path
import unittest
p=Path(__file__).resolve().parents[1]/"integration"/"patch_dosbox_runtime.py"
spec=importlib.util.spec_from_file_location("awe64_dosbox_patcher",p)
patch=importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)

class DosBoxPatchTests(unittest.TestCase):
    def test_emu_port_hw(self):
        s=('#include <string>\nclass EMU8000_Device {\n'
           '\t/* Emulated time in chip frames (44100 Hz) since the reset. */\n'
           'IO_WriteHandleObject WriteHandler[EMU_PORT_GROUPS];\n'
           '\tchan = MixerChan.Install(emu8000_callback, EMU_RATE, EMU8000_MIXER_CHANNEL);\n'
           'void EMU8000_Device::Write(Bitu port, Bitu val, Bitu iolen) {\n\tSync();\n}\n'
           'void EMU8000_Device::Mix(Bitu len) {\n\t/* synthetic mixer anchor */\n}')
        r=patch.patch_emu(s)
        self.assertIn('AweHostRead.Install(0xBEE0u',r)
        self.assertIn('AweHostWrite.Install(0xBEF0u',r)
        self.assertIn('AWE64_HostRegisterIO',r)
        self.assertIn('AWE64_SetAbsoluteSends',r)
        self.assertIn('ApplyAbsoluteSends();',r)
        self.assertIn('AbsoluteSendWord(port,val&0xffffu)',r)
        with self.assertRaises(ValueError):patch.patch_emu(r)
    def test_true_pcm_tap(self):
        s=('#define MIXER_SSIZE 4\n'
           'SDL_AudioDeviceID SDL2_AudioDevice = 0; /* valid IDs are 2 or higher, 1 for compat, 0 is never a valid ID */\n'
           'static void MIXER_Mix(void) {\n    MIXER_FillUp();\n}\n\n'
           'static void SDLCALL MIXER_CallBack(void*,Uint8*,int) {\n    }\n}\n\nstd::string mixerinfo() {\n'
           '    if (mixer.nosound) {\n        SDL_PauseAudioDevice(SDL2_AudioDevice, 0);\n'
           '        mixer.prebuffer_samples = ((unsigned int)ms * (unsigned int)mixer.freq) / 1000u;')
        r=patch.patch_mixer(s)
        self.assertIn('reinterpret_cast<const int16_t*>(stream)',r)
        self.assertIn('awe64_pcm_receiver.load',r)
        self.assertIn('SDL_AudioInit("dummy")',r)
        self.assertIn('AWE64_IsInternalAudioOnly',r)
        self.assertGreater(r.index('AWE64_IsInternalAudioOnly'),r.index('SDL_AudioDeviceID SDL2_AudioDevice = 0'))
        self.assertIn('mixer.nosound=true;',r)
        self.assertIn('AWE64_SetGuestClockPCM',r)
        self.assertIn('MIXER_CallBack(nullptr,reinterpret_cast<Uint8*>(pcm),n*MIXER_SSIZE)',r)
        self.assertIn('if(!awe64_guest_clock_pcm.load(std::memory_order_acquire))',r)
        self.assertIn('mixer.prebuffer_samples=0;',r)
        with self.assertRaises(ValueError):patch.patch_mixer(r)
    def test_real_creative_unchanged_mpu401_input(self):
        s='#include <assert.h>\nextern bool enable_slave_pic;\n' + (
          'static void ClrQueue(void) {\n'
          'static Bitu MPU401_ReadStatus(Bitu port,Bitu iolen) {\n'
          'if (!MIDI_Available()) return;')
        r=patch.patch_mpu(s)
        self.assertIn('AWE64_HostRegisterMidi(',r)
        self.assertIn('AWE64_PumpGuestMidi()',r)
        self.assertIn('MPU401_QUEUE-3',r)
        self.assertIn('if(mpu.mode!=M_UART) {',r)
        self.assertIn('AWE64_IsGuestMidiReady()',r)
        self.assertIn('awe64_guest_midi_ready.store(false',r)
        self.assertIn('if(mpu.queue_used!=0)return;',r)
        self.assertLess(r.index('if(mpu.queue_used!=0)return;'),r.index('auto pull=awe64_mpu_pull.load'))
        with self.assertRaises(ValueError):patch.patch_mpu(r)
    def test_guest_entry(self):
        s='void GFX_Events() {}\n| SDL_WINDOW_SHOWN | (SDL2_resize_enable ? SDL_WINDOW_RESIZABLE : 0)\nint main(int argc, char* argv[]) SDL_MAIN_NOEXCEPT {}\n            DOSBOX_RunMachine();\n        } catch (int x) {'
        r=patch.patch_main(s)
        self.assertIn('AWE64_StartGuest(',r)
        self.assertIn('return main(argc,argv);',r)
        self.assertIn('AWE64_RequestStop',r)
        self.assertIn('| SDL_WINDOW_HIDDEN |',r)
        self.assertNotIn('| SDL_WINDOW_SHOWN |',r)
        self.assertNotIn('awe64_guest_stop.store(false',r)
        self.assertIn('if(awe64_guest_can_stop && awe64_guest_stop.load',r)
        self.assertIn('AWE64GuestRunScope awe64_run_scope;',r)
        self.assertIn('if(awe64_guest_stop.load(std::memory_order_acquire))throw(0);',r)
        self.assertIn('~AWE64GuestRunScope() { awe64_guest_can_stop=false; }',r)
    def test_sdl2_static_library_cannot_supply_dll_crt_entry(self):
        original='#if (!defined(HAVE_LIBC) || defined(__WATCOMC__)) && !defined(SDL_STATIC_LIB)'
        patched=patch.patch_sdl2_static(original)
        self.assertIn('#define SDL_STATIC_LIB 1',patched)
        self.assertIn(original,patched)
        with self.assertRaises(ValueError):
            patch.patch_sdl2_static(patched)
    def test_native_win_x64_dll(self):
        cond="'$(Configuration)|$(Platform)'=='Release SDL2|x64'"
        s=f'  <PropertyGroup Condition="{cond}" Label="Configuration">\n    <ConfigurationType>Application</ConfigurationType>\n  </PropertyGroup>\n  <PropertyGroup Condition="{cond}">\n    <OutDir>x</OutDir>\n  </PropertyGroup>\n  <ItemDefinitionGroup Condition="{cond}">\n    <ClCompile><PreprocessorDefinitions>WIN32;C_SDL2=1</PreprocessorDefinitions></ClCompile>\n    <Link><AdditionalDependencies>kernel32.lib;%(AdditionalDependencies)</AdditionalDependencies></Link>\n  </ItemDefinitionGroup>'
        r=patch.patch_project(s)
        self.assertIn('<ConfigurationType>DynamicLibrary</ConfigurationType>',r)
        self.assertIn('AWE64_VST3_RUNTIME;WIN32;C_SDL2=1',r)
        self.assertIn('libvcruntime.lib;libucrt.lib;libcmt.lib;kernel32.lib',r)
        with self.assertRaises(ValueError):patch.patch_project(r)
if __name__=="__main__":
    unittest.main()
