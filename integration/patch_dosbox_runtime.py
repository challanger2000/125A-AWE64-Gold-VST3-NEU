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
// Disabled for unchanged original reference and synthetic tests until the
// real VST3 host publishes an absolute hardware send setting.
static std::atomic<unsigned> awe64_absolute_sends{0x10000u};
static std::atomic<uint64_t> awe64_voice_controls_lo{0};
static std::atomic<unsigned> awe64_voice_controls_hi{0};
extern "C" __declspec(dllexport) void __cdecl AWE64_SetVoiceControls(uint64_t lo,unsigned hi) noexcept {
    awe64_voice_controls_lo.store(lo,std::memory_order_release);
    awe64_voice_controls_hi.store(hi&65535u,std::memory_order_release);
}
extern "C" __declspec(dllexport) void __cdecl AWE64_SetAbsoluteSends(
    unsigned packed) noexcept {
    awe64_absolute_sends.store(packed<=0xffffu?packed:0x10000u,
                               std::memory_order_release);
}
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
// Opt-in guest-clock PCM for the genuine VST3; direct references stay on SDL.
static std::atomic<bool> awe64_guest_clock_pcm{false};
extern "C" __declspec(dllexport) void __cdecl AWE64_SetGuestClockPCM(unsigned enabled) noexcept {
    awe64_guest_clock_pcm.store(enabled!=0,std::memory_order_release);
}
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
    math=(Path(__file__).resolve().parents[1]/"research/p165-control-port/VoiceOverlay.hpp").read_text(encoding="utf-8").replace("#pragma once\n","",1)
    s=once(s,'#include <string>','#include <string>\n#ifdef AWE64_VST3_RUNTIME\n#include <atomic>\n'+math+'\nextern uint64_t AWE64_OverlayMidiContext() noexcept;\n#endif')
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
    # The original synth and effects stay untouched. Set only the two
    # documented send bytes through the actual EMU8000 register writer.
    s=once(s,'\t/* Emulated time in chip frames (44100 Hz) since the reset. */',r'''#ifdef AWE64_VST3_RUNTIME
    unsigned appliedSends=0x10000u;
    uint64_t overlayContext=0;
    awe64::overlay::Controls voiceControls{};
    struct FilterBase { uint16_t ifatn=0,pefe=0; bool haveIfatn=false,havePefe=false; };
    FilterBase filterBases[30]{};
    void VoiceWrite(Bitu port,Bitu word) {
        using namespace awe64::overlay;
        const auto context=AWE64_OverlayMidiContext();
        if(context!=overlayContext) {
            overlayContext=context;
            const auto lo=awe64_voice_controls_lo.load(std::memory_order_acquire);
            const auto hi=awe64_voice_controls_hi.load(std::memory_order_acquire);
            for(unsigned i=0;i<8;++i)voiceControls[i]=static_cast<uint8_t>(lo>>(8*i));
            voiceControls[8]=static_cast<uint8_t>(hi);
            voiceControls[9]=static_cast<uint8_t>(hi>>8);
            for(auto& f:filterBases)f=FilterBase{};
        }
        const unsigned voice=emu->cur_voice,reg=emu->cur_reg;
        const unsigned kind=static_cast<unsigned>(context&3u);
        // Only complete, actually consumed host MIDI packets, during the
        // synchronous original DEMO handler, and never DMA/reserved voices.
        const bool musical=kind && voice<30 && !(emu->voice[voice].ccca&0x04000000u);
        const bool any=std::any_of(voiceControls.begin(),voiceControls.end(),[](uint8_t x){return x!=0;});
        if(musical&&any) {
            const bool on=kind==1;
            const auto w=static_cast<uint16_t>(word);
            if(port==base+0x400u) {
                if(on&&reg==4)word=delay(w,voiceControls[0]);
                if(on&&reg==6)word=delay(w,voiceControls[4]);
                if(reg==5 && ((on&&!(w&0x8000u)&&!(w&0x80u))||(w&0x8000u)))
                    word=decaySustainRelease(w,on?voiceControls[1]:0,on?voiceControls[2]:0,voiceControls[3]);
                if(reg==7 && (on||(w&0x8000u)))
                    word=decaySustainRelease(w,on?voiceControls[5]:0,on?voiceControls[6]:0,voiceControls[7]);
            } else if(on&&port==base+0x402u) {
                if(reg==4)word=attack(w,voiceControls[0]);
                if(reg==6)word=attack(w,voiceControls[4]);
                if(reg==0&&!(w&0x0400u))word=resonance(w,voiceControls[9]);
            } else if(on&&port==base+0x800u&&(reg==1||reg==2)) {
                // Pair original IFATN/PEFE words in either driver order.
                // Apply only once both have been written in this NoteOn.
                auto& f=filterBases[voice];
                if(reg==1){f.ifatn=w;f.haveIfatn=true;}
                else {f.pefe=w;f.havePefe=true;}
                if(f.haveIfatn&&f.havePefe) {
                    const auto adjusted=filter(f.ifatn,f.pefe,voiceControls);
                    const unsigned otherReg=reg==1?2u:1u;
                    const uint16_t other=reg==1?adjusted.pefe:adjusted.ifatn;
                    const uint16_t before=reg==1?emu->voice[voice].pefe:emu->voice[voice].ifatn;
                    if(other!=before) {
                        AbsoluteSendWrite(base+0x802u,(otherReg<<5)|voice);
                        AbsoluteSendWrite(base+0x800u,other);
                        AbsoluteSendWrite(base+0x802u,(reg<<5)|voice);
                    }
                    word=reg==1?adjusted.ifatn:adjusted.pefe;
                } else if(reg==1) {
                    // Standalone cutoff still works if original driver
                    // does not subsequently write a modulation route.
                    Controls cutoffOnly{};cutoffOnly[8]=voiceControls[8];
                    word=filter(w,0,cutoffOnly).ifatn;
                }
            }
        }
        AbsoluteSendWrite(port,AbsoluteSendWord(port,word));
    }
    void AbsoluteSendWrite(Bitu port,Bitu value) {
        Trace(port,value);
        emu8k_outw((uint16_t)port,(uint16_t)value,emu);
    }
    void ApplyAbsoluteSends() {
        const unsigned requested=awe64_absolute_sends.load(std::memory_order_acquire);
        if(requested==appliedSends)return;
        appliedSends=requested;
        if(requested>0xffffu)return;
        const Bitu saved=(emu->cur_reg<<5)|emu->cur_voice;
        const unsigned rev=requested&255u,cho=requested>>8;
        bool changed=false;
        for(unsigned voice=0;voice<30;++voice) {
            const auto& v=emu->voice[voice];
            if(v.ptrx_revb_send!=rev) {
                AbsoluteSendWrite(base+0x802u,0x20u|voice);
                AbsoluteSendWrite(base,(v.ptrx&255u)|(rev<<8));
                changed=true;
            }
            if(v.csl_chor_send!=cho) {
                AbsoluteSendWrite(base+0x802u,0xe0u|voice);
                AbsoluteSendWrite(base+2,(v.csl>>16&255u)|(cho<<8));
                changed=true;
            }
        }
        if(changed)AbsoluteSendWrite(base+0x802u,saved);
    }
    Bitu AbsoluteSendWord(Bitu port,Bitu word) const {
        if(appliedSends>0xffffu||emu->cur_voice>=30)return word;
        if(port==base&&emu->cur_reg==1)
            return (word&255u)|((appliedSends&255u)<<8);
        if(port==base+2&&emu->cur_reg==7)
            return (word&255u)|(appliedSends&0xff00u);
        return word;
    }
#endif
'''+ '\t/* Emulated time in chip frames (44100 Hz) since the reset. */')
    a=s.index('void EMU8000_Device::Write(Bitu port, Bitu val, Bitu iolen) {')
    b=s.index('void EMU8000_Device::Mix(Bitu len) {',a)
    original=s[a:b]
    changed=once(original,'\tSync();',r'''\tSync();
#ifdef AWE64_VST3_RUNTIME
    ApplyAbsoluteSends();
    // Only rewrite the send byte of actual register writes, retaining the
    // exact other fields, including pitch/pan/loop address.
    if(appliedSends<=0xffffu) {
        if(iolen>=4) {
            const Bitu lo=AbsoluteSendWord(port,val&0xffffu);
            const Bitu hi=AbsoluteSendWord(port+2,(val>>16)&0xffffu);
            val=lo|(hi<<16);
        } else if(iolen==2) {
            val=AbsoluteSendWord(port,val&0xffffu);
        } else if((port&~(Bitu)1)==base || (port&~(Bitu)1)==base+2) {
            // Upstream byte writes are exactly zero-extended word writes.
            const Bitu word=(port&1)?(val&255u)<<8:val&255u;
            AbsoluteSendWrite(port&~(Bitu)1,AbsoluteSendWord(port&~(Bitu)1,word));
            return;
        }
    }
#endif'''.replace(r'\t','\t'))
    # The previous send-only rewrite is superseded by the same original
    # word writer with voice control handling. Split dwords in bus order.
    start=changed.index("#ifdef AWE64_VST3_RUNTIME")
    end=changed.index("#endif",start)+len("#endif")
    changed=changed[:start]+r'''#ifdef AWE64_VST3_RUNTIME
    ApplyAbsoluteSends();
    if(iolen>=4) {
        VoiceWrite(port,val&0xffffu);
        VoiceWrite(port+2,(val>>16)&0xffffu);
    } else if(iolen==2) {
        VoiceWrite(port,val&0xffffu);
    } else {
        VoiceWrite(port&~(Bitu)1,(port&1)?(val&255u)<<8:val&255u);
    }
    return;
#endif'''+changed[end:]
    s=s[:a]+changed+s[b:]
    s=once(s,'void EMU8000_Device::Mix(Bitu len) {\n\t/*',r'''void EMU8000_Device::Mix(Bitu len) {
#ifdef AWE64_VST3_RUNTIME
    Sync();
    ApplyAbsoluteSends();
#endif
\t/*'''.replace(r'\t','\t'))
    return s
