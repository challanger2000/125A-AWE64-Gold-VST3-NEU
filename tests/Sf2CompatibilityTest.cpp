#include "Sf2CreativeDosCompatibility.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
using namespace awe64::guest;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAILED line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
void le32put(std::vector<std::uint8_t>& d,std::size_t at,std::uint32_t v){
    for(int i=0;i<4;i++)d.at(at+i)=static_cast<std::uint8_t>(v>>(8*i));
}
std::vector<std::uint8_t> makeBank(unsigned major,unsigned minor){
    std::vector<std::uint8_t> v{
        'R','I','F','F',0,0,0,0,'s','f','b','k',
        'L','I','S','T',16,0,0,0,'I','N','F','O',
        'i','f','i','l',4,0,0,0,
        static_cast<std::uint8_t>(major),static_cast<std::uint8_t>(major>>8),
        static_cast<std::uint8_t>(minor),static_cast<std::uint8_t>(minor>>8),
        'L','I','S','T',14,0,0,0,'s','d','t','a',
        's','m','p','l',2,0,0,0,0x57,0x33
    };
    le32put(v,4,static_cast<std::uint32_t>(v.size()-8));
    return v;
}
int main(){
    auto bank=makeBank(2,1);
    const auto pristine=bank;
    CHECK(stageCreativeDosBank(bank)==BankStageResult::AdaptedSf2_201);
    CHECK(bank.size()==pristine.size());
    unsigned changes=0;
    for(std::size_t i=0;i<bank.size();i++){
        if(bank[i]!=pristine[i]){++changes;CHECK(i==34);}
    }
    CHECK(changes==1);
    CHECK(bank[34]==0);
    CHECK(stageCreativeDosBank(bank)==BankStageResult::OriginalSf2_200);
    auto sbk=makeBank(1,0),old=sbk;
    CHECK(stageCreativeDosBank(sbk)==BankStageResult::LegacySbk_100);
    CHECK(sbk==old);
    auto unsupported=makeBank(2,2),save=unsupported;
    CHECK(stageCreativeDosBank(unsupported)==BankStageResult::UnsupportedVersion);
    CHECK(unsupported==save);
    auto bad=makeBank(2,1);
    bad[0]='X';save=bad;
    CHECK(stageCreativeDosBank(bad)==BankStageResult::InvalidRiff);
    CHECK(bad==save);
    auto truncated=makeBank(2,1);
    truncated.pop_back();save=truncated;
    CHECK(stageCreativeDosBank(truncated)==BankStageResult::InvalidRiff);
    CHECK(truncated==save);
    auto missing=makeBank(2,1);
    std::memcpy(missing.data()+24,"NOT!",4);
    CHECK(stageCreativeDosBank(missing)==BankStageResult::MissingVersion);
    auto duplicate=makeBank(2,1);
    // A second valid version chunk in the INFO LIST.
    duplicate.insert(duplicate.begin()+36,{'i','f','i','l',4,0,0,0,2,0,1,0});
    le32put(duplicate,16,28);
    le32put(duplicate,4,static_cast<std::uint32_t>(duplicate.size()-8));
    CHECK(stageCreativeDosBank(duplicate)==BankStageResult::DuplicateVersion);
    auto outOfBounds=makeBank(2,1);
    le32put(outOfBounds,28,0xffffffff);
    CHECK(stageCreativeDosBank(outOfBounds)==BankStageResult::InvalidRiff);
    std::puts("Creative DOS SF2 version-metadata staging checks passed");
}
