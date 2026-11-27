#include "ExternalSoundFont.hpp"
#include <fstream>
int main(int argc,char** argv){
    if(argc!=3)return 2;
    std::vector<std::uint8_t> v{19,29};bool adapted=false;
    if(!awe64::guest::readExternalSf2(argv[1],v,adapted))return v==std::vector<std::uint8_t>{19,29}?1:3;
    std::ofstream out(argv[2],std::ios::binary);
    out.write(reinterpret_cast<const char*>(v.data()),v.size());return out?0:4;
}
