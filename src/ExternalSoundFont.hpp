#pragma once
#include "Sf2CreativeDosCompatibility.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace awe64::guest {
// Host-side validation/staging only. The original Creative loader owns
// sample upload and playback. Never modify or overwrite the source bank.
inline bool readExternalSf2(const std::filesystem::path& path,
                            std::vector<std::uint8_t>& bytes,
                            bool& adapted) {
    std::error_code ec;
    const auto size=std::filesystem::file_size(path,ec);
    if(ec || size<12 || size>32ull*1024*1024)return false;
    std::ifstream input(path,std::ios::binary);
    if(!input)return false;
    std::vector<std::uint8_t> candidate(static_cast<std::size_t>(size));
    if(!input.read(reinterpret_cast<char*>(candidate.data()),
                   static_cast<std::streamsize>(candidate.size())))return false;
    const auto result=stageCreativeDosBank(candidate);
    if(result!=BankStageResult::OriginalSf2_200 &&
       result!=BankStageResult::AdaptedSf2_201)return false;
    adapted=result==BankStageResult::AdaptedSf2_201;
    bytes.swap(candidate);
    return true;
}
inline bool writeExternalSf2(const std::filesystem::path& destination,
                             const std::vector<std::uint8_t>& bytes) {
    if(std::filesystem::exists(destination))return false;
    std::ofstream output(destination,std::ios::binary|std::ios::out);
    if(!output)return false;
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    output.close();
    return bool(output);
}
}
