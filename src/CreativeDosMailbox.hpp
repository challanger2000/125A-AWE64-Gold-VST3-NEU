#pragma once
// Host <-> original Creative DOS guest: a hardware-I/O mailbox prototype.
//
// Intended usage: DOSBox's I/O port callbacks call ioRead/ioWrite solely from
// the guest emulation thread. The VST3 audio thread calls only
// RealtimeQueues::pushMidi(); both remain isolated by the SPSC queue.
//
// IMPORTANT: The actual DOSBox I/O port registration, boot and Creative guest
// executable are NOT wired into the VST3 yet. This file is not an audio engine.
#include "RealtimeQueues.hpp"
#include <cstdint>

namespace awe64::guest {
class CreativeDosMailbox final {
public:
    static constexpr std::uint16_t kBase=0xBEE0;
    static constexpr std::uint16_t kAck=0xBEF0;
    static constexpr std::uint8_t kAckOk=0xA5;
    static constexpr std::uint8_t kAckError=0x5A;

private:
    rt::RealtimeQueues& queues_;
    rt::MidiEvent current_{};
    bool pending_=false;
    std::uint64_t processed_=0;
    std::uint64_t failed_=0;
    std::uint64_t unexpectedAcks_=0;

    void fetchIfNeeded() noexcept {
        if(!pending_)pending_=queues_.popMidi(current_);
    }
public:
    explicit CreativeDosMailbox(rt::RealtimeQueues& queues) noexcept
        :queues_(queues) {}
    CreativeDosMailbox(const CreativeDosMailbox&)=delete;
    CreativeDosMailbox& operator=(const CreativeDosMailbox&)=delete;

    // Call only on the DOSBox emulation thread.
    std::uint8_t ioRead(std::uint16_t port) noexcept {
        if(port<kBase||port>kBase+15u)return 0xFF;
        fetchIfNeeded();
        const auto offset=static_cast<unsigned>(port-kBase);
        if(offset==0)return static_cast<std::uint8_t>(1u|(pending_?2u:0u));
        if(!pending_)return 0u;
        switch(offset) {
            case 1:return current_.status;
            case 2:return current_.data1;
            case 3:return current_.data2;
            case 4:return current_.size;
            default:
                if(offset>=8 && offset<=15)
                    return static_cast<std::uint8_t>(
                        (current_.absoluteHostSample>>(8u*(offset-8u)))&255u);
                return 0u;
        }
    }
    void ioWrite(std::uint16_t port,std::uint8_t value) noexcept {
        if(port!=kAck)return;
        if(!pending_ || (value!=kAckOk && value!=kAckError)) {
            ++unexpectedAcks_;
            return;
        }
        if(value==kAckError)++failed_;
        ++processed_;
        pending_=false;
    }
    std::uint64_t processed() const noexcept{return processed_;}
    std::uint64_t failed() const noexcept{return failed_;}
    std::uint64_t unexpectedAcks() const noexcept{return unexpectedAcks_;}
};
} // namespace awe64::guest
