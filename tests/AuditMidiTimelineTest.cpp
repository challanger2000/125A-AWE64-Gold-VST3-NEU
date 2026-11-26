#include "MidiTimeline.hpp"
#include "DosBoxHostAdapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>
#include <vector>
#include <limits>
using namespace awe64;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
    // Clock mapping at 44.1/48/96/192k: independent integer oracle;
    // exercise nonuniform block sizes and a large timeline.
    for(unsigned rate: {44100u,48000u,88200u,96000u,192000u}){
        rt::MidiTimeline timeline;
        CHECK(timeline.configure(rate));
        CHECK(timeline.anchor(1234,600,128));
        CHECK(!timeline.anchor(1234,600,128));
        unsigned long long pos=0;
        for(unsigned i=0;i<10000;++i){
            const unsigned block=1+(i*251u)%4096;
            for(auto off:{0u,block/2,block-1}){
                rt::MidiEvent event{1234+pos+off,0x90,60,100,3};
                CHECK(timeline.stamp(event));
                CHECK(event.guestFrame==728+((pos+off)*44100+rate-1)/rate);
            }
            pos+=block;
        }
        rt::MidiEvent far{1234+100000000000000ull,0x90,60,100,3};
        CHECK(timeline.stamp(far));
        CHECK(far.guestFrame==728+(100000000000000ull*44100ull/rate)+((100000000000000ull*44100ull)%rate!=0));
        timeline.reset();CHECK(!timeline.stamp(far));
        std::printf("RATE %u: variable block / exact rational mapping PASS\n",rate);
    }
    auto q=std::make_unique<rt::RealtimeQueues>();
    integration::DosBoxHostAdapter adapter(*q,nullptr,true);
    rt::MidiTimeline clock;
    CHECK(clock.configure(48000));CHECK(clock.anchor(1000,0,10));
    rt::MidiBlockBatch batch;
    // Host queues intentionally arrive out of order. For equal offsets,
    // setup/controller precede notes; NoteOff/NoteOn remain in input order.
    CHECK(batch.add({1480,0xb0,64,0,3},rt::MidiPhase::Controller));
    CHECK(batch.add({1000,0x80,60,0,3},rt::MidiPhase::Note));
    CHECK(batch.add({1000,0x90,60,100,3},rt::MidiPhase::Note));
    CHECK(batch.add({1000,0xc0,17,0,2},rt::MidiPhase::Setup));
    CHECK(batch.add({1000,0xb0,64,127,3},rt::MidiPhase::Controller));
    CHECK(batch.add({1240,0xe0,0,64,3},rt::MidiPhase::Controller));
    CHECK(batch.submit(clock,*q));
    unsigned char out[11]{};
    auto pull=[&](){return adapter.pullMidi(&adapter,out,11);};
    auto advance=[&](unsigned frames){
        std::vector<std::int16_t> pcm(frames*2);
        adapter.receivePCM(&adapter,pcm.data(),frames);
    };
    CHECK(pull()==0);advance(9);CHECK(pull()==0);advance(1);
    CHECK(pull()==2&&out[0]==0xc0&&out[1]==17);
    CHECK(pull()==3&&out[0]==0xb0&&out[2]==127);
    CHECK(pull()==3&&out[0]==0x80);CHECK(pull()==3&&out[0]==0x90);
    CHECK(pull()==0);advance(220);CHECK(pull()==0);advance(1);
    CHECK(pull()==3&&out[0]==0xe0);CHECK(pull()==0);
    advance(223); // scheduled target 451; actual frame 454 (3 frames late)
    CHECK(pull()==3&&out[0]==0xb0&&out[2]==0);CHECK(pull()==0);
    CHECK(adapter.lateMidi()==1&&adapter.maxLateFrames()==3);
    std::puts("UART gate: no early delivery; sorted offsets; stable equal-offset phases; late-frame telemetry PASS");
    CHECK(batch.add({511,0xb0,7,127,3},rt::MidiPhase::Controller));
    CHECK(batch.add({0,0x90,60,100,3},rt::MidiPhase::Note));
    unsigned ordered=0;
    CHECK(batch.submitLegacy([&](const rt::MidiEvent& e){
        const bool correct=ordered==0?e.status==0x90:e.status==0xb0;
        ++ordered;return correct;
    }));
    CHECK(ordered==2);
    // Future backlog must not prevent overload recovery. One active note is
    // already delivered. Reject the future queue, explicitly release it.
    for(unsigned i=0;i<4096;++i){rt::MidiEvent event{0,0x90,61,100,3};event.guestFrame=999999999;CHECK(q->pushMidi(event));}
    CHECK(!q->pushMidi({0,0x80,60,0,3}));
    unsigned recovered=0,unexpected=0,packets=0;
    for(unsigned i=0;i<500&&q->midiRecoveryPending();++i){
        auto n=pull();if(!n)continue;++packets;
        if(out[0]==0x80&&out[1]==60)++recovered;
        if((out[0]&0xf0)==0x90)++unexpected;
    }
    CHECK(!q->midiRecoveryPending());CHECK(recovered==1&&unexpected==0&&packets==49);
    CHECK(q->snapshot().midiEvents==0);
    std::puts("Clocked overload: 4096 future packets discarded; active note explicitly released PASS");
    // Invalid/oversized host batches must not leak a partial pre-recovery note.
    for(unsigned i=0;i<4096;++i)CHECK(batch.add({1000,0x90,60,100,3},rt::MidiPhase::Note));
    CHECK(!batch.add({1000,0x80,60,0,3},rt::MidiPhase::Note));
    CHECK(!batch.submit(clock,*q));CHECK(q->snapshot().midiEvents==0&&q->midiRecoveryPending());
    std::puts("Oversized host block: fail closed, no partial admission PASS");
}
