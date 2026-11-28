#pragma once
#include "ResidentBankMailbox.hpp"

namespace awe64::guest {
// Owned exclusively by the original DOS CPU thread. The host must close
// admissions with begin() under the lifecycle lock before releasing it.
// First idle UART poll requests MIDI recovery. Only a later idle poll, after
// old queued events are discarded and explicit releases emitted, may expose
// the command. No locks, allocations, sleeps or I/O on this boundary.
class ResidentBankDrain final {
    bool started_=false;
    bool delivered_=false;
public:
    bool shouldDrain(const ResidentBankMailbox& mailbox) const noexcept {
        return started_&&!delivered_&&mailbox.state()==ResidentBankMailbox::State::Queued;
    }
    template<class StartRecovery,class IsDrained>
    std::uint8_t read(ResidentBankMailbox& mailbox,unsigned short port,
                      StartRecovery start,IsDrained drained) noexcept {
        if(port!=ResidentBankMailbox::commandPort)return 0xff;
        if(mailbox.state()!=ResidentBankMailbox::State::Queued||delivered_)return 0;
        if(!started_){started_=true;start();return 0;}
        if(!drained())return 0;
        const auto command=mailbox.read(port);
        if(command)delivered_=true;
        return command;
    }
    void write(ResidentBankMailbox& mailbox,unsigned short port,std::uint8_t value) noexcept {
        if(port!=ResidentBankMailbox::ackPort)return;
        started_=false;delivered_=false;
        mailbox.write(port,value);
    }
};
}
