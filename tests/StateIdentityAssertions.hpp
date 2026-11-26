#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
// Test-only structural oracle for migrating a neutral v2 state.
inline bool matchesMigratedExternalState(const char* data,std::size_t size,const std::vector<std::uint8_t>& legacy) {
    if(!data||legacy.size()<15||legacy[4]!=2||size!=legacy.size()+46)return false;
    const auto* raw=reinterpret_cast<const std::uint8_t*>(data);
    for(unsigned i=0;i<11;++i)if(raw[i]!=(i==4?6:legacy[i]))return false;
    if(raw[11]!=4||raw[12]!=2)return false;
    for(unsigned i=13;i<25;++i)if(raw[i])return false;
    if(!std::equal(legacy.begin()+11,legacy.end(),raw+25))return false;
    return std::any_of(raw+size-32,raw+size,[](auto x){return x!=0;});
}
