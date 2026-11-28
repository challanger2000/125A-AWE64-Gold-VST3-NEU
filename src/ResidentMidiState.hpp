#pragma once
#include "RealtimeQueues.hpp"
#include <array>
#include <cstdint>

namespace awe64::guest {
// Latest desired persistent MIDI state on the serialized host producer.
// A resident Creative reload calls awe32InitMIDI(), so transport recovery
// alone is insufficient. Preserve the last CC121 and relative ordering of
// each latest controller before/after it; the original driver decides what
// CC121 actually resets. Never replay notes, sustain or panic commands.
class ResidentMidiState final {
    struct Entry { rt::MidiEvent event{}; std::uint64_t sequence=0; };
    std::array<std::array<Entry,9>,16> channels_{};
    std::uint64_t sequence_=0;
    static int index(const rt::MidiEvent& e) noexcept {
        if(e.command!=rt::GuestCommand::Midi||e.data1>127||e.data2>127)return -1;
        switch(e.status&0xf0u){
            case 0xb0:
                if(e.size!=3)return -1;
                switch(e.data1){
                    case 1:return 0;case 7:return 1;case 10:return 2;
                    case 11:return 3;case 91:return 4;case 93:return 5;
                    case 121:return 8;default:return -1;
                }
            case 0xd0:return e.size==2?6:-1;
            case 0xe0:return e.size==3?7:-1;
            default:return -1;
        }
    }
public:
    void remember(const rt::MidiEvent& e) noexcept {
        const int i=index(e);
        if(i<0)return;
        // 64-bit sequence cannot wrap during a practical plugin lifetime.
        channels_[e.status&15u][static_cast<unsigned>(i)]={e,++sequence_};
    }
    template<class Send> bool replay(Send&& send) const noexcept {
        // At most 144 entries, each sent once in its original relative order
        // within its channel. This runs before reopening the resident mailbox.
        for(const auto& ch:channels_){
            std::uint64_t previous=0;
            for(unsigned n=0;n<ch.size();++n){
                const Entry* next=nullptr;
                for(const auto& e:ch)
                    if(e.sequence>previous&&(!next||e.sequence<next->sequence))next=&e;
                if(!next)break;
                auto event=next->event;event.absoluteHostSample=0;event.guestFrame=0;
                if(!send(event))return false;
                previous=next->sequence;
            }
        }
        return true;
    }
};
}
