#pragma once
#include <atomic>
#include <cstdint>
#include <limits>

namespace awe64::rt {
// Producer-owned native frame horizon, observed by guest CPU only at
// the original DOSBox millisecond boundary. No wall-clock assumptions.
// The first PCM callback after authentic guest readiness is frame zero.
// 0 = original wall-clock pacing (boot/reference), 1 = one virtual tick,
// 2 = yield without advancing emulated time.
class GuestDemandClock final {
    std::atomic<std::uint64_t> horizon_{0};
public:
    static constexpr unsigned maxTickFrames=45;
    static constexpr unsigned reserveFrames=384;
    static constexpr unsigned ringFrames=32768;
    static bool leadFor(unsigned rate,unsigned maxHostBlock,unsigned& lead) noexcept {
        if(rate<44100||rate>192000||!maxHostBlock)return false;
        const auto block=(std::uint64_t(maxHostBlock)*44100+rate-1)/rate;
        const auto l=block+reserveFrames+2*maxTickFrames;
        if(l+block+maxTickFrames>=ringFrames)return false;
        lead=static_cast<unsigned>(l);return true;
    }
    // Host producer only. No backwards horizon, including after underruns.
    bool publish(std::uint64_t end) noexcept {
        if(end<horizon_.load(std::memory_order_relaxed))return false;
        horizon_.store(end,std::memory_order_release);return true;
    }
    std::uint64_t horizon() const noexcept {return horizon_.load(std::memory_order_acquire);}
    int decision(bool ready,std::uint64_t produced) const noexcept {
        if(!ready)return 0;
        return produced<horizon()?1:2;
    }
    // Host block needs at least one full tick of scheduling separation
    // between the previous producer horizon and the next event deadline.
    static bool blockHorizon(std::uint64_t hostEndNative,unsigned lead,
                             std::uint64_t& result) noexcept {
        if(lead<2*maxTickFrames || hostEndNative>std::numeric_limits<std::uint64_t>::max()-lead)return false;
        result=hostEndNative+lead-2*maxTickFrames;return true;
    }
};
}
