#include "DosBoxHostAdapter.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"Fail line %d: %s\n",__LINE__,#x);std::abort();}}while(false)
using namespace awe64;
int main(){
 rt::RealtimeQueues a,b;
 integration::DosBoxHostAdapter pa(a),pb(b);
 CHECK(a.pushMidi({500,0x90,60,127,3}));
 CHECK(integration::DosBoxHostAdapter::readPort(&pa,0xBEE0)==3);
 CHECK(integration::DosBoxHostAdapter::readPort(&pa,0xBEE1)==0x90);
 CHECK(integration::DosBoxHostAdapter::readPort(&pa,0xBEE2)==60);
 CHECK(integration::DosBoxHostAdapter::readPort(&pb,0xBEE0)==1);
 integration::DosBoxHostAdapter::writePort(&pa,0xBEF0,0xA5);
 CHECK(pa.mailbox().processed()==1);
 CHECK(pb.mailbox().processed()==0);
 const std::array<std::int16_t,8> pcm{{0,32767,-32768,1,16384,-16384,70,-200}};
 integration::DosBoxHostAdapter::receivePCM(&pa,pcm.data(),4);
 float l[5]{},r[5]{};
 CHECK(a.readAudio(l,r,5)==4);
 CHECK(l[0]==0.f&&r[0]==32767.f/32768.f);
 CHECK(l[1]==-1.f&&r[1]==1.f/32768.f);
 CHECK(l[2]==.5f&&r[2]==-.5f);
 CHECK(l[4]==0.f&&r[4]==0.f);
 CHECK(a.audioUnderruns()==1);
 CHECK(a.pushMidi({700,0xB1,64,127,3}));
 CHECK(a.pushMidi({701,0xC1,73,0,2}));
 unsigned char packet[3]{};
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,packet,3)==3);
 CHECK(packet[0]==0xB1 && packet[1]==64 && packet[2]==127);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,packet,3)==2);
 CHECK(packet[0]==0xC1 && packet[1]==73);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,packet,3)==0);
 CHECK(integration::DosBoxHostAdapter::pullMidi(nullptr,packet,3)==0);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,packet,2)==0);
 float lb[1]{},rb[1]{};
 CHECK(b.readAudio(lb,rb,1)==0);
 CHECK(integration::DosBoxHostAdapter::readPort(nullptr,0xBEE0)==0xffu);
 integration::DosBoxHostAdapter::receivePCM(nullptr,pcm.data(),4);
}
