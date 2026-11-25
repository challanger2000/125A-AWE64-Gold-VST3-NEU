#include "MasterGainPlan.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
    awe64::rt::MasterGainPlan p;
    float l[512],r[512];
    auto reset=[&]{for(int i=0;i<512;++i){l[i]=.5f;r[i]=-.25f;}};
    reset();p.begin(512,1.f);
    CHECK(p.add(0,0.)&&p.add(255,0.)&&p.add(256,1.)&&p.add(511,1.));
    p.apply(l,r);for(int i=0;i<512;++i){CHECK(l[i]==(i<256?0.f:.5f));CHECK(r[i]==(i<256?0.f:-.25f));}
    CHECK(p.last()==1.f);
    reset();p.begin(512,0.f);CHECK(p.add(511,1.));p.apply(l,r);
    for(int i=0;i<512;++i)CHECK(std::abs(l[i]-.5f*(i+1)/512.f)<1e-7f);
    p.begin(0,1.f);CHECK(p.add(0,.25)&&p.add(0,.5)&&p.last()==.5f);
    CHECK(!p.add(1,1.)&&!p.add(0,std::numeric_limits<double>::quiet_NaN()));
    p.begin(512,1.f);CHECK(p.add(128,.5)&&!p.add(127,.8)&&!p.add(512,.8));
    std::puts("MASTER_GAIN_PLAN_PASS exact zero, step, ramp, duplicate, flush, invalid points");
}
