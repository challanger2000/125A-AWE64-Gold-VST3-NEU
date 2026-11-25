#include "DosBoxHostAdapter.hpp"
#include "MidiTimeline.hpp"
#include <array>
#include <cstdio>
#include <memory>
using namespace awe64;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(false)
static bool ready=false;
static int guestReady(){return ready;}
int main(){
    auto q=std::make_unique<rt::RealtimeQueues>();
    auto other=std::make_unique<rt::RealtimeQueues>();
    integration::DosBoxHostAdapter a(*q,&guestReady,true), b(*other,nullptr,false);
    CHECK(a.publishDemand(384));
    CHECK(a.tickDecision(&a)==0);CHECK(b.tickDecision(&b)==0);
    std::array<std::int16_t,90> pcm{};
    a.receivePCM(&a,pcm.data(),45);CHECK(a.guestFrames()==0);
    ready=true;
    CHECK(a.tickDecision(&a)==1);
    for(unsigned i=0;i<9;++i)a.receivePCM(&a,pcm.data(),45);
    CHECK(a.guestFrames()==405);CHECK(a.tickDecision(&a)==2);
    CHECK(a.publishDemand(512));CHECK(a.tickDecision(&a)==1);
    CHECK(!a.publishDemand(511));CHECK(!b.publishDemand(512));
    rt::MidiTimeline t;CHECK(t.configure(44100));CHECK(t.anchor(0,0,512));
    rt::MidiBlockBatch batch;CHECK(batch.add({0,0x90,60,100,3},rt::MidiPhase::Note));CHECK(batch.submit(t,*q));
    unsigned char out[11]{};CHECK(a.pullMidi(&a,out,11)==0);
    for(unsigned i=0;i<3;++i)a.receivePCM(&a,pcm.data(),45);
    CHECK(a.tickDecision(&a)==2);CHECK(a.pullMidi(&a,out,11)==3&&out[1]==60);
    CHECK(a.maxLateFrames()==28);CHECK(other->snapshot().audioFrames==0);
    std::puts("Actual adapter clock callback: boot/advance/hold, native PCM frame count, timestamp gate, late accounting, independent legacy instance PASS");
}
