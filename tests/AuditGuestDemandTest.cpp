#include "GuestDemandClock.hpp"
#include "MidiTimeline.hpp"
#include <cstdio>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
using namespace awe64::rt;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
    unsigned l=0;
    CHECK(!GuestDemandClock::leadFor(32000,128,l));
    CHECK(!GuestDemandClock::leadFor(44100,0,l));
    CHECK(!GuestDemandClock::leadFor(44100,32768,l));
    for(auto rate:{44100u,48000u,88200u,96000u,192000u})for(auto maxBlock:{64u,128u,512u,2048u,8192u}){
        GuestDemandClock clock;
        unsigned lead=0;CHECK(GuestDemandClock::leadFor(rate,maxBlock,lead));
        std::uint64_t horizon=0,produced=0,consumed=0,host=0,ticks=0;
        CHECK(clock.decision(false,0)==0);
        CHECK(GuestDemandClock::blockHorizon(0,lead,horizon));CHECK(clock.publish(horizon));
        auto pump=[&](){while(clock.decision(true,produced)==1){const auto next=(++ticks*441u)/10u;produced=next;}return clock.decision(true,produced)==2;};
        CHECK(pump());
        MidiTimeline t;CHECK(t.configure(rate));CHECK(t.anchor(0,0,lead));
        std::mt19937 random(rate+maxBlock);
        for(unsigned b=0;b<10000;++b){
            auto n=1+random()%maxBlock;
            MidiEvent event{host,0x90,60,100,3};CHECK(t.stamp(event));
            // Must be impossible for the virtual CPU to pass the next event
            // timestamp before that host block has even been submitted.
            CHECK(produced<event.guestFrame);
            // Host can read this block from the prefilled ring even if the
            // producer does not execute another tick during process().
            auto nextConsumed=((host+n)*44100+rate-1)/rate;
            // Higher-rate converter may pull one extra lookahead input.
            CHECK(produced>=nextConsumed+1);
            consumed=nextConsumed;host+=n;
            CHECK(GuestDemandClock::blockHorizon(consumed,lead,horizon));CHECK(clock.publish(horizon));
            CHECK(pump());CHECK(produced-consumed<GuestDemandClock::ringFrames);
        }
        CHECK(!clock.publish(horizon-1));
        CHECK(!GuestDemandClock::blockHorizon(std::numeric_limits<std::uint64_t>::max(),lead,horizon));
        std::printf("rate=%u block_max=%u lead_native=%u produced=%llu consumed=%llu PASS\n",rate,maxBlock,lead,(unsigned long long)produced,(unsigned long long)consumed);
    }
}
