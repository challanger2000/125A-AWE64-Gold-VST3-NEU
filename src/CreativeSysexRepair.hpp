#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>
namespace awe64::guest {
// Proven by original negative and repaired positive Windows guest tests.
// Does not authorize a different executable or an already modified image.
inline bool repairCreativeSysexCompare(std::vector<std::uint8_t>& image) noexcept {
    constexpr std::size_t offset=21834+0x35;
    constexpr std::array<std::uint8_t,18> signature{
        0x2e,0x8a,0x04,0x98,0x8e,0x46,0x0a,0x8b,0xd9,
        0x03,0xdf,0x2a,0xf6,0x26,0x8a,0x17,0x3b,0xd0};
    if(image.size()!=40197||!std::equal(signature.begin(),signature.end(),image.begin()+offset))return false;
    image[offset+3]=0x90;
    return true;
}
}
