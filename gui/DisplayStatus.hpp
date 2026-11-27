#pragma once
#include <string>
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
    static const char* hardware(int state) noexcept {
        return state == 2 ? "32 VOICES | 44.1 kHz | READY" :
               state == 1 ? "AWE64 GOLD | INITIALIZING" :
               state < 0 ? "AWE64 GOLD | NOT READY" :
                           "AWE64 GOLD | INACTIVE";
    }
    static std::string bankName(bool accepted,const std::string& path) {
        if(!accepted)return {};
        return path.empty() ? std::string("SYNTHGM.SF2") : path;
    }
};
}
