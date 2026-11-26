#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace awe64::qa {
enum class SchedKind { Start, End, Switch };
struct SchedEvent {std::uint64_t tick; SchedKind kind; std::uint32_t a,b;};
struct SchedWindow {std::uint64_t begin,end;};
// Start/End: a=thread ID, b=process ID. Switch: a=old, b=new thread ID.
// Raw QPC time, never CPU cycles. Input is sorted after complete ETW capture.
// Thread ownership is snapshotted on switch-in, not guessed from event header.
template<std::size_t N>
bool scheduledTime(std::vector<SchedEvent>& events,std::uint32_t pid,
                   const std::array<SchedWindow,N>& windows,
                   std::array<std::uint64_t,N>& totals,std::uint32_t onlyTid=0) {
    totals.fill(0);
    if(!pid||events.empty())return false;
    for(auto w:windows)if(w.begin>=w.end)return false;
    std::stable_sort(events.begin(),events.end(),[](auto a,auto b){return a.tick<b.tick;});
    std::uint64_t end=0,begin=std::numeric_limits<std::uint64_t>::max();
    for(auto w:windows){end=std::max(end,w.end);begin=std::min(begin,w.begin);}
    if(events.front().tick>begin||events.back().tick<end)return false;
    std::unordered_set<std::uint32_t> owned;
    std::unordered_map<std::uint32_t,std::uint64_t> active;
    auto add=[&](std::uint64_t a,std::uint64_t b){
        for(std::size_t i=0;i<N;++i){const auto lo=std::max(a,windows[i].begin),hi=std::min(b,windows[i].end);
            if(hi>lo){if(totals[i]>std::numeric_limits<std::uint64_t>::max()-(hi-lo))return false;totals[i]+=hi-lo;}}
        return true;
    };
    for(auto e:events){
        if(e.tick>end)break;
        if(e.kind==SchedKind::Start){if(e.b==pid&&(!onlyTid||onlyTid==e.a))owned.insert(e.a);else owned.erase(e.a);continue;}
        if(e.kind==SchedKind::End){if(e.b==pid)owned.erase(e.a);continue;}
        if(e.a==e.b)continue;
        auto out=active.find(e.a);
        if(out!=active.end()){if(!add(out->second,e.tick))return false;active.erase(out);}
        else if(owned.count(e.a)&&e.tick>=begin)return false; // missing switch-in
        if(owned.count(e.b)){if(!active.emplace(e.b,e.tick).second)return false;}
    }
    for(auto [tid,start]:active)if(!add(start,end))return false;
    return true;
}
// Callback executes at both QPC endpoints. Zero scheduled ticks cannot
// be a valid observation of that thread, even if unrelated ETW data exist.
inline bool callbackScheduledRatio(std::vector<SchedEvent>& events,std::uint32_t pid,std::uint32_t tid,SchedWindow window,double& ratio) {
    ratio=-1.;
    if(!tid||window.begin>=window.end)return false;
    std::array<SchedWindow,1> w{window};std::array<std::uint64_t,1> t{};
    if(!scheduledTime(events,pid,w,t,tid)||!t[0]||t[0]>window.end-window.begin)return false;
    ratio=double(t[0])/double(window.end-window.begin);return true;
}
}
