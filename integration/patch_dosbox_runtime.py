#!/usr/bin/env python3
"""In-process ABI patch for pinned DOSBox-X AWE32. No DSP/driver replacement."""
import argparse
import re
from pathlib import Path

def once(s,needle,new):
    if s.count(needle)!=1: raise ValueError("Missing or duplicate source anchor: "+needle[:85])
    return s.replace(needle,new,1)

PORT_BRIDGE = r'''
#ifdef AWE64_VST3_RUNTIME
// Emulated CPU thread reads Creative MIDI input port BEE0h..BEEFh.
using Awe64IORead = unsigned char (__cdecl *)(void*,unsigned short);
using Awe64IOWrite = void (__cdecl *)(void*,unsigned short,unsigned char);
static std::atomic<void*> awe64_port_context{nullptr};
static std::atomic<Awe64IORead> awe64_port_reader{nullptr};
static std::atomic<Awe64IOWrite> awe64_port_writer{nullptr};
extern "C" __declspec(dllexport) void __cdecl AWE64_HostRegisterIO(
    void* p,Awe64IORead reader,Awe64IOWrite writer) noexcept {
    awe64_port_reader.store(nullptr,std::memory_order_release);
    awe64_port_writer.store(nullptr,std::memory_order_release);
    awe64_port_context.store(p,std::memory_order_release);
    awe64_port_reader.store(reader,std::memory_order_release);
    awe64_port_writer.store(writer,std::memory_order_release);
}
static Bitu awe64_read_guest_port(Bitu p,Bitu len) {
    if(len!=1)return 0xff;
    auto fn=awe64_port_reader.load(std::memory_order_acquire);
    return fn?fn(awe64_port_context.load(std::memory_order_acquire),
                 static_cast<unsigned short>(p)):0xff;
}
static void awe64_write_guest_port(Bitu p,Bitu v,Bitu len) {
    if(len!=1)return;
    auto fn=awe64_port_writer.load(std::memory_order_acquire);
    if(fn)fn(awe64_port_context.load(std::memory_order_acquire),
             static_cast<unsigned short>(p),static_cast<unsigned char>(v));
}
#endif
'''
PCM_BRIDGE = r'''
#ifdef AWE64_VST3_RUNTIME
using Awe64ReceivePCM = void (__cdecl *)(void*,const int16_t*,unsigned int);
static std::atomic<void*> awe64_pcm_context{nullptr};
static std::atomic<Awe64ReceivePCM> awe64_pcm_receiver{nullptr};
extern "C" __declspec(dllexport) void __cdecl AWE64_HostRegisterPCM(
    void* ctx,Awe64ReceivePCM callback) noexcept {
    awe64_pcm_receiver.store(nullptr,std::memory_order_release);
    awe64_pcm_context.store(ctx,std::memory_order_release);
    awe64_pcm_receiver.store(callback,std::memory_order_release);
}
#endif
'''
def patch_emu(s):
    if 'AWE64_HostRegisterIO(' in s:raise ValueError('Already patched EMU8000')
    s=once(s,'#include <string>','#include <string>\n#ifdef AWE64_VST3_RUNTIME\n#include <atomic>\n#endif')
    s=once(s,'class EMU8000_Device {',PORT_BRIDGE+'\nclass EMU8000_Device {')
    anchor='IO_WriteHandleObject WriteHandler[EMU_PORT_GROUPS];'
    s=once(s,anchor,anchor+'\n#ifdef AWE64_VST3_RUNTIME\n'
           '    IO_ReadHandleObject AweHostRead;\n    IO_WriteHandleObject AweHostWrite;\n#endif')
    anchor='\tchan = MixerChan.Install(emu8000_callback, EMU_RATE, EMU8000_MIXER_CHANNEL);'
    s=once(s,anchor,'''#ifdef AWE64_VST3_RUNTIME
    AweHostRead.Install(0xBEE0u,awe64_read_guest_port,IO_MB,16);
    AweHostWrite.Install(0xBEF0u,awe64_write_guest_port,IO_MB,1);
#endif
'''+anchor)
    return s
