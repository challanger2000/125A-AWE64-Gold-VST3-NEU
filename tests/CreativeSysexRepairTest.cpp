#include "CreativeSysexRepair.hpp"
#include <iostream>
int main() {
    std::vector<std::uint8_t> src(40197);
    constexpr std::uint8_t sig[]={0x2e,0x8a,0x04,0x98,0x8e,0x46,0x0a,0x8b,0xd9,0x03,0xdf,0x2a,0xf6,0x26,0x8a,0x17,0x3b,0xd0};
    constexpr auto at=21834+0x35;
    std::copy(std::begin(sig),std::end(sig),src.begin()+at);
    auto fixed=src;
    if(!awe64::guest::repairCreativeSysexCompare(fixed))return 1;
    src[at+3]=0x90;
    if(src!=fixed||awe64::guest::repairCreativeSysexCompare(fixed))return 2;
    std::vector<std::uint8_t> bad(40196);
    if(awe64::guest::repairCreativeSysexCompare(bad))return 3;
    std::cout<<"CREATIVE_SYSEX_REPAIR_ONE_BYTE_PASS\n";
}
