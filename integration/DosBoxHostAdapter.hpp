#pragma once
#include <utility>
// Same-process DOSBox-X C ABI -> original Creative DOS mailbox / real mixer PCM.
#include "CreativeDosMailbox.hpp"
#include "ResidentBankDrain.hpp"
#include "CreativeEffectMidi.hpp"
#include "GuestDemandClock.hpp"
#include "AWE64_DOSBOX_ABI.h"
#include <cstdint>
#include <array>
#include <memory>
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
    // Opt-in only for the original guest-clock PCM mode; never infer a
    // guest clock from asynchronous SDL dummy callback timing.
    const bool clocked_;
    const AWE64_SetVoiceControlsFn voiceControls_;
    const AWE64_SetAbsoluteSendsFn sends_;
    const std::shared_ptr<rt::LegacyCaptureGate> captureGate_;
    guest::ResidentBankMailbox* const resident_;
    guest::ResidentBankDrain residentDrain_; // DOS CPU thread only
    std::atomic<std::uint64_t> suspendedFrames_{0};
    // Independent of the host pause gate: a replacement guest may boot
    // while the old guest is playing. It must not accumulate pre-handover PCM.
    std::atomic<bool> captureStarted_{true};
    // Queue early MIDI immediately but do not hand it to the legacy UART
    // before the host has a primed PCM transport. Independent of capture.
    std::atomic<bool> legacyMidiStarted_{true};
    std::atomic<std::uint64_t> pulledNoteOns_{0};
    std::atomic<std::uint64_t> replacementSuppressedFrames_{0};
    rt::GuestDemandClock demand_;
    std::atomic<std::uint64_t> guestFrames_{0};
    std::atomic<std::uint64_t> lateMidi_{0};
    std::atomic<std::uint64_t> maxLateFrames_{0};
    // Consumer-owned shadow of notes actually emitted to the UART. A
    // lost NoteOff must not leave an already dispatched voice hanging.
    std::array<std::array<std::uint8_t,128>,16> notes_{};
    unsigned recoveryChannel_=0, recoveryPitch_=0, recoveryStage_=0;
    bool recovering_=false;
    void observeMidi(const rt::MidiEvent& ev) noexcept {
        const auto type=ev.status&0xf0u;
        auto& channel=notes_[ev.status&15u];
        if(type==0x90u && ev.data2) {
            // EMU8000 has 32 physical voices; bound repeats per key.
            if(channel[ev.data1]<32)++channel[ev.data1];
        } else if(type==0x80u || type==0x90u) {
            if(channel[ev.data1])--channel[ev.data1];
        }
        // Do not clear the shadow on CC120/123: some original Creative
        // implementations do not honor all panic controllers.
    }
    unsigned recoverMidi(unsigned char* out) noexcept {
        if(!queues_.midiRecoveryPending())return 0;
        if(!recovering_) {
            // The producer could fill the FIFO between our first empty
            // observation and the acquire of the recovery flag. Recheck
            // after acquiring it: no further admissions until ACK.
            rt::MidiEvent queued{};
            if(queues_.peekMidi(queued))return 0;
            recovering_=true;recoveryChannel_=recoveryPitch_=recoveryStage_=0;
        }
        // One message per pull. Sustain OFF precedes explicit NoteOffs;
        // CC120/123 are supplementary, never the only recovery mechanism.
        while(recoveryChannel_<16) {
            const auto ch=recoveryChannel_;
            if(recoveryStage_==0) {
                recoveryStage_=1;out[0]=static_cast<unsigned char>(0xb0u|ch);
                out[1]=64;out[2]=0;return 3;
            }
            if(recoveryStage_==1) {
                while(recoveryPitch_<128) {
                    auto& n=notes_[ch][recoveryPitch_];
                    if(n) {
                        --n;out[0]=static_cast<unsigned char>(0x80u|ch);
                        out[1]=static_cast<unsigned char>(recoveryPitch_);out[2]=0;return 3;
                    }
                    ++recoveryPitch_;
                }
                recoveryStage_=2;
            }
            out[0]=static_cast<unsigned char>(0xb0u|ch);out[1]=recoveryStage_==2?123:120;out[2]=0;
            if(recoveryStage_==2)recoveryStage_=3;
            else {++recoveryChannel_;recoveryPitch_=recoveryStage_=0;}
            if(recoveryChannel_==16) {
                recovering_=false;queues_.completeMidiRecovery();
            }
            return 3;
        }
        return 0;
    }
