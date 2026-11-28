#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace awe64 { namespace overlay {
// Zero bypasses each individual overlay exactly. Positions 1..255 are
// absolute musical controls, not interpolation from the preset: even a
// slow preset can be made short. The deliberate 0 -> 1 transition enters
// override mode; monotonicity applies within the active range.
using Controls = std::array<std::uint8_t,10>;
constexpr unsigned activeValue(unsigned amount,unsigned maximum) noexcept {
    return ((amount-1u)*maximum+127u)/254u;
}
// Pinned EMU8000 rate encoding: 127 = fastest, 0 = stopped, not instant.
// Attack rate 6 is ~1.98 s; decay/release rate 12 ~3.96 s per 100 dB.
// Linear register travel gives a useful nonlinear time response.
constexpr unsigned activeRate(unsigned amount,unsigned slowest) noexcept {
    return 127u-activeValue(amount,127u-slowest);
}
constexpr std::uint16_t delay(std::uint16_t w,unsigned amount) noexcept {
    // Attack controls the attack itself, not an additional silent delay.
    return !amount||(w&0x8000u)?w:std::uint16_t(0x8000u);
}
constexpr std::uint16_t attack(std::uint16_t w,unsigned amount) noexcept {
    if(!amount)return w;
    // Keep trigger/reserved flags; remove the original hold interval only
    // when the user explicitly overrides Attack.
    return static_cast<std::uint16_t>((w&0x8080u)|0x7f00u|activeRate(amount,6u));
}
constexpr std::uint16_t decaySustainRelease(std::uint16_t w,unsigned decay,
        unsigned sustain,unsigned release) noexcept {
    // Preserve release/engine flags and the original release destination.
    if(w&0x8000u)return !release?w:static_cast<std::uint16_t>((w&0xff80u)|activeRate(release,12u));
    const auto rate=decay?activeRate(decay,12u):unsigned(w&127u);
    const auto level=sustain?activeValue(sustain,127u):unsigned((w>>8)&127u);
    return static_cast<std::uint16_t>((w&0x8080u)|rate|(level<<8));
}
struct FilterWords {std::uint16_t ifatn,pefe;};
constexpr FilterWords filter(std::uint16_t ifatn,std::uint16_t pefe,const Controls& c) noexcept {
    // Active cutoff: closed to open. Preserve attenuation, signed existing
    // modulation and pitch routing. With no existing filter route, provide
    // a bounded positive envelope depth independent of individual ADSR
    // positions; preserve the requested fully-open envelope target.
    auto cutoff=c[8]?activeValue(c[8],255u):unsigned(ifatn>>8);
    const bool envelope=c[4]||c[5]||c[6]||c[7];
    if(envelope && !(pefe&255u)) {
        const auto depth=(std::min)(64u,cutoff/2u);
        pefe=static_cast<std::uint16_t>((pefe&0xff00u)|depth);
        // At active low cutoff, quantize the target down by at most one
        // register step rather than alternate the base between 0 and 1.
        cutoff=c[8]?(cutoff>128u?cutoff-128u:0u):cutoff-2u*depth;
    }
    return {static_cast<std::uint16_t>((cutoff<<8)|(ifatn&255u)),pefe};
}
constexpr std::uint16_t resonance(std::uint16_t cccaHigh,unsigned amount) noexcept {
    // Caller must separately prove musical voice context, not a DMA write.
    return !amount?cccaHigh:static_cast<std::uint16_t>((cccaHigh&0xfffu)|(activeValue(amount,15u)<<12));
}
}} // namespace awe64::overlay
