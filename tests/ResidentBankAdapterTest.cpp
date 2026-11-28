#include "DosBoxHostAdapter.hpp"
#include <cstdio>
#include <cstdlib>
using M=awe64::guest::ResidentBankMailbox;
using A=awe64::integration::DosBoxHostAdapter;
using Q=awe64::rt::RealtimeQueues;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
    auto q=std::make_unique<Q>();
    M m;A a(*q,nullptr,false,nullptr,nullptr,{},false,false,&m);
    unsigned char midi[16]{};
    const std::int16_t oldPcm[2]{1000,-1000},newPcm[2]{2000,-2000};
    auto write=[&](unsigned char v){A::writePort(&a,M::ackPort,v);};
    auto read=[&]{return A::readPort(&a,M::commandPort);};
    auto pull=[&]{return A::pullMidi(&a,midi,sizeof midi);};
    write(M::bankA);CHECK(m.transportOpen());
    for(unsigned i=0;i<4;++i){
        const auto prior=m.current(),slot=prior==M::bankA?M::bankB:M::bankA;
        a.startReplacementCapture();a.startLegacyMidi();
        CHECK(q->pushMidi({0,0x90,60,100,3}));
        CHECK(pull()==3&&midi[0]==0x90&&midi[1]==60);
        CHECK(q->pushMidi({0,0x90,61,100,3}));
        A::receivePCM(&a,oldPcm,1);CHECK(q->snapshot().audioFrames==1);
        CHECK(m.begin(slot));
        CHECK(pull()==0);CHECK(read()==0);CHECK(read()==0);
        bool released=false;
        for(unsigned j=0;j<128&&q->midiRecoveryPending();++j){
            const auto n=pull();if(n){
                CHECK((midi[0]&0xf0)!=0x90);
                if((midi[0]&0xf0)==0x80){CHECK(midi[1]==60);released=true;}
            }
        }
        CHECK(released&&!q->midiRecoveryPending()&&q->snapshot().midiEvents==0);
        CHECK(read()==slot&&read()==0);write(M::busy);
        CHECK(pull()==0);A::receivePCM(&a,newPcm,1);
        CHECK(q->snapshot().audioFrames==1);
        const auto result=i==1?M::rejected:i==2?M::restored:slot;
        write(result);CHECK(!m.transportOpen()&&m.current()==prior);
        q->discardAudio();a.resetResidentTransport();CHECK(m.finish());
        CHECK(m.current()==((i==1||i==2)?prior:slot));
        A::receivePCM(&a,newPcm,1);CHECK(q->snapshot().audioFrames==0);
        CHECK(q->pushMidi({0,0x90,62,100,3}));CHECK(pull()==0);
        a.startReplacementCapture();a.startLegacyMidi();
        A::receivePCM(&a,newPcm,1);
        float l=0,r=0;CHECK(q->readAudio(&l,&r,1)==1&&l==2000.f/32768.f&&r==-2000.f/32768.f);
        CHECK(pull()==3&&midi[1]==62);
        CHECK(q->pushMidi({0,0x80,62,0,3}));CHECK(pull()==3);
    }
    std::puts("RESIDENT_ADAPTER_MIDI_DRAIN_PCM_GATE_ROLLBACK_PASS");
}
