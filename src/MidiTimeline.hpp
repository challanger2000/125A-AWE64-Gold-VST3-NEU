#pragma once
#include "RealtimeQueues.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace awe64::rt {
// Producer-owned timeline. Anchor is supplied once after guest PCM is ready.
// It is deliberately not an implicit wall-clock or per-block re-anchor.
// Scheduling target = anchor guest frame + explicit lead + ceil(host delta * 44100 / host rate).
class MidiTimeline final {
    static constexpr std::uint64_t nativeRate=44100;
    unsigned rate_=0;
    std::uint64_t hostAnchor_=0,guestAnchor_=0;
    bool anchored_=false;
public:
    bool configure(unsigned rate) noexcept {
        reset();
        rate_=rate>=44100&&rate<=192000?rate:0;
        return rate_!=0;
    }
    void reset() noexcept {anchored_=false;hostAnchor_=guestAnchor_=0;}
    bool anchor(std::uint64_t host,std::uint64_t guest,unsigned lead) noexcept {
        if(!rate_||anchored_||guest>std::numeric_limits<std::uint64_t>::max()-lead)return false;
        hostAnchor_=host;guestAnchor_=guest+lead;anchored_=true;return true;
    }
    bool stamp(MidiEvent& event) const noexcept {
        if(!anchored_||event.absoluteHostSample<hostAnchor_)return false;
        const auto d=event.absoluteHostSample-hostAnchor_;
        // Quotient/remainder avoids overflow in delta*44100.
        const auto frames=(d/rate_)*nativeRate;
        const auto rem=((d%rate_)*nativeRate+rate_-1)/rate_;
        constexpr auto max=std::numeric_limits<std::uint64_t>::max();
        if(frames>max-rem||guestAnchor_>max-frames-rem)return false;
        event.guestFrame=guestAnchor_+frames+rem;return true;
    }
};
// One host block; preallocated member, never a heap allocation in process().
// Host queues are separate, so their iteration order is not event time order.
// At the same offset, explicit phase order: program/setup -> controller -> notes.
// Preserve the original order within each phase, including NoteOff/NoteOn order.
enum class MidiPhase : unsigned {Setup,Controller,Note};
inline MidiEvent makeVoiceControlCommand(std::uint64_t hostSample,std::uint64_t lo,unsigned hi) noexcept {
    MidiEvent e{};e.absoluteHostSample=hostSample;e.command=GuestCommand::VoiceControls;
    e.controlLo=lo;e.controlHi=hi&65535u;return e;
}
inline MidiEvent makeAbsoluteSendCommand(std::uint64_t hostSample,unsigned packed) noexcept {
    MidiEvent e{};e.absoluteHostSample=hostSample;e.command=GuestCommand::AbsoluteSends;
    e.controlLo=packed&65535u;return e;
}
class MidiBlockBatch final {
    struct Entry {MidiEvent event{}; MidiPhase phase{}; unsigned ordinal=0;};
    static constexpr unsigned capacity_=4096;
    std::array<Entry,capacity_> entries_{};
    unsigned size_=0;
    bool invalid_=false;
public:
    void reset() noexcept {size_=0;invalid_=false;}
    bool add(const MidiEvent& event,MidiPhase phase) noexcept {
        if(invalid_||size_==capacity_){invalid_=true;return false;}
        entries_[size_]={event,phase,size_};++size_;return true;
    }
    bool hasNoteOn() const noexcept {
        for(unsigned i=0;i<size_;++i){const auto& e=entries_[i].event;
            if(e.command==GuestCommand::Midi&&(e.status&0xf0u)==0x90u&&e.data2)return true;}
        return false;
    }
    void order() noexcept {
        const auto compare=[](const Entry& a,const Entry& b) noexcept {
            if(a.event.absoluteHostSample!=b.event.absoluteHostSample)return a.event.absoluteHostSample<b.event.absoluteHostSample;
            if(a.phase!=b.phase)return a.phase<b.phase;
            return a.ordinal<b.ordinal;
        };
        // In-place heapsort: bounded O(n log n), no recursion/auxiliary buffer.
        std::make_heap(entries_.begin(),entries_.begin()+size_,compare);
        std::sort_heap(entries_.begin(),entries_.begin()+size_,compare);
    }
    template<class Send> bool submitLegacy(Send&& send) noexcept {
        if(invalid_){reset();return false;}
        order();
        bool ok=true;
        // Journal all rejected persistent controls, not only the first.
        for(unsigned i=0;i<size_;++i)if(!send(entries_[i].event))ok=false;
        reset();return ok;
    }
    bool submit(const MidiTimeline& timeline,RealtimeQueues& queue) noexcept {
        if(!invalid_)for(unsigned i=0;i<size_;++i)if(!timeline.stamp(entries_[i].event)){invalid_=true;break;}
        if(invalid_){queue.requestMidiRecovery();reset();return false;}
        order();
        bool ok=true;
        for(unsigned i=0;i<size_;++i)if(!queue.pushMidi(entries_[i].event)){ok=false;break;}
        reset();return ok;
    }
};
} // namespace awe64::rt
