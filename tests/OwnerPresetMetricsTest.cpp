#include "OwnerPresetMetrics.hpp"
#include <cstdio>
#include <sstream>
#include <limits>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
    using namespace awe64::qa;
    std::vector<OwnerProbe> rows;
    std::istringstream good("3\n2 48 65\n2 60 100\n77 72 115\n");
    CHECK(readOwnerProbes(good,rows)&&rows.size()==3&&rows[0][0]==2&&rows[2][2]==115);
    for(auto invalid:{"0", "1\n128 60 100", "1\n0 128 100", "1\n0 60 0", "2\n0 60 1\n0 60 1", "2\n0 60 1", "1\n0 60 1\n4", "1\n-1 60 1"}){
        std::istringstream in(invalid);CHECK(!readOwnerProbes(in,rows)&&rows.size()==3);
    }
    float l[512],r[512];
    for(unsigned i=0;i<512;++i){l[i]=.01f;r[i]=-.01f;}
    OwnerMetrics dc;CHECK(dc.add(l,r,512));CHECK(dc.rms()>.009&&dc.acMax<1e-8&&dc.jumps==0);
    for(unsigned i=0;i<512;++i){l[i]=.1f*std::sin(6.283185307179586*i/100.);r[i]=-l[i];}
    OwnerMetrics sine;CHECK(sine.add(l,r,512));CHECK(sine.acMax>.065&&sine.acMax<.075&&sine.stepMax<.007&&sine.jumps==0);
    OwnerMetrics pulse;for(auto& x:l)x=0;for(auto& x:r)x=0;l[31]=.8f;
    CHECK(pulse.add(l,r,512)&&pulse.peak>.79&&pulse.acMax>.02&&pulse.jumps==2);
    l[31]=0;OwnerMetrics crossing;CHECK(crossing.add(l,r,512));r[0]=.4f;
    CHECK(crossing.add(l,r,512)&&crossing.stepMax>.39&&crossing.jumps==2);
    l[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!crossing.add(l,r,512));
    std::puts("OWNER_METRICS_PASS: strict plan, DC not audible AC, sine, one-shot, cross-block discontinuity, nonfinite reject");
}
