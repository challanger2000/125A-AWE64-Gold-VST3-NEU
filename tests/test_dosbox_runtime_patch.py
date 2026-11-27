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
        s+="\n#include <stdlib.h>\n\tunsigned int Base(void) const { return (unsigned int)base; }\n\tstd::string empty_rom;\n\tif (rom.empty()) {\n\t\trom_file = rom;\n\t}\n\n\temu = (emu8k_t *)calloc(1, sizeof(emu8k_t));\n\tif (emu == NULL) E_Exit(\"EMU8000: out of memory\");\n\temu8k_init(emu, (uint16_t)base, ram_kb);\n\twavetable_pos_global = emu->pos;\n\tconst std::string rom = AWE32ROM_Locate(rom_path);\n\temu8000 = new EMU8000_Device(sb_base, rom, ram_kb);"
        r=patch.patch_emu(s)
        self.assertIn('if(!candidate||!candidate->Valid())',r)
        self.assertIn('if(!emu->rom||!emu->empty',r)
        self.assertIn('AWE64_ReportEmuInitFailure();return;',r)
        self.assertIn('AweHostRead.Install(0xBEE0u',r)
        self.assertIn('AweHostWrite.Install(0xBEF0u',r)
        self.assertIn('AWE64_HostRegisterIO',r)
        self.assertIn('AWE64_SetAbsoluteSends',r)
        self.assertIn('ApplyAbsoluteSends();',r)
        self.assertIn('VoiceWrite(port,val&0xffffu)',r)
        self.assertIn('AbsoluteSendWord(port,word)',r)
        self.assertIn('AWE64_SetVoiceControls',r)
        self.assertNotIn(r'\n#ifdef',r)
        self.assertNotIn(r'\nextern uint64_t',r)
        self.assertNotIn('#pragma once',r)
        self.assertIn('\nextern uint64_t AWE64_OverlayMidiContext() noexcept;\n#endif',r)
        self.assertIn('voice<30 && !(emu->voice[voice].ccca&0x04000000u)',r)
        with self.assertRaises(ValueError):patch.patch_emu(r)
    def test_emu_c_partial_initialization(self):
        s = r'''/* The values of emu8k_mod_attack_calc by the phase 0..1 << 21 (it clamps the
 * phase there), each computed at its first use and kept as value + 1 (0 = not
 * yet): the pow of every attack sample is the hot spot of the voice loop. The
 * table (8 MB) is allocated at the first use; without it the value is computed. */
static int32_t *emu8k_mod_attack_cache = NULL;

static inline int32_t
emu8k_mod_attack_value(int32_t phase)
{
    const int32_t index = (phase < 0) ? 0 : (phase > (1 << 21)) ? (1 << 21) : phase;
    if (!emu8k_mod_attack_cache) {
        emu8k_mod_attack_cache = (int32_t *) calloc((1 << 21) + 1, sizeof(int32_t));
        if (!emu8k_mod_attack_cache)
            return emu8k_mod_attack_calc(phase);
    }
    if (!emu8k_mod_attack_cache[index])
        emu8k_mod_attack_cache[index] = emu8k_mod_attack_calc(index) + 1;
    return emu8k_mod_attack_cache[index] - 1;
}
emu8k_mod_attack_value(modenv->value_amp_hz)
emu8k_mod_attack_value(mod_env->value_amp_hz)
/* onboard_ram in kilobytes */
void
emu8k_init
#ifndef EMU8K_ROM_READ
    FILE          *fp;
#endif
#ifndef EMU8K_ROM_READ
    fp = rom_fopen(EMU8K_ROM_PATH, "rb");
    if (!fp)
        fatal("AWE32.RAW not found\n");

#endif
    emu8k->rom = calloc(1024, 1024);
    /* AWE32Emu: also accept a card dump without the leading AWE-DUMP word
     * (1048574 bytes); the missing last word stays zero.
     * remc2: the image comes from rom_read (it may be stored packed), not rom_fopen
     * (EMU8K_ROM_READ, defined by its rom.h). */
#ifdef EMU8K_ROM_READ
    if (rom_read(emu8k->rom, 1048576) < 1048574)
        fatal("emu8k_init(): Error reading data\n");
#else
    if (fread(emu8k->rom, 1, 1048576, fp) < 1048574)
        fatal("emu8k_init(): Error reading data\n");
    fclose(fp);
#endif
    emu8k->empty = calloc(2, BLOCK_SIZE_WORDS);
        emu8k->ram = calloc(1024, onboard_ram);
    emu8k->hwcf3 = 0x00;
}
emu8k_close(emu8k_t *emu8k)
{
        for (pos = emu8k->pos; pos < wavetable_pos_global; pos++) {
            int32_t dat;'''
        patched = patch.patch_emu_c_init(s)
        self.assertIn('if (bad || count != 1048576)', patched)
        self.assertIn('emu8k->empty = failure == 4', patched)
        self.assertIn('emu8k->ram = failure == 5', patched)
        self.assertIn('if (rom_read(emu8k->rom, 1048576) < 1048574)', patched)
        self.assertIn('emu8k->mod_attack_cache = calloc', patched)
        self.assertIn('free(emu8k->mod_attack_cache);', patched)
        self.assertIn('!emu_voice->volumeslide.last', patched)
        self.assertIn('!CCCA_DMA_ACTIVE(emu_voice->ccca)', patched)
        self.assertIn('emu_voice->filt_oct_curr = emu_voice->filt_oct_target;', patched)
        with self.assertRaises(ValueError):
            patch.patch_emu_c_init(patched)

    def test_emu_header_keeps_embedded_cache_per_chip(self):
        s = "typedef struct emu8k_t {\n    emu8k_voice_t voice[32];"
        self.assertIn('#ifdef AWE64_VST3_RUNTIME\n    int32_t *mod_attack_cache;', patch.patch_emu_header(s))

    def test_true_pcm_tap(self):
        s=('#define MIXER_SSIZE 4\n'
           'SDL_AudioDeviceID SDL2_AudioDevice = 0; /* valid IDs are 2 or higher, 1 for compat, 0 is never a valid ID */\n'
           'static void MIXER_Mix(void) {\n    MIXER_FillUp();\n}\n\n'
           'static void SDLCALL MIXER_CallBack(void*,Uint8*,int) {\n'
           '    if (need > 0)\n        mixer.prebuffer_wait = true;\n'
           '    if ((unsigned long)remains >= (mixer.blocksize*2UL)) {\n'
           '    }\n}\n\nstd::string mixerinfo() {\n'
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
        self.assertIn('&& AWE64_HostDemandEnabled()',r)
        self.assertIn('after!=remains-n',r)
        self.assertIn('if(!AWE64_GuestClockEnabled())',r)
        with self.assertRaises(ValueError):patch.patch_mixer(r)
    def test_real_creative_unchanged_mpu401_input(self):
        s='#include <assert.h>\nextern bool enable_slave_pic;\n' + (
          'mpu.queue[pos]=data;\n'
          'ret=mpu.queue[mpu.queue_pos];\n'
          'static void ClrQueue(void) {\n'
          'static Bitu MPU401_ReadStatus(Bitu port,Bitu iolen) {\n'
          'if (!MIDI_Available()) return;')
        r=patch.patch_mpu(s)
        self.assertIn('AWE64_HostRegisterMidi(',r)
        self.assertIn('AWE64_PumpGuestMidi()',r)
        self.assertIn('MPU401_QUEUE-3',r)
        self.assertIn('const bool clocked=AWE64_HostDemandEnabled();',r)
        self.assertIn('if(clocked&&mpu.queue_used!=0)return;',r)
        self.assertIn('if(mpu.mode!=M_UART) {',r)
        self.assertIn('AWE64_IsGuestMidiReady()',r)
        self.assertIn('awe64_overlay_context=tag && mpu.mode==M_UART',r)
        self.assertNotIn(r'\n#ifdef',r)
        self.assertNotIn(r'\n#endif',r)
        self.assertIn('mpu.queue[pos]=data;\n#ifdef AWE64_VST3_RUNTIME',r)
        self.assertIn('ret=mpu.queue[mpu.queue_pos];\n#ifdef AWE64_VST3_RUNTIME',r)
        self.assertIn('awe64_overlay_context=0; // previous synchronous DEMO handler returned',r)
        self.assertIn('awe64_guest_midi_ready.store(false',r)
        self.assertIn('if(mpu.queue_used!=0)return;',r)
        self.assertLess(r.index('if(mpu.queue_used!=0)return;'),r.index('auto pull=awe64_mpu_pull.load'))
        with self.assertRaises(ValueError):patch.patch_mpu(r)
    def test_host_demand_clock_opt_in(self):
        src='static Bitu Normal_Loop(void) {\n                GFX_Events();\n                if (DOSBox_Paused() == false && ticksRemain > 0) {'
        out=patch.patch_dosbox_clock(src)
        self.assertIn('awe64_clock_decision.load(std::memory_order_acquire)!=nullptr',out)
        self.assertIn('if(hostTick!=0)',out)
        self.assertIn('if(hostTick==1 && !DOSBox_Paused())TIMER_AddTick();',out)
        self.assertIn('else {SDL_Delay(1);return 0;}',out)
        with self.assertRaises(ValueError):
            patch.patch_dosbox_clock('')
    def test_guest_entry(self):
        s='void GFX_Events() {}\n| SDL_WINDOW_SHOWN | (SDL2_resize_enable ? SDL_WINDOW_RESIZABLE : 0)\nint main(int argc, char* argv[]) SDL_MAIN_NOEXCEPT {}\n            DOSBOX_RunMachine();\n        } catch (int x) {'
        s+='\n'+"        SetConsoleCtrlHandler((PHANDLER_ROUTINE) ConsoleEventHandler,TRUE);"+'\n'+"        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0) {"+'\n'+"            LOG(LOG_GUI,LOG_WARN)(\"Failed to init joystick support\");\n            sdl.num_joysticks = 0;\n        }"
        s+='\n'+"        std::string videodriver = static_cast<Section_prop *>(control->GetSection(\"sdl\"))->Get_string(\"videodriver\");"+'\n'+"            sdl.init_ignore = true;\n        }\n    {\n        Section_prop *section = static_cast<Section_prop *>(control->GetSection(\"dosbox\"));"+'\n'+"        if (sdl_videodrv != NULL && !strcmp(sdl_videodrv,\"windows\")) sdl.using_windib = true;"
        s+="\n        /* -- SDL init */\n        if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) >= 0)\n            sdl.inited = true;\n        else\n            E_Exit(\"Can't init SDL %s\",SDL_GetError());\n    CommandLine com_line(argc,argv);\n    Config myconf(&com_line);\n#if defined(WIN32) && !defined(HX_DOS)\n    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);\n#endif"
        s+="\nstatic void SetPriority(PRIORITY_LEVELS level) {\nvoid Windows_DPI_Awareness_Init() {\nvoid sticky_keys(bool restore){\nvoid DOSBox_ShowConsole() {\n        if (!enableime&&!tonoime) ImmDisableIME((DWORD)(-1));"
        s+="\n    configfile = \"\";\n    exepath = GetDOSBoxXPath();\n    std::string config_path = Cross::GetPlatformConfigDir();\n    /* -- Parse configuration files */\n    {\n\n#if defined(WIN32) && !defined(C_SDL2) && !defined(HX_DOS) && !defined(_WIN32_WINDOWS)\n\t\tMSG_Add(\"PROGRAM_CONFIG_PROPERTY_ERROR\",\"No such section or property.\\n\");\n#if (ENVIRON_LINKED)\n        /* -- parse environment block (why?) */\n        control->ParseEnv(environ);\n#endif"
        r=patch.patch_main(s)
        self.assertIn('control->opt_defaultconf=true;',r)
        self.assertIn('control->opt_promptfolder=0;',r)
        self.assertIn('#endif // standalone config discovery, folder prompts and chdir',r)
        self.assertIn('#endif // standalone config load/save and migration',r)
        self.assertIn('#if (ENVIRON_LINKED) && !defined(AWE64_VST3_RUNTIME)',r)
        self.assertIn("\t\tMSG_Add(\"PROGRAM_CONFIG_PROPERTY_ERROR\",\"No such section or property.\\n\");",r)
        self.assertIn("static void SetPriority(PRIORITY_LEVELS level) {\n#ifdef AWE64_VST3_RUNTIME\n    (void)level;return; // process scheduling belongs to the DAW\n#endif",r)
        self.assertIn("void Windows_DPI_Awareness_Init() {\n#ifdef AWE64_VST3_RUNTIME\n    return; // the DAW owns process DPI awareness\n#endif",r)
        self.assertIn("void sticky_keys(bool restore){\n#ifdef AWE64_VST3_RUNTIME\n    (void)restore;return; // never change user accessibility settings\n#endif",r)
        self.assertIn("void DOSBox_ShowConsole() {\n#ifdef AWE64_VST3_RUNTIME\n    return; // never redirect DAW standard streams or allocate its console\n#endif",r)
        self.assertIn('#ifndef AWE64_VST3_RUNTIME\n        if (!enableime&&!tonoime) ImmDisableIME((DWORD)(-1));\n#endif',r)
        self.assertIn('if(awe64_sdl_init<0)',r)
        self.assertIn('if(awe64_sdl_init>=0 && awe64_audit_sdl_init_failure)',r)
        self.assertIn('AWE64AuditScope',r)
        self.assertIn('forwarded[count++]=argv[++i];',r)
        self.assertIn('"-awe64-audit-sdl-init"',r)
        self.assertIn('AWE64_EMBEDDED_SDL_CLEANUP_RETURN',r)
        self.assertIn('return -12566;',r)
        self.assertIn('return -12567;',r)
        self.assertIn('awe64_emu_init_failed=true;',r)
        self.assertIn('AWE64_EMBEDDED_EMU_CLEANUP_RETURN',r)
        self.assertIn('SDL_Quit();\n            sdl.inited=false;',r)
        self.assertIn('~AWE64ComScope(){if(SUCCEEDED(hr))CoUninitialize();}',r)
        self.assertIn('control=nullptr; // Config stack object has already been destroyed',r)
        self.assertIn('std::string videodriver="dummy";',r)
        self.assertIn('sdl.using_windib=false;',r)
        self.assertIn('#ifndef AWE64_VST3_RUNTIME\n'+"        if (sdl_videodrv != NULL && !strcmp(sdl_videodrv,\"windows\")) sdl.using_windib = true;"+'\n#endif',r)
        self.assertIn('#ifndef AWE64_VST3_RUNTIME\n'+"        SetConsoleCtrlHandler((PHANDLER_ROUTINE) ConsoleEventHandler,TRUE);"+'\n#endif',r)
        self.assertIn('#ifdef AWE64_VST3_RUNTIME\n        sdl.num_joysticks = 0;\n#else\n'+"        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0) {",r)
        self.assertIn("            LOG(LOG_GUI,LOG_WARN)(\"Failed to init joystick support\");\n            sdl.num_joysticks = 0;\n        }"+'\n#endif',r)
        self.assertIn('AWE64_StartGuest(',r)
        self.assertIn('const int result=main(count,forwarded);',r)
        self.assertIn('AWE64_ValidateExplicitRom(argc,argv,rom)',r)
        self.assertIn('return -12564;',r)
        self.assertIn('return -12565;',r)
        self.assertIn('AWE64_EMBEDDED_ARG_PREFLIGHT_REJECTED\\n',r)
        self.assertLess(r.index('if(!AWE64_ValidateEmbeddedArgs(argc,argv))'),r.index('AWE64RomEntryGuard rom;'))
        self.assertIn('return noConfig;',r)
        self.assertIn("argv[i][0]=='-'",r)
        self.assertLess(r.index('return -12564;'),r.index('const int result=main(count,forwarded);'))
        self.assertIn('FILE_SHARE_READ,nullptr,OPEN_EXISTING',r)
        self.assertIn('size.QuadPart==1048576',r)
        self.assertIn('if(rom)return false;',r)
        self.assertIn('AWE64_EMBEDDED_ROM_PREFLIGHT_REJECTED\\n',r)
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
        patched=patch.patch_sdl2_static(original+'\nif (SDL_VideoInit(NULL) < 0) {\nif (SDL_AudioInit(NULL) < 0) {')
        self.assertIn('if (SDL_AudioInit("dummy") < 0) {',patched)
        self.assertIn('SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "dummy", SDL_HINT_OVERRIDE);\n            if (SDL_VideoInit("dummy") < 0) {',patched)
        self.assertIn('#define SDL_STATIC_LIB 1',patched)
        self.assertIn(original,patched)
        with self.assertRaises(ValueError):
            patch.patch_sdl2_static(patched)
    def test_embedded_direct3d_uses_surface_without_video_restart(self):
        source='void d3d_init(void)\n{\n    sdl.desktop.want_type = SCREEN_DIRECT3D;'
        result=patch.patch_direct3d(source)
        self.assertIn('#ifdef AWE64_VST3_RUNTIME\n    OUTPUT_SURFACE_Select();\n    return;\n#endif',result)
        self.assertIn('sdl.desktop.want_type = SCREEN_DIRECT3D;',result)
        self.assertLess(result.index('OUTPUT_SURFACE_Select();'),result.index('SCREEN_DIRECT3D'))
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