public:
    explicit DosBoxHostAdapter(rt::RealtimeQueues& q,AWE64_IsGuestMidiReadyFn ready=nullptr,
                               bool clocked=false,
                               AWE64_SetVoiceControlsFn voiceControls=nullptr,
                               AWE64_SetAbsoluteSendsFn sends=nullptr,
                               std::shared_ptr<rt::LegacyCaptureGate> captureGate={},
                               bool deferCapture=false,bool deferLegacyMidi=false,
                               guest::ResidentBankMailbox* resident=nullptr) noexcept:
        queues_(q),mailbox_(q),ready_(ready),clocked_(clocked),
        voiceControls_(voiceControls),sends_(sends),captureGate_(clocked?nullptr:std::move(captureGate)),
        resident_(resident),captureStarted_(clocked||!deferCapture),
        legacyMidiStarted_(clocked||!deferLegacyMidi){}
    void startLegacyMidi() noexcept {legacyMidiStarted_.store(true,std::memory_order_release);}
    void resetResidentTransport() noexcept {
        captureStarted_.store(false,std::memory_order_release);
        legacyMidiStarted_.store(false,std::memory_order_release);
    }
    std::uint64_t pulledNoteOns() const noexcept {return pulledNoteOns_.load(std::memory_order_relaxed);}
    void startReplacementCapture() noexcept {captureStarted_.store(true,std::memory_order_release);}
    std::uint64_t replacementSuppressedFrames() const noexcept {
        return replacementSuppressedFrames_.load(std::memory_order_relaxed);
    }
    std::uint64_t suspendedFrames() const noexcept {return suspendedFrames_.load(std::memory_order_relaxed);}
    std::uint64_t guestFrames() const noexcept {return guestFrames_.load(std::memory_order_acquire);}
    bool publishDemand(std::uint64_t horizon) noexcept {
        return clocked_&&demand_.publish(horizon);
    }
    static int AWE64_ABI_CALL tickDecision(void* ctx) noexcept {
        if(!ctx)return 0;
        auto* self=static_cast<DosBoxHostAdapter*>(ctx);
        if(!self->clocked_)return 0;
        return self->demand_.decision(!self->ready_||self->ready_(),self->guestFrames());
    }
    std::uint64_t lateMidi() const noexcept {return lateMidi_.load(std::memory_order_relaxed);}
    std::uint64_t maxLateFrames() const noexcept {return maxLateFrames_.load(std::memory_order_relaxed);}
    DosBoxHostAdapter(const DosBoxHostAdapter&)=delete;
    DosBoxHostAdapter& operator=(const DosBoxHostAdapter&)=delete;
    guest::CreativeDosMailbox& mailbox() noexcept{return mailbox_;}
    static unsigned char AWE64_ABI_CALL readPort(void* ctx,unsigned short p) noexcept {
        if(!ctx)return 0xffu;
        auto* self=static_cast<DosBoxHostAdapter*>(ctx);
        if(!self->resident_)return self->mailbox_.ioRead(p);
        return self->residentDrain_.read(*self->resident_,p,
            [self]() noexcept {self->queues_.requestMidiRecovery();},
            [self]() noexcept {
                rt::MidiEvent queued{};
                return !self->queues_.midiRecoveryPending()&&!self->queues_.peekMidi(queued);
            });
    }
    static void AWE64_ABI_CALL writePort(void* ctx,unsigned short p,
                                         unsigned char v) noexcept {
        if(!ctx)return;
        auto* self=static_cast<DosBoxHostAdapter*>(ctx);
        if(self->resident_)self->residentDrain_.write(*self->resident_,p,v);
        else self->mailbox_.ioWrite(p,v);
    }
    // Preferred path: feed byte-complete MIDI into the ORIGINAL DOSBox-X
    // MPU-401 UART. Unmodified Creative DEMO.EXE reads 330h/331h and calls
    // the actual DOS Creative SF2LIB APIs, no recompiled 16-bit guest required.
    // ONLY the DOSBox emulation thread consumes this SPSC MIDI queue.
    static unsigned int AWE64_ABI_CALL pullMidi(void* ctx,
           unsigned char* out,unsigned int capacity) noexcept {
        if(!ctx||!out||capacity<3u)return 0u;
        rt::MidiEvent ev{};
        auto* self=static_cast<DosBoxHostAdapter*>(ctx);
        auto& queue=self->queues_;
        if(self->resident_&&!self->resident_->transportOpen()){
            // Discard pending old MIDI on the sole consumer; explicitly
            // release notes before dispatch at a later UART-idle poll.
            if(!self->residentDrain_.shouldDrain(*self->resident_))return 0u;
            for(unsigned i=0;i<32;++i)if(!queue.popMidi(ev))return self->recoverMidi(out);
            return 0u;
        }
        if(!self->legacyMidiStarted_.load(std::memory_order_acquire))return 0u;
        // In clocked mode, abort undelivered packets on overflow instead
        // of playing an arbitrarily far-future backlog before recovery.
        // Already delivered notes remain tracked and are explicitly released.
        if(self->clocked_&&queue.midiRecoveryPending()) {
            for(unsigned i=0;i<32;++i)if(!queue.popMidi(ev))return self->recoverMidi(out);
            return 0;
        }
        if(!queue.peekMidi(ev))return self->recoverMidi(out);
        const auto now=self->guestFrames();
        if(self->clocked_&&ev.guestFrame>now)return 0; // never consume future packet
        const auto recordLate=[&]() noexcept {
            if(self->clocked_&&now>ev.guestFrame) {
                self->lateMidi_.fetch_add(1,std::memory_order_relaxed);
                const auto late=now-ev.guestFrame;
                // Only the guest MIDI consumer writes this maximum.
                if(late>self->maxLateFrames_.load(std::memory_order_relaxed))
                    self->maxLateFrames_.store(late,std::memory_order_relaxed);
            }
        };
        // This path must be called only after the previously emitted MIDI
        // packet has been fully consumed and its synchronous guest handler
        // returned. The native MPU pump contract is part of integration QA.
        if(ev.command!=rt::GuestCommand::Midi) {
            if(!queue.popMidi(ev))return 0u;
            recordLate();
            // Legacy and clocked controls run on the guest CPU thread.
            if(ev.command==rt::GuestCommand::VoiceControls&&self->voiceControls_)
                self->voiceControls_(ev.controlLo,ev.controlHi&65535u);
            else if(ev.command==rt::GuestCommand::AbsoluteSends&&self->sends_)
                self->sends_(static_cast<unsigned>(ev.controlLo)&65535u);
            return 0; // no synthetic byte sent to the original guest
        }
        if(ev.status==0xf0u) {
            if(!rt::validCreativeEffectType(ev)){(void)queue.popMidi(ev);return 0u;}
            if(capacity<ev.size)return 0u; // preserve FIFO ordering and whole SysEx
            if(!queue.popMidi(ev))return 0u;
            recordLate();
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
        recordLate();
        self->observeMidi(ev);
        if(category==0x90u&&ev.data2)self->pulledNoteOns_.fetch_add(1,std::memory_order_relaxed);
        return ev.size;
    }
    static void AWE64_ABI_CALL receivePCM(void* ctx,const std::int16_t* s,
                                           unsigned int frames) noexcept {
        if(!ctx||!s)return;
        auto* self=static_cast<DosBoxHostAdapter*>(ctx);
        if(self->resident_&&!self->resident_->transportOpen())return;
        // Discard hardware startup PCM before it reaches the audio ring.
        if(self->ready_&&!self->ready_())return;
        if(!self->captureStarted_.load(std::memory_order_acquire)){
            self->replacementSuppressedFrames_.fetch_add(frames,std::memory_order_relaxed);
            return;
        }
        auto& ring=self->queues_;
        constexpr float scale=1.f/32768.f;
        if(self->captureGate_){
            const auto epoch=self->captureGate_->epoch();
            if(epoch&1u){self->suspendedFrames_.fetch_add(frames,std::memory_order_relaxed);return;}
            for(unsigned int i=0;i<frames;++i){
                if(self->captureGate_->epoch()!=epoch){
                    self->suspendedFrames_.fetch_add(frames-i,std::memory_order_relaxed);break;
                }
                (void)ring.pushAudio({float(s[2u*i])*scale,float(s[2u*i+1u])*scale},epoch);
            }
        }else for(unsigned int i=0;i<frames;i++)
            (void)ring.pushAudio({float(s[2u*i])*scale,float(s[2u*i+1u])*scale});
        // Count generated frames, including dropped PCM; never pretend
        // the guest clock stopped when the host queue was full.
        if(self->clocked_)self->guestFrames_.fetch_add(frames,std::memory_order_release);
    }
};
}
