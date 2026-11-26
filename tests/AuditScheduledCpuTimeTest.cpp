#include "ScheduledCpuTime.hpp"
#include <cstdio>
using namespace awe64::qa;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
    using K=SchedKind;
    std::vector<SchedEvent> e{{0,K::Start,10,7},{1,K::Start,11,7},{2,K::Start,12,8},
        {5,K::Switch,0,10},{8,K::Switch,0,11},{10,K::Switch,10,12},
        {15,K::Switch,12,10},{20,K::Switch,11,0},{22,K::End,11,7},
        {23,K::Start,11,8},{25,K::Switch,10,11},{30,K::Switch,11,0},{40,K::Switch,0,0}};
    std::array<SchedWindow,3> w{{{6,24},{9,16},{25,35}}};std::array<std::uint64_t,3> t{};
    CHECK(scheduledTime(e,7,w,t));CHECK(t[0]==25&&t[1]==9&&t[2]==0);
    auto missing=e;missing.erase(missing.begin()+3);CHECK(!scheduledTime(missing,7,w,t));
    auto duplicate=e;duplicate.push_back({9,K::Switch,0,10});CHECK(!scheduledTime(duplicate,7,w,t));
    auto truncated=e;truncated.pop_back();CHECK(!scheduledTime(truncated,7,w,t));
    std::reverse(e.begin(),e.end());CHECK(scheduledTime(e,7,w,t));CHECK(t[0]==25&&t[1]==9);
    double ratio=0.;
    CHECK(callbackScheduledRatio(e,7,10,{6,24},ratio));CHECK(ratio==13./18.);
    CHECK(!callbackScheduledRatio(e,7,99,{6,24},ratio)&&ratio==-1.);
    CHECK(!callbackScheduledRatio(e,7,10,{11,14},ratio)&&ratio==-1.);
    CHECK(!callbackScheduledRatio(missing,7,10,{6,24},ratio)&&ratio==-1.);
    CHECK(!callbackScheduledRatio(e,7,0,{6,24},ratio)&&ratio==-1.);
    w[0]={8,8};CHECK(!scheduledTime(e,7,w,t));
    std::puts("SCHEDULED_CPU_TIME_PASS");
}