def patch_mixer(s):
    if 'AWE64_HostRegisterPCM(' in s:raise ValueError('Already patched mixer')
    s=once(s,'#define MIXER_SSIZE 4',
        '#ifdef AWE64_VST3_RUNTIME\n#include <atomic>\n#endif\n'+PCM_BRIDGE+
        '\n#define MIXER_SSIZE 4')
    # SDL2 is statically linked into this private DOSBox DLL. Switch ONLY
    # this runtime's SDL audio backend to dummy so Studio Pro/ASIO never gets
    # a second hardware output. Dummy runs the same original callback clock.
    snd='    if (mixer.nosound) {'
    s=once(s,snd,'''#ifdef AWE64_VST3_RUNTIME
    if(!mixer.nosound && SDL_AudioInit("dummy")<0) {
        LOG(LOG_MISC,LOG_ERROR)("125A AWE64: internal SDL dummy audio unavailable: %s",SDL_GetError());
        mixer.nosound=true;  // fail closed, never open physical output
    }
#endif
'''+snd)
    # This export must be defined AFTER SDL2_AudioDevice, not before it:
    # MSVC previously rejected the inline function at the MIXER_SSIZE define
    # because the SDL2 device variable is declared several lines later.
    anchor='SDL_AudioDeviceID SDL2_AudioDevice = 0; /* valid IDs are 2 or higher, 1 for compat, 0 is never a valid ID */'
    s=once(s,anchor,'''SDL_AudioDeviceID SDL2_AudioDevice = 0; /* valid IDs are 2 or higher, 1 for compat, 0 is never a valid ID */
#ifdef AWE64_VST3_RUNTIME
extern "C" __declspec(dllexport) int __cdecl AWE64_IsInternalAudioOnly() noexcept {
    const char* name=SDL_GetCurrentAudioDriver();
    return name && strcmp(name,"dummy")==0 && SDL2_AudioDevice!=0 ? 1 : 0;
}
#endif''')
    anchor='    }\n}\n\nstd::string mixerinfo() {'
    s=once(s,anchor,'''    }
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
    # Reuse the original final mixer callback on the guest's existing 1-ms
    # timer. SDL dummy remains open but paused. No change to the chip DSP.
    s=once(s,'static void MIXER_Mix(void) {',r'''#ifdef AWE64_VST3_RUNTIME
static void SDLCALL MIXER_CallBack(void*,Uint8*,int);
#endif
static void MIXER_Mix(void) {''')
    s=once(s,'    MIXER_FillUp();\n}\n\nstatic void SDLCALL MIXER_CallBack',r'''    MIXER_FillUp();
#ifdef AWE64_VST3_RUNTIME
    if(awe64_guest_clock_pcm.load(std::memory_order_acquire) && SDL2_AudioDevice!=0) {
        int remains=(int)mixer.work_in-(int)mixer.work_out;
        if(remains<0)remains+=(int)mixer.work_wrap;
        int16_t pcm[128*2];
        while(remains>0) {
            const int n=remains>128?128:remains;
            MIXER_CallBack(nullptr,reinterpret_cast<Uint8*>(pcm),n*MIXER_SSIZE);
            remains-=n;
        }
    }
#endif
}

static void SDLCALL MIXER_CallBack''')
    s=once(s,'        SDL_PauseAudioDevice(SDL2_AudioDevice, 0);',r'''#ifdef AWE64_VST3_RUNTIME
        if(!awe64_guest_clock_pcm.load(std::memory_order_acquire))
#endif
        SDL_PauseAudioDevice(SDL2_AudioDevice, 0);''')
    s=once(s,'        mixer.prebuffer_samples = ((unsigned int)ms * (unsigned int)mixer.freq) / 1000u;',r'''        mixer.prebuffer_samples = ((unsigned int)ms * (unsigned int)mixer.freq) / 1000u;
#ifdef AWE64_VST3_RUNTIME
        if(awe64_guest_clock_pcm.load(std::memory_order_acquire)) {
            mixer.prebuffer_samples=0;
            mixer.prebuffer_wait=false;
        }
#endif''')
    return s
MPU_BRIDGE = r'''
#ifdef AWE64_VST3_RUNTIME
// Host -> guest through the ORIGINAL MPU-401 UART FIFO at 330h/331h.
// The original, UNMODIFIED Creative DEMO.EXE polls those ports and calls
// Creative's genuine awe32NoteOn/Off/Controller/ProgramChange functions.
using Awe64PullMidi = unsigned int (__cdecl *)(void*,unsigned char*,unsigned int);
static std::atomic<void*> awe64_mpu_ctx{nullptr};
static std::atomic<Awe64PullMidi> awe64_mpu_pull{nullptr};
static std::atomic<bool> awe64_guest_midi_ready{false};
// Guest CPU thread only. Tags belong to actual FIFO entries, not host
// enqueue time. The last consumed byte opens the synchronous DEMO handler.
static unsigned char awe64_overlay_tags[32]{};
static uint64_t awe64_overlay_epoch=0,awe64_overlay_context=0;
uint64_t AWE64_OverlayMidiContext() noexcept { return awe64_overlay_context; }
extern "C" __declspec(dllexport) int __cdecl AWE64_IsGuestMidiReady() noexcept {
    return awe64_guest_midi_ready.load(std::memory_order_acquire)?1:0;
}
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
    if(mpu.mode!=M_UART) {
        awe64_guest_midi_ready.store(false,std::memory_order_release);
        return;
    }
    // The untouched Creative DEMO consumes its UART ACK before SF2 load.
    // Only its subsequent status poll signals entry into the MIDI loop.
    // Do not let host MIDI contaminate the initialization ACK.
    if(!awe64_guest_midi_ready.load(std::memory_order_relaxed)) {
        if(mpu.queue_used!=0)return;
        awe64_guest_midi_ready.store(true,std::memory_order_release);
    }
    auto pull=awe64_mpu_pull.load(std::memory_order_acquire);
    if(!pull)return;
    // The original UART FIFO holds 32 bytes. Queue complete messages only.
    for(unsigned n=0;n<8 && mpu.queue_used<=MPU401_QUEUE-3;++n) {
        unsigned char buf[11]{};
        const unsigned free_bytes=MPU401_QUEUE-mpu.queue_used;
        const unsigned capacity=free_bytes<11u?free_bytes:11u;
        unsigned len=pull(awe64_mpu_ctx.load(std::memory_order_acquire),buf,capacity);
        if(len==0)break;
        const unsigned type=buf[0]&0xf0u;
        if(len>capacity)continue;
        if(buf[0]==0xf0u) {
            // Complete documented 11-byte Creative effect SysEx only.
            if(len!=11u||buf[10]!=0xf7u)continue;
            bool valid=true;
            for(unsigned i=1;i<10u;++i)if(buf[i]>=0x80u)valid=false;
            if(!valid)continue;
        } else if(buf[0]<0x80u || buf[0]>=0xf0u ||
           len!=((type==0xc0u || type==0xd0u)?2u:3u))continue;
        unsigned kind=0;
        if(type==0x90u&&len==3)kind=buf[2]?1u:2u;
        if(type==0x80u&&len==3)kind=2u;
        if(type==0xb0u&&len==3&&(buf[1]==64||buf[1]==120||buf[1]==121||buf[1]==123))kind=2u;
        for(unsigned i=0;i<len;++i) {
            const Bitu before=mpu.queue_used;
            QueueByte(buf[i]);
            if(i+1==len && mpu.queue_used==before+1)
                awe64_overlay_tags[(mpu.queue_pos+mpu.queue_used-1)%MPU401_QUEUE]=static_cast<unsigned char>(kind);
        }
    }
}
#endif

'''
    s=once(s,anchor,pump+anchor)
    s=once(s,'mpu.queue[pos]=data;','mpu.queue[pos]=data;\n#ifdef AWE64_VST3_RUNTIME\n        awe64_overlay_tags[pos]=0;\n#endif')
    s=once(s,'ret=mpu.queue[mpu.queue_pos];','ret=mpu.queue[mpu.queue_pos];\n#ifdef AWE64_VST3_RUNTIME\n        const auto tag=awe64_overlay_tags[mpu.queue_pos];\n        awe64_overlay_tags[mpu.queue_pos]=0;\n        awe64_overlay_context=tag && mpu.mode==M_UART ? (++awe64_overlay_epoch<<2)|tag : 0;\n#endif')
    anchor='static Bitu MPU401_ReadStatus(Bitu port,Bitu iolen) {'
    s=once(s,anchor,anchor+'''
#ifdef AWE64_VST3_RUNTIME
    awe64_overlay_context=0; // previous synchronous DEMO handler returned
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
    event='void GFX_Events() {'
    injected='''
#ifdef AWE64_VST3_RUNTIME
#include <atomic>
static std::atomic<bool> awe64_guest_stop{false};
// GFX_Events is also called during initialization and teardown. Throwing
// there skips the original main() cleanup. Only the original run-machine
// try/catch is an established safe cancellation boundary.
static thread_local bool awe64_guest_can_stop=false;
struct AWE64GuestRunScope {
    AWE64GuestRunScope() noexcept { awe64_guest_can_stop=true; }
    ~AWE64GuestRunScope() { awe64_guest_can_stop=false; }
};
// Never exit() the DAW process. The emulator thread observes this flag,
// throws DOSBox-X's existing shutdown signal and releases its own state.
extern "C" __declspec(dllexport) void __cdecl AWE64_RequestStop() noexcept {
    awe64_guest_stop.store(true,std::memory_order_release);
}
#endif
'''
    # The only visible UI is the approved P165 VST3 editor. Do not display
    # an additional DOSBox-X emulation window inside the DAW.
    # This is ONLY applied to the private runtime DLL working copy.
    visible_flag='| SDL_WINDOW_SHOWN | (SDL2_resize_enable ? SDL_WINDOW_RESIZABLE : 0)'
    s=once(s,visible_flag,
           '| SDL_WINDOW_HIDDEN | (SDL2_resize_enable ? SDL_WINDOW_RESIZABLE : 0)')
    s=once(s,event,injected+'\n'+event+'''
#ifdef AWE64_VST3_RUNTIME
    if(awe64_guest_can_stop && awe64_guest_stop.load(std::memory_order_acquire))throw(0);
#endif''')
    # Standalone DOSBox owns these process-level facilities. An embedded
    # instrument must not register a process console shutdown handler
    # (which raises SIGTERM), or detect physical game controllers through
    # SDL's shared Win32 notification-window class "Message".
    # Keep the original standalone paths; don't change guest hardware/DSP.
    s=once(s,'        SetConsoleCtrlHandler((PHANDLER_ROUTINE) ConsoleEventHandler,TRUE);',r'''#ifndef AWE64_VST3_RUNTIME
        SetConsoleCtrlHandler((PHANDLER_ROUTINE) ConsoleEventHandler,TRUE);
#endif''')
    s=once(s,'        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0) {',r'''#ifdef AWE64_VST3_RUNTIME
        sdl.num_joysticks = 0;
#else
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) >= 0) {''')
    s=once(s,"            LOG(LOG_GUI,LOG_WARN)(\"Failed to init joystick support\");\n            sdl.num_joysticks = 0;\n        }",r'''            LOG(LOG_GUI,LOG_WARN)("Failed to init joystick support");
            sdl.num_joysticks = 0;
        }
#endif''')
    # The embedded SDL copy now selects dummy audio/video directly.
    # Do not run the standalone environment-dependent driver-selection
    # branch, which otherwise sets using_windib despite dummy video.
    s=once(s,"        std::string videodriver = static_cast<Section_prop *>(control->GetSection(\"sdl\"))->Get_string(\"videodriver\");",r'''#ifdef AWE64_VST3_RUNTIME
        std::string videodriver="dummy";
        sdl.using_windib=false;
        sdl.init_ignore=true;
#else
        std::string videodriver = static_cast<Section_prop *>(control->GetSection("sdl"))->Get_string("videodriver");''')
    s=once(s,"            sdl.init_ignore = true;\n        }\n    {\n        Section_prop *section = static_cast<Section_prop *>(control->GetSection(\"dosbox\"));",r'''            sdl.init_ignore = true;
        }
#endif
    {
        Section_prop *section = static_cast<Section_prop *>(control->GetSection("dosbox"));''')
    s=once(s,"        if (sdl_videodrv != NULL && !strcmp(sdl_videodrv,\"windows\")) sdl.using_windib = true;",r'''#ifndef AWE64_VST3_RUNTIME
        if (sdl_videodrv != NULL && !strcmp(sdl_videodrv,"windows")) sdl.using_windib = true;
#endif''')
    # Defer even a stop requested BEFORE main() until original init is
    # complete and the existing kill-switch handler can perform cleanup.
    run='            DOSBOX_RunMachine();\n        } catch (int x) {'
    s=once(s,run,'''
#ifdef AWE64_VST3_RUNTIME
            AWE64GuestRunScope awe64_run_scope;
            if(awe64_guest_stop.load(std::memory_order_acquire))throw(0);
#endif
'''+run)
    return s+'''
#ifdef AWE64_VST3_RUNTIME
#include <cstring>
#include <cstdio>
// Embedded callers have a deliberately small, explicit command-line contract.
// Fail before DOSBox initializes devices if an AWE ROM is missing or malformed.
// Do not enter the standalone fallback/ROM-download or temporary-empty-ROM path.
struct AWE64RomEntryGuard {
    HANDLE file=INVALID_HANDLE_VALUE;
    ~AWE64RomEntryGuard(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);}
    bool open(const char* path) {
        if(!path||!*path)return false;
        file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)return false;
        LARGE_INTEGER size{};
        return GetFileType(file)==FILE_TYPE_DISK&&GetFileSizeEx(file,&size)&&size.QuadPart==1048576;
    }
};
static bool AWE64_ValidateExplicitRom(int argc,char** argv,AWE64RomEntryGuard& guard) {
    if(argc<1||!argv||!argv[0])return false;
    const char* rom=nullptr;
    bool awe=false;
    for(int i=1;i<argc;++i){
        if(!argv[i])return false;
        if(std::strcmp(argv[i],"-set")!=0)continue;
        if(++i>=argc||!argv[i])return false;
        constexpr char prefix[]="sblaster awe32rom=";
        if(std::strncmp(argv[i],prefix,sizeof(prefix)-1)==0){
            if(rom)return false; // ambiguous duplicate path
            rom=argv[i]+sizeof(prefix)-1;
        }else if(std::strcmp(argv[i],"sblaster sbtype=sbawe")==0)awe=true;
    }
    if(rom)return guard.open(rom);
    return !awe;
}
// Must be invoked on a dedicated host-owned worker, inside the VST3 process.
extern "C" __declspec(dllexport) int __cdecl AWE64_StartGuest(
    int argc,char** argv) {
    AWE64RomEntryGuard rom;
    if(!AWE64_ValidateExplicitRom(argc,argv,rom)){
        std::fputs("AWE64_EMBEDDED_ROM_PREFLIGHT_REJECTED\\n",stderr);
        std::fflush(stderr);
        return -12564;
    }
    // Never reset a stop request here: the VST3 host can deactivate before
    // the worker has entered this call. A late reset would cause join() hang.
    // The DLL is loaded afresh for each activation, so the flag starts false.
    return main(argc,argv);
}
#endif
'''
def patch_sdl2_static(s):
    # The pinned fork statically links SDL2 from vs/sdl2/VisualC/SDL.
    # However vs/sdl2/src/SDL.c still compiles its own Windows
    # _DllMainCRTStartup when SDL_STATIC_LIB is undefined, which clashes
    # with the embedded DLL's proper CRT startup. Disable only SDL's
    # duplicate entrypoint in this CI working copy; SDL callback/DSP stay.
    if '#define SDL_STATIC_LIB 1' in s:
        raise ValueError('SDL2 static CRT entry already patched')
    anchor='#if (!defined(HAVE_LIBC) || defined(__WATCOMC__)) && !defined(SDL_STATIC_LIB)'
    if s.count(anchor)!=1:
        raise ValueError('SDL2 startup anchor missing/duplicated')
    s=once(s,anchor,
         '#define SDL_STATIC_LIB 1 /* 125A embedded DLL: SDL2 is a static library */\n'
         +anchor)
    # The already-tested dummy video backend is mandatory for this private
    # embedded SDL copy. Avoid shared Win32 window classes (SDL_app) and
    # dependence on the host's SDL_VIDEODRIVER environment variable.
    # SDL_nullvideo.c DUMMY_Available() requires this hint even when
    # SDL_VideoInit receives an explicit driver name. Set it in this
    # static SDL copy, never in the host process environment.
    s=once(s,'if (SDL_VideoInit(NULL) < 0) {',
                  'SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "dummy", SDL_HINT_OVERRIDE);\n'
                  '            if (SDL_VideoInit("dummy") < 0) {')
    # Match the successful dual-DLL test from the first SDL init, not
    # merely when the emulated mixer later reinitializes its audio.
    return once(s,'if (SDL_AudioInit(NULL) < 0) {',
                  'if (SDL_AudioInit("dummy") < 0) {')

def patch_direct3d(s):
    # The dummy SDL window has no native HWND. The standalone Direct3D
    # init checks getenv rather than the active SDL driver, restarts video
    # and invalidates the existing dummy window. Use the exact software
    # surface fallback immediately in the embedded runtime.
    anchor='void d3d_init(void)\n{'
    return once(s,anchor,anchor+'''
#ifdef AWE64_VST3_RUNTIME
    OUTPUT_SURFACE_Select();
    return;
#endif
''')

def patch_project(s):
    cond="'$(Configuration)|$(Platform)'=='Release SDL2|x64'"
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
    # Classic Release|x64 builds SDL1: no hidden window and no SDL2
    # dummy audio. The internal VST3 runtime MUST use Release SDL2|x64.
    if 'C_SDL2' not in group:raise ValueError('SDL2 build configuration missing')
    # Original Release SDL2|x64 project was authored for an executable.
    # When converted to an embedded DLL, VS linker omits the usual CRT
    # startup-library linkage and yields hundreds of unresolved UCRT/VCRUNTIME
    # imports. Explicitly name the STATIC CRT family in this DLL-only target.
    link_pattern=r'(<AdditionalDependencies>)([^<]+)(</AdditionalDependencies>)'
    def complete_static_crt(m):
        return (m.group(1)+'libvcruntime.lib;libucrt.lib;libcmt.lib;'
                +m.group(2)+m.group(3))
    group,n_crt=re.subn(link_pattern,complete_static_crt,group,count=1)
    if n_crt!=1:raise ValueError('Release SDL2 x64 linker dependencies missing')

    return left+marker+group+'  </ItemDefinitionGroup>'+tail


# Audit A01: opt-in host-demand pacing. Legacy reference and synthetic
# fixtures keep their original wall-clock behaviour. Never change SDL or
# EMU8000 synthesis for this scheduling adjustment.
_original_patch_mixer = patch_mixer
_original_patch_mpu = patch_mpu

def patch_mixer(s):
    s = _original_patch_mixer(s)
    anchor = 'static std::atomic<bool> awe64_guest_clock_pcm{false};'
    s = once(s, anchor, anchor + r"""
static std::atomic<bool> awe64_guest_clock_fault{false};
extern bool AWE64_HostDemandEnabled() noexcept;
bool AWE64_GuestClockEnabled() noexcept {
    return awe64_guest_clock_pcm.load(std::memory_order_acquire) && AWE64_HostDemandEnabled();
}
extern "C" __declspec(dllexport) int __cdecl AWE64_GuestClockFault() noexcept {
    return awe64_guest_clock_fault.load(std::memory_order_acquire)?1:0;
}
""")
    # In the synchronous guest mixer, only count real frames. The standalone
    # callback's backlog drop and last-DAC padding are not a clock source.
    anchor = '    if ((unsigned long)remains >= (mixer.blocksize*2UL)) {'
    s = once(s, anchor, r"""#ifdef AWE64_VST3_RUNTIME
    if(!AWE64_GuestClockEnabled())
#endif
""" + anchor)
    anchor = '    if (need > 0)\n        mixer.prebuffer_wait = true;'
    s = once(s, anchor, r"""#ifdef AWE64_VST3_RUNTIME
    const auto actualFrames=static_cast<unsigned int>(len/MIXER_SSIZE-need);
    if(AWE64_GuestClockEnabled() && need!=0)
        awe64_guest_clock_fault.store(true,std::memory_order_release);
#endif
"""+anchor)
    anchor = '        if(fn)fn(awe64_pcm_context.load(std::memory_order_acquire),\n                 reinterpret_cast<const int16_t*>(stream),\n                 static_cast<unsigned int>(len/MIXER_SSIZE));'
    s = once(s, anchor, r"""        const auto frames=AWE64_GuestClockEnabled()?actualFrames:static_cast<unsigned int>(len/MIXER_SSIZE);
        if(fn&&frames)fn(awe64_pcm_context.load(std::memory_order_acquire),
                 reinterpret_cast<const int16_t*>(stream),frames);""")
    anchor = '        int remains=(int)mixer.work_in-(int)mixer.work_out;\n        if(remains<0)remains+=(int)mixer.work_wrap;\n        int16_t pcm[128*2];\n        while(remains>0) {\n            const int n=remains>128?128:remains;\n            MIXER_CallBack(nullptr,reinterpret_cast<Uint8*>(pcm),n*MIXER_SSIZE);\n            remains-=n;\n        }'
    s = once(s, anchor, r"""        const auto available=[]() noexcept {
            int count=(int)mixer.work_in-(int)mixer.work_out;
            if(count<0)count+=(int)mixer.work_wrap;
            return count;
        };
        int remains=available();
        int16_t pcm[128*2];
        if(!AWE64_GuestClockEnabled()) {
            while(remains>0) {
                const int n=remains>128?128:remains;
                MIXER_CallBack(nullptr,reinterpret_cast<Uint8*>(pcm),n*MIXER_SSIZE);
                remains-=n;
            }
        } else {
        // Verify actual work-pointer progress, not requested sample count.
        // If the invariant fails, do not generate more padding or loop.
        while(remains>0 && !awe64_guest_clock_fault.load(std::memory_order_acquire)) {
            const int n=remains>128?128:remains;
            MIXER_CallBack(nullptr,reinterpret_cast<Uint8*>(pcm),n*MIXER_SSIZE);
            const int after=available();
            if(after!=remains-n) {
                awe64_guest_clock_fault.store(true,std::memory_order_release);
                break;
            }
            remains=after;
        }
        }""")
    return s

def patch_mpu(s):
    s = _original_patch_mpu(s)
    anchor = 'static unsigned char awe64_overlay_tags[32]{};'
    s = once(s, anchor, 'extern bool AWE64_HostDemandEnabled() noexcept;\n'+anchor)
    anchor = '    for(unsigned n=0;n<8 && mpu.queue_used<=MPU401_QUEUE-3;++n) {'
    s = once(s, anchor, r"""    // The timed command stream is processed between complete original MIDI
    // handlers. Never publish the next overlay while the previous packet is
    // still pending in the guest UART. Legacy mode retains eight packets.
    const bool clocked=AWE64_HostDemandEnabled();
    if(clocked&&mpu.queue_used!=0)return;
    for(unsigned n=0;n<(clocked?1u:8u) && mpu.queue_used<=MPU401_QUEUE-3;++n) {""")
    return s

def patch_dosbox_clock(s):
    anchor = 'static Bitu Normal_Loop(void) {'
    s = once(s, anchor, r"""#ifdef AWE64_VST3_RUNTIME
#include <atomic>
using Awe64TickDecision = int (__cdecl *)(void*);
static std::atomic<void*> awe64_clock_ctx{nullptr};
static std::atomic<Awe64TickDecision> awe64_clock_decision{nullptr};
extern "C" __declspec(dllexport) void __cdecl AWE64_HostRegisterClock(void* ctx,Awe64TickDecision fn) noexcept {
    awe64_clock_decision.store(nullptr,std::memory_order_release);
    awe64_clock_ctx.store(ctx,std::memory_order_release);
    awe64_clock_decision.store(fn,std::memory_order_release);
}
bool AWE64_HostDemandEnabled() noexcept {
    return awe64_clock_decision.load(std::memory_order_acquire)!=nullptr;
}
static int AWE64_HostTickDecision() noexcept {
    auto fn=awe64_clock_decision.load(std::memory_order_acquire);
    return fn?fn(awe64_clock_ctx.load(std::memory_order_acquire)):0;
}
#endif
""" + anchor)
    anchor = '                GFX_Events();\n                if (DOSBox_Paused() == false && ticksRemain > 0) {'
    s = once(s, anchor, r"""                GFX_Events();
#ifdef AWE64_VST3_RUNTIME
                // At the original safe tick boundary, after the original
                // cooperative stop check. Boot/direct reference returns 0.
                const int hostTick=AWE64_HostTickDecision();
                if(hostTick!=0) {
                    if(hostTick==1 && !DOSBox_Paused())TIMER_AddTick();
                    else {SDL_Delay(1);return 0;}
                    continue;
                }
#endif
                if (DOSBox_Paused() == false && ticksRemain > 0) {""")
    return s

PATCHES={
    'src/dosbox.cpp':patch_dosbox_clock,
    'src/hardware/emu8000.cpp':patch_emu,
    'src/hardware/mixer.cpp':patch_mixer,
    'src/hardware/mpu401.cpp':patch_mpu,
    'src/gui/sdlmain.cpp':patch_main,
    'src/output/output_direct3d.cpp':patch_direct3d,
    'vs/dosbox-x.vcxproj':patch_project,
    'vs/sdl2/src/SDL.c':patch_sdl2_static
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
