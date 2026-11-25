#include "HostEventPlan.hpp"
#include "DosBoxHostAdapter.hpp"
#include <cstdio>
#include <memory>
#include <vector>
#include <random>
#include <limits>
using namespace awe64;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
struct Seen {std::uint64_t at,voice;unsigned sends;unsigned pitch,vel,status;};
static std::uint64_t voiceLo=0;static unsigned sends=0;
static void setVoice(std::uint64_t lo,unsigned){voiceLo=lo;}
static void setSend(unsigned s){sends=s;}
int main(){
    auto p=std::make_unique<rt::HostEventPlan>();auto b=std::make_unique<rt::MidiBlockBatch>();
    auto q=std::make_unique<rt::RealtimeQueues>();integration::DosBoxHostAdapter a(*q,nullptr,true,setVoice,setSend);
    rt::HostControlState s{};rt::MidiTimeline t;CHECK(t.configure(44100));CHECK(t.anchor(0,0,100));
    const auto allowed=[](unsigned p){return p==0||p==17;};
    p->begin(64);
    CHECK(p->note(50,3,63,1.,true));CHECK(p->note(10,3,60,.5,true));
    CHECK(p->parameter(50,1100,1.));CHECK(p->parameter(10,1100,.25));
    CHECK(p->parameter(50,1110,.5));CHECK(p->parameter(10,1110,0.));
    CHECK(p->parameter(50,1000,17./127));CHECK(p->parameter(20,1000,1.)); // invalid sparse program ignored
    CHECK(p->parameter(50,2000+4*16+3,1.)); // sustain same offset
    CHECK(p->note(50,3,60,0.,false));
    CHECK(p->parameter(0,1001,0.));CHECK(p->parameter(63,1001,1.));
    CHECK(p->build(0,s,*b,allowed));CHECK(s.program==17&&s.voice[0]==255&&s.reverbSend==128&&s.master==1.f);
    float l[64],r[64];std::fill_n(l,64,1.f);std::fill_n(r,64,-1.f);p->applyMaster(l,r,1.f);
    for(unsigned i=0;i<64;++i){CHECK(std::abs(l[i]-float(i)/63)<1e-6);CHECK(r[i]==-l[i]);}
    CHECK(b->submit(t,*q));p->committed(s);
    std::vector<Seen> seen;
    std::int16_t pcm[2]{};unsigned char out[11]{};
    for(unsigned frame=0;frame<165;++frame){
        for(unsigned j=0;j<40;++j){auto n=a.pullMidi(&a,out,11);if(n==2||n==3)seen.push_back({a.guestFrames(),voiceLo,sends,out[1],n==3?out[2]:0u,out[0]});}
        a.receivePCM(&a,pcm,1);
    }
    CHECK(seen.size()==7); // bank, initial program, first note, later program, sustain, second note, first off
    CHECK(seen[2].at==110&&seen[2].status==0x93&&seen[2].pitch==60&&seen[2].voice==64&&seen[2].sends==0);
    CHECK(seen[3].at==150&&seen[3].status==0xc0&&seen[3].pitch==17);
    CHECK(seen[4].status==0xb3&&seen[4].pitch==64&&seen[4].vel==127);
    CHECK(seen[5].status==0x93&&seen[5].pitch==63&&seen[5].voice==255&&seen[5].sends==128);
    CHECK(seen[6].status==0x83&&seen[6].pitch==60);
    // Flush points and next block, same absolute time, no stale setup.
    p->begin(0);CHECK(p->parameter(0,1100,.5));CHECK(p->build(64,s,*b,allowed));CHECK(b->submit(t,*q));p->committed(s);
    p->begin(64);CHECK(p->note(0,3,64,1.,true));CHECK(p->build(64,s,*b,allowed));CHECK(b->submit(t,*q));
    CHECK(a.pullMidi(&a,out,11)==0&&voiceLo==128);CHECK(a.pullMidi(&a,out,11)==3&&out[1]==64);
    // Explicit bad inputs and overcapacity never partially emit a block.
    p->begin(64);CHECK(!p->note(64,0,60,1.,true));CHECK(!p->build(128,s,*b,allowed));
    p->begin(64);CHECK(!p->note(0,0,60,std::numeric_limits<double>::quiet_NaN(),true));CHECK(!p->build(128,s,*b,allowed));
    p->begin(64);for(unsigned i=0;i<4096;++i)CHECK(p->note(0,0,60,1.,true));CHECK(!p->note(0,0,60,1.,true));CHECK(!p->build(128,s,*b,allowed));
    // Force state retransmit after dropped batch (initial state not assumed committed).
    p->invalidateCommit();p->begin(64);CHECK(p->build(128,s,*b,allowed));CHECK(b->submit(t,*q));rt::MidiEvent ev{};unsigned commands=0;while(q->popMidi(ev))++commands;CHECK(commands==6);
    std::puts("Host event plan: cross-queue offsets, same-offset setup/CC/note order, sparse program, zero-block flush, master interpolation, NaN and overflow, resync PASS");
}
