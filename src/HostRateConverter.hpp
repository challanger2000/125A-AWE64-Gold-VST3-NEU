#pragma once
#include "RealtimeQueues.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace awe64::rt {
// The original 44.1 kHz DOSBox mixer and chip are unchanged. Convert only
// its final stereo PCM to a higher DAW rate, using a causal windowed sinc.
// 44.1 kHz is deliberately bit-transparent and bypasses this converter.
// All coefficients are prepared on the non-realtime setup thread.
class HostRateConverter final {
    static constexpr unsigned taps=32, phases=1024;
    static constexpr double pi=3.14159265358979323846;
    std::array<std::array<float,taps>,phases+1> table_{};
    std::array<StereoFrame,taps> history_{};
    unsigned newest_=0;
    // Exact rational phase: no floating-point clock drift.
    unsigned rate_=44100, remainder_=0;
    bool primed_=false;
public:
    static constexpr unsigned inputRate=44100, delayInputSamples=16;
    static bool supports(double rate) noexcept {
        return std::isfinite(rate) && rate>=44100. && rate<=192000. &&
               std::abs(rate-std::round(rate))<0.01;
    }
    bool configure(double rate) noexcept {
        if(!supports(rate))return false;
        rate_=static_cast<unsigned>(std::lround(rate));
        if(rate_!=inputRate) {
            for(unsigned p=0;p<=phases;++p) {
                double sum=0.;
                for(unsigned k=0;k<taps;++k) {
                    const double x=double(k)+double(p)/phases-delayInputSamples;
                    const double sinc=std::abs(x)<1.e-12?1.:std::sin(pi*x)/(pi*x);
                    // Blackman window, 32-tap support. No per-sample trig.
                    const double w=std::abs(x)<16.?
                        0.42+0.5*std::cos(pi*x/16.)+0.08*std::cos(2.*pi*x/16.):0.;
                    table_[p][k]=static_cast<float>(sinc*w);
                    sum+=table_[p][k];
                }
                for(auto& c:table_[p])c=static_cast<float>(c/sum);
            }
        }
        reset();
        return true;
    }
    void reset() noexcept {
        history_.fill({0.f,0.f});newest_=0;remainder_=0;primed_=false;
    }
    bool bypass() const noexcept{return rate_==inputRate;}
    std::size_t requiredInputFrames(std::size_t count) const noexcept {
        if(bypass()||!count)return count;
        // render() pulls one initial sample and one per native phase wrap,
        // including the wrap following the last output sample.
        return (primed_?0u:1u)+static_cast<std::size_t>((std::uint64_t(remainder_)+std::uint64_t(count)*inputRate)/rate_);
    }
    unsigned latencySamples() const noexcept {
        return bypass()?0u:(delayInputSamples*rate_+inputRate-1)/inputRate;
    }
    // Pull returns exactly one stereo source sample, or a zero on underrun.
    // No allocation, locks, or file access in this routine.
    template<class Pull>
    void render(float* l,float* r,std::size_t count,Pull&& pull) noexcept {
        if(bypass()) {
            for(std::size_t i=0;i<count;++i){const auto f=pull();l[i]=f.left;r[i]=f.right;}
            return;
        }
        if(count&&!primed_){history_[newest_]=pull();primed_=true;}
        for(std::size_t i=0;i<count;++i) {
            const unsigned scaled=remainder_*phases;
            const unsigned index=scaled/rate_;
            const float frac=float(scaled%rate_)/rate_;
            double left=0.,right=0.;
            for(unsigned k=0;k<taps;++k) {
                const auto& x=history_[(newest_+taps-k)%taps];
                const float h=table_[index][k]+frac*(table_[index+1][k]-table_[index][k]);
                left+=x.left*h;right+=x.right*h;
            }
            l[i]=static_cast<float>(left);r[i]=static_cast<float>(right);
            remainder_+=inputRate;
            if(remainder_>=rate_) {
                remainder_-=rate_;
                newest_=(newest_+1)%taps;
                history_[newest_]=pull();
            }
        }
    }
};
}
