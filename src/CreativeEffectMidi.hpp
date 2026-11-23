#pragma once
#include "RealtimeQueues.hpp"
namespace awe64::rt {
// Creative ADIP v3.01, Part II, DOS awe32Sysex API: GS effect variation
// macros recognized by the genuine Creative DOS MIDI engine.
// https://doczz.net/doc/8351592/developer-s-information-pack
inline MidiEvent makeCreativeEffectType(std::uint64_t time,bool chorus,std::uint8_t type) noexcept {
    MidiEvent ev{time,0xf0,0,0,11};
    const auto address=static_cast<std::uint8_t>(chorus?0x38:0x30);
    type=static_cast<std::uint8_t>(type&7u);
    const auto checksum=static_cast<std::uint8_t>((128u-(0x40u+1u+address+type)%128u)%128u);
    ev.sysex={0xf0,0x41,0x10,0x42,0x12,0x40,0x01,address,type,checksum,0xf7};
    return ev;
}
inline bool validCreativeEffectType(const MidiEvent& ev) noexcept {
    return ev.status==0xf0 && ev.size==11 && ev.sysex[7]==0x30+8*(ev.sysex[7]==0x38) &&
        ev.sysex[8]<8 && ev.sysex==makeCreativeEffectType(0,ev.sysex[7]==0x38,ev.sysex[8]).sysex;
}
}
