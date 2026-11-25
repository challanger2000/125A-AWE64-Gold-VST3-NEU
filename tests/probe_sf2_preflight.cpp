#include "ExternalSoundFont.hpp"
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
int main(int argc,char** argv){
    if(argc<3)return 2;
    bool all=true;
    for(int i=2;i<argc;++i){
        std::vector<std::uint8_t> bytes{0x19,0x29};bool adapted=false;
        const auto path=std::filesystem::path(argv[i]);
        const bool accepted=awe64::guest::readExternalSf2(path,bytes,adapted);
        const bool expected=std::string(argv[1])=="accept";
        const bool rollback=accepted||bytes==std::vector<std::uint8_t>{0x19,0x29};
        std::printf("%s expected=%d accepted=%d adapted=%d rollback=%d\n",path.filename().string().c_str(),expected,accepted,adapted,rollback);
        all &= accepted==expected && rollback;
    }
    return all?0:1;
}
