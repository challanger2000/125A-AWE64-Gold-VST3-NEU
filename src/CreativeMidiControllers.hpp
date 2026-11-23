#pragma once
#include "RealtimeQueues.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace awe64::rt {
// Host VST3 parameter IDs are stable. No emulation or interpretation of CCs:
// the original Creative DOS guest decides their meaning.
inline constexpr unsigned midiControllerBase=2000, midiControllerStride=16;
struct MidiControllerSpec {unsigned number; const char* name; double initial;};
inline constexpr std::array<MidiControllerSpec,12> midiControllerSpecs{{
    {1,"Modulation",0.},{7,"Channel Volume",100./127.},{10,"Pan",64./127.},
    {11,"Expression",1.},{64,"Sustain",0.},{91,"Reverb CC",0.},{93,"Chorus CC",0.},
    {120,"All Sound Off",0.},{121,"Reset Controllers",0.},{123,"All Notes Off",0.},
    {128,"Channel Pressure",0.},{129,"Pitch Bend",0.5}
}};
inline int midiControllerIndex(unsigned number) noexcept {
    for(unsigned i=0;i<midiControllerSpecs.size();++i)
        if(midiControllerSpecs[i].number==number)return static_cast<int>(i);
    return -1;
}
inline bool makeMappedController(unsigned id,double normalized,std::uint64_t time,MidiEvent& event) noexcept {
    if(id<midiControllerBase || !std::isfinite(normalized))return false;
    const unsigned slot=(id-midiControllerBase)/midiControllerStride;
    const auto ch=static_cast<std::uint8_t>((id-midiControllerBase)%midiControllerStride);
    if(slot>=midiControllerSpecs.size())return false;
    const auto number=midiControllerSpecs[slot].number;
    const double v=std::clamp(normalized,0.,1.);
    const auto value=static_cast<std::uint8_t>(std::lround(v*127.));
    if(number==129) {
        const auto bend=static_cast<unsigned>(std::lround(v*16383.));
        event={time,static_cast<std::uint8_t>(0xe0|ch),static_cast<std::uint8_t>(bend&127),static_cast<std::uint8_t>(bend>>7),3};
    } else if(number==128) {
        event={time,static_cast<std::uint8_t>(0xd0|ch),value,0,2};
    } else {
        event={time,static_cast<std::uint8_t>(0xb0|ch),static_cast<std::uint8_t>(number),value,3};
    }
    return true;
}
} // namespace awe64::rt
