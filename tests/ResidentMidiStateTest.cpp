#include "ResidentMidiState.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using S=awe64::guest::ResidentMidiState;
using E=awe64::rt::MidiEvent;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
int main(){
    S s,other;
    auto cc=[&](unsigned ch,unsigned cc,unsigned value){s.remember({999,static_cast<std::uint8_t>(0xb0|ch),static_cast<std::uint8_t>(cc),static_cast<std::uint8_t>(value),3});};
    cc(0,7,30);cc(0,10,40);cc(0,1,80);cc(0,121,0);
    cc(0,1,90);cc(0,11,45);cc(0,64,127);cc(0,120,0);cc(0,123,0);
    cc(15,7,99);cc(15,121,0);cc(15,7,10);cc(15,93,12);
    s.remember({500,0x90,60,100,3});s.remember({501,0x80,60,0,3});
    s.remember({502,0xdf,65,0,2});s.remember({503,0xe0,5,60,3});
    s.remember({504,0xc0,6,0,2});s.remember({505,0xb0,7,127,2});
    s.remember({506,0xb0,7,255,3});
    E overlay{507,0xb0,7,127,3};overlay.command=awe64::rt::GuestCommand::VoiceControls;s.remember(overlay);
    std::vector<E> out;out.reserve(144);
    CHECK(s.replay([&](E e){out.push_back(e);return true;}));
    CHECK(out.size()==10);
    const unsigned expected[][3]={{0xb0,7,30},{0xb0,10,40},{0xb0,121,0},{0xb0,1,90},{0xb0,11,45},{0xe0,5,60},{0xbf,121,0},{0xbf,7,10},{0xbf,93,12},{0xdf,65,0}};
    for(unsigned i=0;i<out.size();++i){CHECK(out[i].status==expected[i][0]&&out[i].data1==expected[i][1]&&out[i].data2==expected[i][2]);CHECK(out[i].absoluteHostSample==0&&out[i].guestFrame==0);}
    CHECK(other.replay([](E){std::abort();return false;}));
    unsigned count=0;CHECK(!s.replay([&](E){return ++count<3;}));CHECK(count==3);
    out.clear();CHECK(s.replay([&](E e){out.push_back(e);return true;}));CHECK(out.size()==10);
    S full;
    for(unsigned ch=0;ch<16;++ch){
        for(unsigned c:{1u,7u,10u,11u,91u,93u,121u})full.remember({1,static_cast<std::uint8_t>(0xb0|ch),static_cast<std::uint8_t>(c),55,3});
        full.remember({1,static_cast<std::uint8_t>(0xd0|ch),100,0,2});
        full.remember({1,static_cast<std::uint8_t>(0xe0|ch),1,64,3});
    }
    count=0;CHECK(full.replay([&](E){++count;return true;}));CHECK(count==144);
    std::puts("RESIDENT_MIDI_PERSISTENT_REPLAY_ORDER_RESET_ISOLATION_FAILURE_PASS");
}
