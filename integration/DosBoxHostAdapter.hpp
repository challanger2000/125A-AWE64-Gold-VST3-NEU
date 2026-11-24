#pragma once
// Same-process DOSBox-X C ABI -> original Creative DOS mailbox / real mixer PCM.
#include "CreativeDosMailbox.hpp"
#include "CreativeEffectMidi.hpp"
#include "AWE64_DOSBOX_ABI.h"
#include <cstdint>
#if defined(_WIN32)
#define AWE64_ABI_CALL __cdecl
#else
#define AWE64_ABI_CALL
#endif
namespace awe64::integration {
class DosBoxHostAdapter final {
    rt::RealtimeQueues& queues_;
    guest::CreativeDosMailbox mailbox_;
    AWE64_IsGuestMidiReadyFn ready_=nullptr;
public:
    explicit DosBoxHostAdapter(rt::RealtimeQueues& q,AWE64_IsGuestMidiReadyFn ready=nullptr) noexcept:
        queues_(q),mailbox_(q),ready_(ready){}
    DosBoxHostAdapter(const DosBoxHostAdapter&)=delete;
    DosBoxHostAdapter& operator=(const DosBoxHostAdapter&)=delete;
    guest::CreativeDosMailbox& mailbox() noexcept{return mailbox_;}
    static unsigned char AWE64_ABI_CALL readPort(void* ctx,unsigned short p) noexcept {
        return ctx?static_cast<DosBoxHostAdapter*>(ctx)->mailbox_.ioRead(p):0xffu;
    }
    static void AWE64_ABI_CALL writePort(void* ctx,unsigned short p,
                                         unsigned char v) noexcept {
        if(ctx)static_cast<DosBoxHostAdapter*>(ctx)->mailbox_.ioWrite(p,v);
    }
    // Preferred path: feed byte-complete MIDI into the ORIGINAL DOSBox-X
    // MPU-401 UART. Unmodified Creative DEMO.EXE reads 330h/331h and calls
    // the actual DOS Creative SF2LIB APIs, no recompiled 16-bit guest required.
    // ONLY the DOSBox emulation thread consumes this SPSC MIDI queue.
    static unsigned int AWE64_ABI_CALL pullMidi(void* ctx,
           unsigned char* out,unsigned int capacity) noexcept {
        if(!ctx||!out||capacity<3u)return 0u;
        rt::MidiEvent ev{};
        auto& queue=static_cast<DosBoxHostAdapter*>(ctx)->queues_;
        if(!queue.peekMidi(ev))return 0u;
        if(ev.status==0xf0u) {
            if(!rt::validCreativeEffectType(ev)){(void)queue.popMidi(ev);return 0u;}
            if(capacity<ev.size)return 0u; // preserve FIFO ordering and whole SysEx
            if(!queue.popMidi(ev))return 0u;
            for(unsigned i=0;i<ev.size;++i)out[i]=ev.sysex[i];
            return ev.size;
        }
        if(!queue.popMidi(ev))return 0u;
        const unsigned status=ev.status;
        const unsigned category=status&0xf0u;
        if(status<0x80u||status>=0xf0u||
           ev.size!=((category==0xc0u||category==0xd0u)?2u:3u)||
           ev.data1>127u||ev.data2>127u)return 0u;
        out[0]=ev.status;out[1]=ev.data1;
        if(ev.size==3u)out[2]=ev.data2;
        return ev.size;
    }
    static void AWE64_ABI_CALL receivePCM(void* ctx,const std::int16_t* s,
                                           unsigned int frames) noexcept {
        if(!ctx||!s)return;
        auto* self=static_cast<DosBoxHostAdapter*>(ctx);
        // Discard hardware startup PCM before it reaches the audio ring.
        if(self->ready_&&!self->ready_())return;
        auto& ring=self->queues_;
        constexpr float scale=1.f/32768.f;
        for(unsigned int i=0;i<frames;i++)
            (void)ring.pushAudio({float(s[2u*i])*scale,float(s[2u*i+1u])*scale});
    }
};
}
