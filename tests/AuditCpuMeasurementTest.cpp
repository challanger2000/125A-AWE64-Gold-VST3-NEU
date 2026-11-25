#include "PerfCpuMeasurement.hpp"
#include <cstdio>
#include <limits>
#include <cmath>
using namespace awe64::qa;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
    CpuCounters a{1000000,100,true},b{2000000,200,true};CpuUsage u{};
    CHECK(cpuUsage(a,b,1.,4,u));CHECK(std::abs(u.oneCorePercent-10.)<1e-12&&std::abs(u.machinePercent-2.5)<1e-12);
    CHECK(cpuPositiveControl(a,b,{1000,10,true},{501000,50,true}));
    CHECK(!cpuPositiveControl(a,a,{1000,10,true},{501000,50,true}));
    CHECK(!cpuPositiveControl(a,b,{1000,10,true},{1000,50,true}));
    CHECK(!cpuPositiveControl(a,b,{1000,10,true},{1001,50,true}));
    CHECK(!cpuPositiveControl(a,b,{1000,10,true},{9999999,50,true}));
    const auto saved=u;
    CHECK(!cpuUsage(a,a,1.,4,u));CHECK(!cpuUsage(a,{a.time100ns,200,true},1.,4,u));
    CHECK(!cpuUsage(a,{b.time100ns,a.cycles,true},1.,4,u));CHECK(!cpuUsage(b,a,1.,4,u));
    CHECK(!cpuUsage(a,{b.time100ns,b.cycles,false},1.,4,u));CHECK(!cpuUsage(a,b,0.,4,u));
    CHECK(!cpuUsage(a,b,-1.,4,u));CHECK(!cpuUsage(a,b,std::numeric_limits<double>::quiet_NaN(),4,u));
    CHECK(!cpuUsage(a,b,std::numeric_limits<double>::infinity(),4,u));CHECK(!cpuUsage(a,b,1.,0,u));
    CHECK(!cpuUsage(a,b,.001,4,u));CHECK(u.oneCorePercent==saved.oneCorePercent&&u.machinePercent==saved.machinePercent);
    CHECK(cpuUsage(a,{21000000,200,true},1.,4,u)&&std::abs(u.oneCorePercent-200.)<1e-10&&std::abs(u.machinePercent-50.)<1e-10);
    std::puts("CPU_ORACLE_PASS: nonzero monotonic time AND cycles, independent thread positive control, invalid counters and units rejected");
}
