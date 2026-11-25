#include "MidiTimeline.hpp"
#include "DosBoxHostAdapter.hpp"
#include <cstdio>
#include <memory>
#include <vector>
using namespace awe64;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(false)
static std::uint64_t voiceLo=0;static unsigned voiceHi=0,sends=0;
static void voice(std::uint64_t l,unsigned h){voiceLo=l;voiceHi=h;}
static void send(unsigned s){sends=s;}
int main(){
    auto q=std::make_unique<rt::RealtimeQueues>();
    integration::DosBoxHostAdapter a(*q,nullptr,true,voice,send);
    rt::MidiTimeline t;CHECK(t.configure(44100));CHECK(t.anchor(0,0,0));
    rt::MidiBlockBatch b;
    CHECK(b.add({20,0x90,61,100,3},rt::MidiPhase::Note));
    CHECK(b.add(rt::makeVoiceControlCommand(20,0xabcdef,0x1234),rt::MidiPhase::Setup));
    CHECK(b.add(rt::makeVoiceControlCommand(10,0x123456,0xabcd),rt::MidiPhase::Setup));
    CHECK(b.add({10,0x90,60,100,3},rt::MidiPhase::Note));
    CHECK(b.add(rt::makeAbsoluteSendCommand(20,0xabcd),rt::MidiPhase::Setup));
    CHECK(b.submit(t,*q));
    unsigned char out[11]{};auto pull=[&]{return a.pullMidi(&a,out,11);};
    auto advance=[&](unsigned n){std::vector<std::int16_t> x(n*2);a.receivePCM(&a,x.data(),n);};
    CHECK(pull()==0&&voiceLo==0&&sends==0);
    advance(10);CHECK(pull()==0&&voiceLo==0x123456&&voiceHi==0xabcd);
    CHECK(pull()==3&&out[1]==60&&voiceLo==0x123456&&sends==0);
    CHECK(pull()==0&&voiceLo==0x123456);
    advance(10);CHECK(pull()==0&&voiceLo==0xabcdef&&voiceHi==0x1234);
    CHECK(pull()==0&&sends==0xabcd);CHECK(pull()==3&&out[1]==61&&voiceLo==0xabcdef);
    CHECK(pull()==0);CHECK(a.lateMidi()==0);
    CHECK(q->midiRecoveryEpoch()==0);q->requestMidiRecovery();
    for(unsigned n=0;n<100&&q->midiRecoveryPending();++n)(void)pull();
    CHECK(!q->midiRecoveryPending()&&q->midiRecoveryEpoch()==1);
    std::puts("Timestamped control commands: earlier note sees earlier overlay, later note sees later overlay/sends; recovery epoch PASS (mock UART consumer, not native guest)");
}
