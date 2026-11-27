#pragma once
#include "Sf2StructuralValidation.hpp"
#include <cstdint>
#include <vector>

namespace awe64::guest {
// Byte count of the actual 16-bit sample payload supplied to Creative.
// No file size estimate, no speculative count from presets. The original
// loader transfers the smpl chunk, including its interpolation guards.
// ROM-only banks do not contain RAM samples. Allocator overhead is excluded.
inline constexpr std::uint32_t unknownSampleRam=0xffffffffu;
inline std::uint32_t sampleRamBytes(const std::vector<std::uint8_t>& bank) noexcept {
    using namespace sf2preflight;
    if(bank.size()<12||!eq4(bank.data(),"RIFF")||!eq4(bank.data()+8,"sfbk")||
       std::uint64_t(le32(bank.data()+4))+8!=bank.size())return unknownSampleRam;
    Chunk sdta{},smpl{};
    if(!walk({bank.data()+12,bank.size()-12},[&](const auto* tag,Chunk c){
        if(!eq4(tag,"LIST"))return true;
        if(c.size<4)return false;
        return !eq4(c.p,"sdta")||assign(sdta,{c.p+4,c.size-4});
    }))return unknownSampleRam;
    if(sdta.p&&!walk(sdta,[&](const auto* tag,Chunk c){
        return !eq4(tag,"smpl")||assign(smpl,c);
    }))return unknownSampleRam;
    if(smpl.size%2||smpl.size>28u*1024u*1024u)return unknownSampleRam;
    return static_cast<std::uint32_t>(smpl.size);
}
}
