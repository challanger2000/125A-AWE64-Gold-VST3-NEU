#pragma once
// Non-realtime, in-memory compatibility staging ONLY.
// Original owner's SF2/SBK file is never overwritten. Audio DSP untouched.
// Creative DOS SF2LIB 3.01 (P30) rejected 'ifil' v2.01 but accepted v2.00.
// This adjusts ONLY the format-version metadata of a PRIVATE working copy.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace awe64::guest {
enum class BankStageResult {
    InvalidRiff,
    MissingVersion,
    DuplicateVersion,
    UnsupportedVersion,
    OriginalSf2_200,
    AdaptedSf2_201,
    LegacySbk_100,
};
inline constexpr bool isUsable(BankStageResult r) noexcept {
    return r==BankStageResult::OriginalSf2_200 ||
           r==BankStageResult::AdaptedSf2_201 ||
           r==BankStageResult::LegacySbk_100;
}
inline std::uint16_t le16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<unsigned>(p[1])<<8);
}
inline std::uint32_t le32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1])<<8) |
           (static_cast<std::uint32_t>(p[2])<<16) |
           (static_cast<std::uint32_t>(p[3])<<24);
}
inline bool eq4(const std::uint8_t* p, const char* s) noexcept {
    return p[0]==static_cast<std::uint8_t>(s[0]) &&
           p[1]==static_cast<std::uint8_t>(s[1]) &&
           p[2]==static_cast<std::uint8_t>(s[2]) &&
           p[3]==static_cast<std::uint8_t>(s[3]);
}
// Mutates the caller's PRIVATE COPY on success; leaves all data intact on failure.
// Carefully bounds every RIFF and LIST chunk; does not use string searching
// (which can accidentally match a sample's bytes).
inline BankStageResult stageCreativeDosBank(std::vector<std::uint8_t>& bank) noexcept {
    const std::size_t fileSize=bank.size();
    if(fileSize<12||!eq4(bank.data(),"RIFF")||!eq4(bank.data()+8,"sfbk"))
        return BankStageResult::InvalidRiff;
    const std::size_t end=static_cast<std::size_t>(le32(bank.data()+4))+8u;
    if(end!=fileSize||end<12)return BankStageResult::InvalidRiff;
    bool found=false;
    std::size_t ifilAt=0;
    for(std::size_t outer=12;outer<end;) {
        if(end-outer<8)return BankStageResult::InvalidRiff;
        const auto length=static_cast<std::size_t>(le32(bank.data()+outer+4));
        const auto data=outer+8u;
        if(length>end-data)return BankStageResult::InvalidRiff;
        if(eq4(bank.data()+outer,"LIST")) {
            if(length<4)return BankStageResult::InvalidRiff;
            if(eq4(bank.data()+data,"INFO")) {
                const auto stop=data+length;
                for(std::size_t inner=data+4;inner<stop;) {
                    if(stop-inner<8)return BankStageResult::InvalidRiff;
                    const auto n=static_cast<std::size_t>(le32(bank.data()+inner+4));
                    const auto begin=inner+8u;
                    if(n>stop-begin)return BankStageResult::InvalidRiff;
                    if(eq4(bank.data()+inner,"ifil")) {
                        if(found)return BankStageResult::DuplicateVersion;
                        if(n!=4)return BankStageResult::InvalidRiff;
                        found=true;
                        ifilAt=begin;
                    }
                    const auto extra=n&1u;
                    if(extra>stop-begin-n)return BankStageResult::InvalidRiff;
                    inner=begin+n+extra;
                }
            }
        }
        const auto extra=length&1u;
        if(extra>end-data-length)return BankStageResult::InvalidRiff;
        outer=data+length+extra;
    }
    if(!found)return BankStageResult::MissingVersion;
    const auto major=le16(bank.data()+ifilAt);
    const auto minor=le16(bank.data()+ifilAt+2);
    if(major==1u && minor==0u)return BankStageResult::LegacySbk_100;
    if(major==2u && minor==0u)return BankStageResult::OriginalSf2_200;
    if(major==2u && minor==1u) {
        bank[ifilAt+2]=0u;
        bank[ifilAt+3]=0u;
        return BankStageResult::AdaptedSf2_201;
    }
    return BankStageResult::UnsupportedVersion;
}
} // namespace awe64::guest
