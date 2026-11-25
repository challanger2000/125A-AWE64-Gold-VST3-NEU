#pragma once
// Test-host CPU oracle. A successful OS query alone is not a measurement.
#include <cmath>
#include <cstdint>
namespace awe64::qa {
struct CpuCounters {std::uint64_t time100ns=0,cycles=0;bool valid=false;};
struct CpuUsage {double oneCorePercent=0,machinePercent=0;};
enum class CpuCounterStatus { Ok, QueryFailed, Regressed, FlatTime, FlatCycles };
struct CpuInterval {std::uint64_t ticks=0,cycles=0;CpuCounterStatus status=CpuCounterStatus::QueryFailed;};
inline CpuInterval cpuInterval(const CpuCounters& a,const CpuCounters& b) noexcept {
    if(!a.valid||!b.valid)return {};
    if(b.time100ns<a.time100ns||b.cycles<a.cycles)return {0,0,CpuCounterStatus::Regressed};
    const auto t=b.time100ns-a.time100ns,c=b.cycles-a.cycles;
    return {t,c,!t?CpuCounterStatus::FlatTime:(!c?CpuCounterStatus::FlatCycles:CpuCounterStatus::Ok)};
}
inline const char* cpuStatusName(CpuCounterStatus s) noexcept {
    switch(s){
        case CpuCounterStatus::Ok:return "OK";
        case CpuCounterStatus::QueryFailed:return "QUERY_FAILED";
        case CpuCounterStatus::Regressed:return "REGRESSED";
        case CpuCounterStatus::FlatTime:return "FLAT_TIME";
        case CpuCounterStatus::FlatCycles:return "FLAT_CYCLES";
    }
    return "UNKNOWN";
}
inline bool cpuDelta(const CpuCounters& a,const CpuCounters& b,std::uint64_t& ticks) noexcept {
    const auto r=cpuInterval(a,b);
    if(r.status!=CpuCounterStatus::Ok)return false;
    ticks=r.ticks;return true;
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
