#include "DisplayStatus.hpp"
#include "../src/Sf2SampleRam.hpp"
#include <string>
#include <array>
int main() {
    using D=AWE125::DisplayStatus;
    constexpr std::array<int,7> states{0,1,2,-1,-2,-3,-4};
    const std::array<std::string,7> expected{"System inactive","Operating system booting...","System ready",
        "Error: engine or original files missing","Error: system stopped","Error: startup timeout",
        "Error: default SoundFont missing or invalid"};
    for(unsigned i=0;i<states.size();++i) {
        if(D::status(states[i])!=expected[i])return 1;
        const std::string hardware=D::hardware(states[i]);
        if(hardware.find("DOSBOX")!=std::string::npos||hardware.find("WIN95")!=std::string::npos)return 2;
    }
    if(!D::bankName(false,"").empty()||!D::bankName(false,"user.sf2").empty())return 3;
    if(D::bankName(true,"")!="SYNTHGM.SF2"||D::bankName(true,"user.sf2")!="user.sf2")return 4;
    if(D::hardware(2,0)!="32 VOICES | 44.1 kHz | RAM 0.00 / 28 MB")return 5;
    if(D::hardware(2,6945750)!="32 VOICES | 44.1 kHz | RAM 6.62 / 28 MB")return 6;
    if(D::hardware(2,28u*1048576u)!="32 VOICES | 44.1 kHz | RAM 28.00 / 28 MB")return 7;
    if(D::hardware(2).find("RAM --")==std::string::npos||D::hardware(1,6945750).find("RAM")!=std::string::npos)return 8;
    // Tiny generated RIFF: 8 bytes of sample data; INFO/PDTA/file bytes
    // must not enter the RAM count. Truncation and duplicate smpl rejected.
    using B=std::vector<std::uint8_t>;
    auto append=[](B& out,const B& in){out.insert(out.end(),in.begin(),in.end());};
    auto chunk=[&](const char* tag,const B& data){
        B b(tag,tag+4);auto n=static_cast<std::uint32_t>(data.size());
        for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(n>>(8*i)));
        append(b,data);if(n&1)b.push_back(0);return b;
    };
    auto make=[&](bool samples,bool duplicate,bool odd){
        B body{'s','f','b','k'}, info{'I','N','F','O'}, sdta{'s','d','t','a'};
        append(info,chunk("INAM",B{'T','e','s','t',0,0}));
        append(body,chunk("LIST",info));
        if(samples){
            append(sdta,chunk("smpl",B(odd?7:8,42)));
            if(duplicate)append(sdta,chunk("smpl",B(8,0)));
            append(body,chunk("LIST",sdta));
        }
        append(body,chunk("LIST",B{'p','d','t','a'}));
        return chunk("RIFF",body);
    };
    using namespace awe64::guest;
    if(sampleRamBytes(make(false,false,false))!=0||sampleRamBytes(make(true,false,false))!=8)return 9;
    if(sampleRamBytes(make(true,true,false))!=unknownSampleRam||sampleRamBytes(make(true,false,true))!=unknownSampleRam)return 10;
    auto truncated=make(true,false,false);truncated.pop_back();
    if(sampleRamBytes(truncated)!=unknownSampleRam)return 11;
    return 0;
}
