#pragma once
// Test-host CPU oracle. A successful OS query alone is not a measurement.
#include <cmath>
#include <cstdint>
namespace awe64::qa {
struct CpuCounters {std::uint64_t time100ns=0,cycles=0;bool valid=false;};
struct CpuUsage {double oneCorePercent=0,machinePercent=0;};
inline bool cpuDelta(const CpuCounters& a,const CpuCounters& b,std::uint64_t& ticks) noexcept {
    if(!a.valid||!b.valid||b.time100ns<=a.time100ns||b.cycles<=a.cycles)return false;
    ticks=b.time100ns-a.time100ns;return true;
}
inline bool cpuPositiveControl(const CpuCounters& processBefore,const CpuCounters& processAfter,
                              const CpuCounters& threadBefore,const CpuCounters& threadAfter) noexcept {
    std::uint64_t processTicks=0,threadTicks=0;
    if(!cpuDelta(processBefore,processAfter,processTicks)||!cpuDelta(threadBefore,threadAfter,threadTicks))return false;
    // At least 1 ms CPU in an intentionally busy calling thread. Take the
    // process measurements outside the thread interval, allowing one 16 ms
    // accounting quantum for independent Windows counter update boundaries.
    constexpr std::uint64_t quantum=160000;
    return threadTicks>=10000&&(processTicks>=threadTicks||threadTicks-processTicks<=quantum);
}
inline bool cpuUsage(const CpuCounters& a,const CpuCounters& b,double seconds,unsigned logicalCpus,CpuUsage& output) noexcept {
    std::uint64_t ticks=0;
    if(!cpuDelta(a,b,ticks)||!std::isfinite(seconds)||seconds<=0||!logicalCpus)return false;
    const double oneCore=double(ticks)*1.e-5/seconds;
    const double machine=oneCore/logicalCpus;
    // A small accounting-boundary tolerance; no owner-CPU acceptance claim.
    if(!std::isfinite(oneCore)||!std::isfinite(machine)||oneCore<=0||machine>105.)return false;
    output={oneCore,machine};return true;
}
}
