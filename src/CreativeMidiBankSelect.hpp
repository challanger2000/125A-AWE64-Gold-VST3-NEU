#pragma once
// Creative's original DOS DEMO routes MIDI CC 0 to awe32Controller(channel,0,bank).
// The bank is the MSB (controller 0), not LSB (controller 32).
#include "RealtimeQueues.hpp"
namespace awe64::rt {
[[nodiscard]] constexpr MidiEvent makeCreativeBankSelect(std::uint64_t sample,
    std::uint8_t bank, std::uint8_t channel=0) noexcept {
    return {sample,static_cast<std::uint8_t>(0xB0u|(channel&0x0fu)),
            0u,bank,3u};
}
[[nodiscard]] constexpr MidiEvent makeCreativeProgramChange(std::uint64_t sample,
    std::uint8_t program, std::uint8_t channel=0) noexcept {
    return {sample,static_cast<std::uint8_t>(0xC0u|(channel&0x0fu)),
            program,0u,2u};
}
} // namespace awe64::rt