def patch_mixer(s):
    if 'AWE64_HostRegisterPCM(' in s:raise ValueError('Already patched mixer')
    s=once(s,'#define MIXER_SSIZE 4',
        '#ifdef AWE64_VST3_RUNTIME\n#include <atomic>\n#endif\n'+PCM_BRIDGE+
        '\n#define MIXER_SSIZE 4')
    anchor='    }\n}\n\nstd::string mixerinfo() {'
    return once(s,anchor,'''    }
#ifdef AWE64_VST3_RUNTIME
    // Original final stereo mixer PCM, not a generated test signal.
    if(len>0 && len%MIXER_SSIZE==0) {
        auto fn=awe64_pcm_receiver.load(std::memory_order_acquire);
        if(fn)fn(awe64_pcm_context.load(std::memory_order_acquire),
                 reinterpret_cast<const int16_t*>(stream),
                 static_cast<unsigned int>(len/MIXER_SSIZE));
    }
#endif
}

std::string mixerinfo() {''')
MPU_BRIDGE = r'''
#ifdef AWE64_VST3_RUNTIME
// Host -> guest through the ORIGINAL MPU-401 UART FIFO at 330h/331h.
// The original, UNMODIFIED Creative DEMO.EXE polls those ports and calls
// Creative's genuine awe32NoteOn/Off/Controller/ProgramChange functions.
using Awe64PullMidi = unsigned int (__cdecl *)(void*,unsigned char*,unsigned int);
static std::atomic<void*> awe64_mpu_ctx{nullptr};
static std::atomic<Awe64PullMidi> awe64_mpu_pull{nullptr};
extern "C" __declspec(dllexport) void __cdecl AWE64_HostRegisterMidi(
    void* p,Awe64PullMidi fn) noexcept {
    awe64_mpu_pull.store(nullptr,std::memory_order_release);
    awe64_mpu_ctx.store(p,std::memory_order_release);
    awe64_mpu_pull.store(fn,std::memory_order_release);
}
#endif
'''
def patch_mpu(s):
    if 'AWE64_HostRegisterMidi(' in s:raise ValueError('MPU already patched')
    s=once(s,'#include <assert.h>',
           '#include <assert.h>\n#ifdef AWE64_VST3_RUNTIME\n#include <atomic>\n#endif')
    s=once(s,'extern bool enable_slave_pic;',
           MPU_BRIDGE+'\nextern bool enable_slave_pic;')
    anchor='static void ClrQueue(void) {'
    pump='''#ifdef AWE64_VST3_RUNTIME
static void AWE64_PumpGuestMidi() {
    if(mpu.mode!=M_UART)return;
    auto pull=awe64_mpu_pull.load(std::memory_order_acquire);
    if(!pull)return;
    // The original UART FIFO holds 32 bytes. Queue complete messages only.
    for(unsigned n=0;n<8 && mpu.queue_used<=MPU401_QUEUE-3;++n) {
        unsigned char buf[3]{};
        unsigned len=pull(awe64_mpu_ctx.load(std::memory_order_acquire),buf,3);
        if(len==0)break;
        const unsigned type=buf[0]&0xf0u;
        if(buf[0]<0x80u || buf[0]>=0xf0u ||
           len!=((type==0xc0u || type==0xd0u)?2u:3u))continue;
        for(unsigned i=0;i<len;++i)QueueByte(buf[i]);
    }
}
#endif

'''
    s=once(s,anchor,pump+anchor)
    anchor='static Bitu MPU401_ReadStatus(Bitu port,Bitu iolen) {'
    s=once(s,anchor,anchor+'''
#ifdef AWE64_VST3_RUNTIME
    AWE64_PumpGuestMidi();
#endif''')
    anchor='if (!MIDI_Available()) return;'
    s=once(s,anchor,'''#ifndef AWE64_VST3_RUNTIME
        if (!MIDI_Available()) return;
#else
        // The embedded DOSBox guest has a virtual UART even when the host
        // system has no physical MIDI device.
#endif''')
    return s

def patch_main(s):
    a='int main(int argc, char* argv[]) SDL_MAIN_NOEXCEPT {'
    if s.count(a)!=1 or 'AWE64_StartGuest(' in s:
        raise ValueError('Unknown or already patched DOSBox main')
    return s+'''
#ifdef AWE64_VST3_RUNTIME
// Must be invoked on a dedicated host-owned worker, inside the VST3 process.
extern "C" __declspec(dllexport) int __cdecl AWE64_StartGuest(
    int argc,char** argv) {
    return main(argc,argv);
}
#endif
'''
def patch_project(s):
    cond="'$(Configuration)|$(Platform)'=='Release|x64'"
    pre='  <PropertyGroup Condition="'+cond+'" Label="Configuration">\n'
    s=once(s,pre+'    <ConfigurationType>Application</ConfigurationType>',
           pre+'    <ConfigurationType>DynamicLibrary</ConfigurationType>')
    pre='  <PropertyGroup Condition="'+cond+'">\n'
    s=once(s,pre+'    <OutDir>',
           pre+'    <TargetName>AWE64DOSBoxRuntime</TargetName>\n    <OutDir>')
    marker='  <ItemDefinitionGroup Condition="'+cond+'">'
    if s.count(marker)!=1: raise ValueError('Release x64 build block missing')
    left,right=s.split(marker,1)
    group,tail=right.split('  </ItemDefinitionGroup>',1)
    group,count=re.subn(r'<PreprocessorDefinitions>([^<]+)</PreprocessorDefinitions>',
                       lambda m:m.group(0).replace('<PreprocessorDefinitions>',
                        '<PreprocessorDefinitions>AWE64_VST3_RUNTIME;'),group,count=1)
    if count!=1: raise ValueError('Compiler macro block missing')
    return left+marker+group+'  </ItemDefinitionGroup>'+tail

PATCHES={
    'src/hardware/emu8000.cpp':patch_emu,
    'src/hardware/mixer.cpp':patch_mixer,
    'src/hardware/mpu401.cpp':patch_mpu,
    'src/gui/sdlmain.cpp':patch_main,
    'vs/dosbox-x.vcxproj':patch_project
}
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('source',type=Path)
    root=parser.parse_args().source.resolve()
    writes={}
    for filename,fn in PATCHES.items():
        path=root/filename
        raw=path.read_bytes()
        bom=b'\xef\xbb\xbf' if raw.startswith(b'\xef\xbb\xbf') else b''
        end='\r\n' if b'\r\n' in raw else '\n'
        original=raw[len(bom):].decode('utf-8').replace('\r\n','\n')
        changed=fn(original)
        if changed==original:raise ValueError('No change: '+filename)
        writes[path]=bom+changed.replace('\n',end).encode('utf-8')
    # Validate ALL anchors before changing ANY checkout file.
    for path,data in writes.items():
        path.write_bytes(data)
        print('Patched '+str(path.relative_to(root)))
    print('DOSBox-X runtime source patch applied; no guest audio proof')
if __name__=='__main__':main()
