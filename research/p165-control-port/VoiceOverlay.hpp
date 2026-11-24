#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace awe64 { namespace overlay {
// Quantized normalized controls, zero = exact original register word.
using Controls = std::array<std::uint8_t,10>;
constexpr unsigned scale(unsigned n,unsigned amount) noexcept {
    return (n*amount+127u)/255u;
}
constexpr unsigned faster(unsigned n,unsigned amount) noexcept {
    return n+scale(127u-n,amount);
}
constexpr std::uint16_t delay(std::uint16_t w,unsigned amount) noexcept {
    if(!amount||(w&0x8000u))return w;
    return static_cast<std::uint16_t>(w+scale(0x8000u-w,amount));
}
constexpr std::uint16_t attack(std::uint16_t w,unsigned amount) noexcept {
    return static_cast<std::uint16_t>((w&0x8080u)|faster(w&127u,amount)|
        (faster((w>>8)&127u,amount)<<8));
}
constexpr std::uint16_t decaySustainRelease(std::uint16_t w,unsigned decay,
        unsigned sustain,unsigned release) noexcept {
    // Preserve release/trigger and engine bits in every raw hardware word.
    if(w&0x8000u)return static_cast<std::uint16_t>((w&0xff80u)|faster(w&127u,release));
    const auto s=(w>>8)&127u;
    return static_cast<std::uint16_t>((w&0x8080u)|faster(w&127u,decay)|
        ((s-scale(s,sustain))<<8));
}
struct FilterWords {std::uint16_t ifatn,pefe;};
constexpr FilterWords filter(std::uint16_t ifatn,std::uint16_t pefe,const Controls& c) noexcept {
    auto cutoff=unsigned(ifatn>>8);
    const auto depthControl=(std::max)({c[4],c[5],c[6],c[7]});
    // Existing signed filter and pitch modulation routes stay intact.
    if(depthControl && !(pefe&255u)) {
        const auto depth=(std::min)(scale(96u,depthControl),cutoff/2u);
        pefe=static_cast<std::uint16_t>((pefe&0xff00u)|depth);
        cutoff-=2u*depth;
    }
    cutoff-=scale(cutoff,c[8]);
    return {static_cast<std::uint16_t>((cutoff<<8)|(ifatn&255u)),pefe};
}
constexpr std::uint16_t resonance(std::uint16_t cccaHigh,unsigned amount) noexcept {
    // Caller must separately prove musical voice context, not a DMA write.
    const auto q=unsigned(cccaHigh>>12);
    return static_cast<std::uint16_t>((cccaHigh&0xfffu)|((q+scale(15u-q,amount))<<12));
}
}} // namespace awe64::overlay
