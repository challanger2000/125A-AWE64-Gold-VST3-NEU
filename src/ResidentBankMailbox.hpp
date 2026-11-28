#pragma once
#include <atomic>
#include <cstdint>

namespace awe64::guest {
// Single non-RT host owner, one DOS CPU thread per isolated Engine DLL.
// Host calls begin/finish/fail; DOS thread calls read/write via existing ABI.
// Audio/MIDI callbacks only query transportOpen. No file I/O or waits here.
class ResidentBankMailbox final {
public:
    enum class State : unsigned { Booting, Idle, Queued, Busy, Loaded, Rejected, Restored, Failed };
    static constexpr unsigned short commandPort=0xbee0,ackPort=0xbef0;
    static constexpr std::uint8_t bankA=0xa1,bankB=0xa2,busy=0x11,
        rejected=0xe1,restored=0xe2,fatal=0xe3;
private:
    std::atomic<State> state_{State::Booting};
    std::atomic<std::uint8_t> command_{0},target_{0},current_{0};
    static bool isBank(std::uint8_t b) noexcept {return b==bankA||b==bankB;}
public:
    State state() const noexcept {return state_.load(std::memory_order_acquire);}
    std::uint8_t current() const noexcept {return current_.load(std::memory_order_acquire);}
    bool transportOpen() const noexcept {return state()==State::Idle;}
    bool begin(std::uint8_t slot) noexcept {
        if(state()!=State::Idle||!isBank(slot)||slot==current())return false;
        target_.store(slot,std::memory_order_relaxed);
        State expected=State::Idle;
        if(!state_.compare_exchange_strong(expected,State::Queued,std::memory_order_acq_rel))return false;
        command_.store(slot,std::memory_order_release);
        return true;
    }
    std::uint8_t read(unsigned short port) noexcept {
        if(port!=commandPort)return 0xff;
        if(state()!=State::Queued)return 0;
        return command_.exchange(0,std::memory_order_acq_rel);
    }
    void write(unsigned short port,std::uint8_t value) noexcept {
        if(port!=ackPort)return;
        const auto old=state();
        if(old==State::Failed)return;
        auto next=State::Failed;
        if(old==State::Booting&&value==bankA){
            current_.store(bankA,std::memory_order_release);next=State::Idle;
        }else if(old==State::Queued&&value==busy&&command_.load(std::memory_order_acquire)==0){
            next=State::Busy;
        }else if(old==State::Busy){
            if(value==target_.load(std::memory_order_acquire))next=State::Loaded;
            else if(value==rejected)next=State::Rejected;
            else if(value==restored)next=State::Restored;
        }
        auto expected=old;
        if(!state_.compare_exchange_strong(expected,next,std::memory_order_acq_rel))return;
    }
    // Host commits new catalog, selection, parameters and discards old PCM
    // before finish. Do not reopen transport merely because guest ACK arrived.
    bool finish() noexcept {
        auto old=state();
        if(old!=State::Loaded&&old!=State::Rejected&&old!=State::Restored)return false;
        if(old==State::Loaded)current_.store(target_.load(std::memory_order_acquire),std::memory_order_release);
        return state_.compare_exchange_strong(old,State::Idle,std::memory_order_acq_rel);
    }
    // A destructive load cannot simply be canceled. Stop/quarantine the guest
    // on timeout/fatal. Late ACK must never reopen a failed transport.
    void fail() noexcept {
        state_.store(State::Failed,std::memory_order_release);
        command_.store(0,std::memory_order_release);
    }
};
}
