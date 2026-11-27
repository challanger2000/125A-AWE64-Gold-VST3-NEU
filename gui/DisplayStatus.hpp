#pragma once
#include <string>
#include <cstdint>
#include <cstdio>
namespace AWE125 {
struct DisplayStatus {
    static const char* status(int state) noexcept {
        switch(state) {
            case 2: return "System ready";
            case 1: return "Operating system booting...";
            case -1: return "Error: engine or original files missing";
            case -2: return "Error: system stopped";
            case -3: return "Error: startup timeout";
            case -4: return "Error: default SoundFont missing or invalid";
            default: return "System inactive";
        }
    }
    // Real sample payload in the accepted, staged bank, not file size or
    // process memory. SF2 metadata and hardware allocator overhead excluded.
    static constexpr std::uint32_t unknownRam=0xffffffffu;
    static std::string hardware(int state,std::uint32_t sampleBytes=unknownRam) {
        if(state==2){
            if(sampleBytes==unknownRam)return "32 VOICES | 44.1 kHz | RAM -- / 28 MB";
            // Binary MB, consistent with the emulated 28 MiB RAM capacity.
            const auto hundredths=(std::uint64_t(sampleBytes)*100u+524288u)/1048576u;
            char text[96]{};
            std::snprintf(text,sizeof(text),"32 VOICES | 44.1 kHz | RAM %u.%02u / 28 MB",
                          static_cast<unsigned>(hundredths/100u),static_cast<unsigned>(hundredths%100u));
            return text;
        }
        return state==1?"AWE64 GOLD | INITIALIZING":
               state<0?"AWE64 GOLD | NOT READY":"AWE64 GOLD | INACTIVE";
    }
    static std::string bankName(bool accepted,const std::string& path) {
        if(!accepted)return {};
        return path.empty() ? std::string("SYNTHGM.SF2") : path;
    }
};
}
