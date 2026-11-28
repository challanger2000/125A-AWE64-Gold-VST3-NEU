#include "ResidentStartupPolicy.hpp"
#include <cstdio>
using awe64::guest::ResidentStartup;
using awe64::guest::selectResidentStartup;
int main(){
    using S=ResidentStartup;
    // Independently enumerate the required decisions, not the implementation.
    const S expected[4][8]={
        {S::Legacy,S::Invalid,S::Invalid,S::Resident,S::Legacy,S::Legacy,S::Legacy,S::Legacy},
        {S::Legacy,S::Legacy,S::Legacy,S::Legacy,S::Legacy,S::Legacy,S::Legacy,S::Legacy},
        {S::Invalid,S::Invalid,S::Invalid,S::Resident,S::Invalid,S::Invalid,S::Invalid,S::Invalid},
        {S::Invalid,S::Invalid,S::Invalid,S::Invalid,S::Invalid,S::Invalid,S::Invalid,S::Invalid}};
    const char* modes[]={nullptr,"0","1","unexpected"};
    for(unsigned m=0;m<4;++m)for(unsigned n=0;n<8;++n)
        if(selectResidentStartup(modes[m],(n&4)!=0,(n&2)!=0,(n&1)!=0)!=expected[m][n])return 1;
    if(selectResidentStartup("",false,true,true)!=S::Invalid)return 2;
    std::puts("RESIDENT_STARTUP_POLICY_33_CASES_PASS");
}
