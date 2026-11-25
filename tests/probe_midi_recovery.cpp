#include "DosBoxHostAdapter.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(false)
using namespace awe64;
int main(){
    rt::RealtimeQueues a,b; integration::DosBoxHostAdapter pa(a),pb(b);
    unsigned char bytes[11]{};
    auto pull=[&](){return integration::DosBoxHostAdapter::pullMidi(&pa,bytes,11);};
    // Positive: actual notes on two channels and repeated pitch.
    for(const auto& e:std::array<rt::MidiEvent,5>{{{0,0x90,60,100,3},{0,0x90,60,100,3},{0,0x95,64,100,3},{0,0xb5,64,127,3},{0,0x85,64,0,3}}}){
        CHECK(a.pushMidi(e));CHECK(pull()==3);CHECK(bytes[0]==e.status&&bytes[1]==e.data1);
    }
    CHECK(pull()==0);
    CHECK(b.pushMidi({0,0x92,67,100,3}));
    // Overflow deliberately loses a NoteOff with a sounding pitch.
    for(unsigned i=0;i<4096;++i)CHECK(a.pushMidi({0,0xb0,1,0,3}));
    CHECK(!a.pushMidi({0,0x80,60,0,3}));
    for(unsigned i=0;i<4096;++i){CHECK(pull()==3);CHECK(bytes[0]==0xb0&&bytes[1]==1);}
    // Even though the original FIFO is now empty, new notes must wait
    // for real release packets rather than sneaking behind the lost off.
    CHECK(!a.pushMidi({0,0x90,69,100,3}));
    unsigned releases=0,sustainOff=0,panic=0,packets=0;
    while(pull()){
        CHECK(++packets<2100);
        if(bytes[0]==0x80&&bytes[1]==60)++releases;
        if((bytes[0]&0xf0)==0xb0&&bytes[1]==64&&bytes[2]==0)++sustainOff;
        if((bytes[0]&0xf0)==0xb0&&(bytes[1]==120||bytes[1]==123))++panic;
    }
    CHECK(releases==2&&sustainOff==16&&panic==32);
    CHECK(a.pushMidi({0,0x90,69,100,3}));CHECK(pull()==3&&bytes[0]==0x90&&bytes[1]==69);
    CHECK(integration::DosBoxHostAdapter::pullMidi(&pb,bytes,11)==3&&bytes[0]==0x92&&bytes[1]==67);
    CHECK(integration::DosBoxHostAdapter::pullMidi(&pb,bytes,11)==0);
    std::printf("MIDI_RECOVERY_PASS explicit_note_off=%u sustain_off=%u supplemental_panic=%u drops=%llu independent_instance=PASS new_note=PASS\n",releases,sustainOff,panic,(unsigned long long)a.midiDrops());
}
