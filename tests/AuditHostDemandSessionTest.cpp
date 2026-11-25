#include "HostDemandSession.hpp"
#include "DosBoxHostAdapter.hpp"
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
using namespace awe64;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
struct Engine {
    rt::RealtimeQueues q;
    integration::DosBoxHostAdapter adapter{q,nullptr,true};
    std::uint64_t submitted=0,published=0,horizon=0;
    bool failPublish=false;
    bool submitTimed(rt::MidiBlockBatch& b,const rt::MidiTimeline& t) noexcept {
        ++submitted;return b.submit(t,q);
    }
    bool publishDemand(std::uint64_t n) noexcept {if(failPublish)return false;published=submitted;horizon=n;return adapter.publishDemand(n);}
    auto midiRecoveryEpoch() const noexcept {return q.midiRecoveryEpoch();}
    void requestMidiRecovery() noexcept {q.requestMidiRecovery();}
};
int main(){
 for(unsigned rate:{44100u,48000u,88200u,96000u,192000u}){
    auto s=std::make_unique<rt::HostDemandSession>();auto e=std::make_unique<Engine>();
    CHECK(s->configure(rate,512));CHECK(s->activate());CHECK(s->lead()>0);
    rt::HostControlState state{};std::uint64_t pos=0;std::mt19937 rng(rate);
    const auto allowed=[](unsigned p){return p<=127;};
    unsigned notes=0;std::uint64_t last=0;
    for(unsigned i=0;i<2000;++i){
        unsigned n=1+rng()%512;unsigned a=rng()%n,b=rng()%n;
        CHECK(s->begin(n));CHECK(s->note(a,0,60,1.,true));CHECK(s->note(b,0,61,1.,true));
        CHECK(s->parameter(a,1100,.5));CHECK(s->submit(*e,state,allowed)==rt::HostDemandSession::Result::Accepted);
        CHECK(e->published==e->submitted);CHECK(e->horizon==( (pos+n)*44100+rate-1)/rate+s->lead()-90);
        rt::MidiEvent event{};std::vector<std::uint64_t> at;
        while(e->q.popMidi(event)){
            CHECK(event.guestFrame>=last);last=event.guestFrame;
            if(event.command==rt::GuestCommand::Midi&&event.status==0x90){++notes;at.push_back(event.guestFrame);}
        }
        CHECK(at.size()==2);CHECK(at[0]==s->lead()+((pos+std::min(a,b))*44100+rate-1)/rate);
        CHECK(at[1]==s->lead()+((pos+std::max(a,b))*44100+rate-1)/rate);
        pos+=n;CHECK(s->position()==pos);
    }
    CHECK(notes==4000);
    CHECK(s->begin(512));for(unsigned i=0;i<4096;++i)CHECK(s->note(0,0,60,1.,true));CHECK(!s->note(0,0,60,1.,true));
    CHECK(s->submit(*e,state,allowed)==rt::HostDemandSession::Result::Recovery);CHECK(e->q.midiRecoveryPending());CHECK(s->position()==pos+512);
    unsigned char out[11];for(unsigned i=0;i<100&&e->q.midiRecoveryPending();++i)e->adapter.pullMidi(&e->adapter,out,11);
    CHECK(!e->q.midiRecoveryPending());CHECK(s->begin(1));CHECK(s->note(0,0,65,1.,true));CHECK(s->submit(*e,state,allowed)==rt::HostDemandSession::Result::Accepted);
    rt::MidiEvent x{};unsigned count=0;while(e->q.popMidi(x))++count;CHECK(count==7); // force resync then note
    e->failPublish=true;CHECK(s->begin(1));CHECK(s->submit(*e,state,allowed)==rt::HostDemandSession::Result::Fault);CHECK(!s->begin(1));
    std::printf("rate %u: 2000 blocks/4000 notes, horizon, recovery and sticky fault PASS\n",rate);
 }
}
