#include "DisplayStatus.hpp"
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
    return 0;
}
