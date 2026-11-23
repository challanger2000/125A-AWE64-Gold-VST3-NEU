#include "CreativeDosMailbox.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>

#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"Bridge test line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
using namespace awe64;
using namespace awe64::guest;
int main() {
    rt::RealtimeQueues first, second;
    CreativeDosMailbox guest1(first), guest2(second);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase)==1);
    CHECK(guest2.ioRead(CreativeDosMailbox::kBase)==1);
    guest1.ioWrite(CreativeDosMailbox::kAck,CreativeDosMailbox::kAckOk);
    CHECK(guest1.unexpectedAcks()==1);
    const rt::MidiEvent a{0xFEDCBA9876543210ULL,0x90,60,100,3};
    const rt::MidiEvent b{150,0xC3,28,0,2};
    const rt::MidiEvent c{250,0x80,60,0,3};
    CHECK(first.pushMidi(a));
    CHECK(first.pushMidi(b));
    CHECK(second.pushMidi(c));
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase)==3);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+1)==0x90);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+2)==60);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+3)==100);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+4)==3);
    for(unsigned i=0;i<8;i++)
        CHECK(guest1.ioRead(CreativeDosMailbox::kBase+8+i)==
            static_cast<unsigned char>(a.absoluteHostSample>>(i*8)));
    CHECK(guest1.ioRead(0xFFEF)==255);
    CHECK(guest2.ioRead(CreativeDosMailbox::kBase+1)==0x80);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+1)==0x90);
    guest1.ioWrite(CreativeDosMailbox::kAck,0x00); // no silent dequeue
    CHECK(guest1.unexpectedAcks()==2);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+1)==0x90);
    guest1.ioWrite(CreativeDosMailbox::kAck,CreativeDosMailbox::kAckOk);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+1)==0xC3);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase+4)==2);
    guest1.ioWrite(CreativeDosMailbox::kAck,CreativeDosMailbox::kAckError);
    CHECK(guest1.ioRead(CreativeDosMailbox::kBase)==1);
    CHECK(guest1.processed()==2 && guest1.failed()==1);
    CHECK(guest2.processed()==0 && guest2.failed()==0);
    guest2.ioWrite(CreativeDosMailbox::kAck,CreativeDosMailbox::kAckOk);
    CHECK(guest2.processed()==1);
    std::puts("Original Creative DOS mailbox protocol tests passed");
}
