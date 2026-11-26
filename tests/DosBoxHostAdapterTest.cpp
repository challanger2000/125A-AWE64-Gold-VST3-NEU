#include "DosBoxHostAdapter.hpp"
#include "MidiTimeline.hpp"
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
 const auto reverb=rt::makeCreativeEffectType(702,false,5);
 CHECK(rt::validCreativeEffectType(reverb));
 CHECK(a.pushMidi(reverb));CHECK(a.pushMidi({703,0x90,60,100,3}));
 unsigned char sysex[11]{};
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,sysex,10)==0);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,sysex,11)==11);
 for(unsigned i=0;i<11;i++)CHECK(sysex[i]==reverb.sysex[i]);
 CHECK(sysex[0]==0xf0&&sysex[7]==0x30&&sysex[8]==5&&sysex[10]==0xf7);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&pa,packet,3)==3&&packet[0]==0x90);
 for(unsigned i=0;i<8;i++){
   auto ev=rt::makeCreativeEffectType(704,true,static_cast<std::uint8_t>(i));
   CHECK(rt::validCreativeEffectType(ev)&&ev.sysex[7]==0x38&&ev.sysex[8]==i);
   ev.sysex[4]^=1;CHECK(!rt::validCreativeEffectType(ev));
 }
 float lb[1]{},rb[1]{};
 CHECK(b.readAudio(lb,rb,1)==0);
 CHECK(integration::DosBoxHostAdapter::readPort(nullptr,0xBEE0)==0xffu);
 integration::DosBoxHostAdapter::receivePCM(nullptr,pcm.data(),4);
 auto gate=std::make_shared<rt::LegacyCaptureGate>();
 auto pausedQueue=std::make_unique<rt::RealtimeQueues>();
 integration::DosBoxHostAdapter paused(*pausedQueue,nullptr,false,nullptr,nullptr,gate);
 gate->suspend();
 for(unsigned i=0;i<10000;++i)integration::DosBoxHostAdapter::receivePCM(&paused,pcm.data(),4);
 CHECK(pausedQueue->snapshot().audioFrames==0&&pausedQueue->audioDrops()==0&&paused.suspendedFrames()==40000);
 pausedQueue->discardAudio();gate->resume();
 integration::DosBoxHostAdapter::receivePCM(&paused,pcm.data(),4);
 CHECK(pausedQueue->readAudio(l,r,4,gate->epoch())==4&&l[2]==.5f&&r[2]==-.5f);
 // Independent candidate gate: same host pause gate, separate PCM rings.
 // The previous engine keeps producing; the replacement cannot accumulate
 // even a full ring of obsolete pre-handover audio.
 auto candidateQueue=std::make_unique<rt::RealtimeQueues>();
 integration::DosBoxHostAdapter candidate(*candidateQueue,nullptr,false,nullptr,nullptr,gate,true);
 for(unsigned i=0;i<10000;++i)integration::DosBoxHostAdapter::receivePCM(&candidate,pcm.data(),4);
 integration::DosBoxHostAdapter::receivePCM(&paused,pcm.data(),4);
 CHECK(pausedQueue->readAudio(l,r,4,gate->epoch())==4);
 CHECK(candidateQueue->snapshot().audioFrames==0&&candidateQueue->audioDrops()==0);
 CHECK(candidate.replacementSuppressedFrames()==40000);
 gate->suspend();candidate.startReplacementCapture();
 integration::DosBoxHostAdapter::receivePCM(&candidate,pcm.data(),4);
 CHECK(candidateQueue->snapshot().audioFrames==0&&candidate.suspendedFrames()==4);
 gate->resume();
 integration::DosBoxHostAdapter::receivePCM(&candidate,pcm.data(),4);
 CHECK(candidateQueue->readAudio(l,r,4,gate->epoch())==4&&l[2]==.5f&&r[2]==-.5f);
 // Demand-clock mode must not be gated by legacy candidate suppression.
 auto clockQueue=std::make_unique<rt::RealtimeQueues>();
 integration::DosBoxHostAdapter clocked(*clockQueue,nullptr,true,nullptr,nullptr,gate,true);
 integration::DosBoxHostAdapter::receivePCM(&clocked,pcm.data(),4);
 CHECK(clocked.guestFrames()==4&&clockQueue->readAudio(l,r,4)==4);
 // Early packet stays in its original FIFO until the host explicitly
 // arms the primed PCM transport. No retransmit or fabricated note.
 auto startupQueue=std::make_unique<rt::RealtimeQueues>();
 integration::DosBoxHostAdapter startup(*startupQueue,nullptr,false,nullptr,nullptr,gate,false,true);
 CHECK(startupQueue->pushMidi({0,0xb0,0,0,3}));
 CHECK(startupQueue->pushMidi({0,0xc0,0,0,2}));
 CHECK(startupQueue->pushMidi({0,0x90,60,100,3}));
 for(unsigned n=0;n<1000;++n)CHECK(integration::DosBoxHostAdapter::pullMidi(&startup,packet,3)==0);
 CHECK(startupQueue->snapshot().midiEvents==3&&startup.pulledNoteOns()==0);
 integration::DosBoxHostAdapter::receivePCM(&startup,pcm.data(),4);
 CHECK(startupQueue->snapshot().audioFrames==4);
 startup.startLegacyMidi();
 CHECK(integration::DosBoxHostAdapter::pullMidi(&startup,packet,3)==3&&packet[0]==0xb0);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&startup,packet,3)==2&&packet[0]==0xc0);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&startup,packet,3)==3&&packet[0]==0x90&&packet[1]==60&&packet[2]==100);
 CHECK(integration::DosBoxHostAdapter::pullMidi(&startup,packet,3)==0&&startup.pulledNoteOns()==1);
 // Guest-only consumer delivers both halves in a single command before
 // the following NoteOn, and the next generation after that note.
 static std::uint64_t observedLo=0;static unsigned observedHi=0;
 auto commands=std::make_unique<rt::RealtimeQueues>();
 integration::DosBoxHostAdapter consumer(*commands,nullptr,false,
     +[](std::uint64_t lo,unsigned hi){observedLo=lo;observedHi=hi;});
 CHECK(commands->pushMidi(rt::makeVoiceControlCommand(0,0xffffffffffffffffull,0xffff)));
 CHECK(commands->pushMidi({0,0x90,60,100,3}));
 CHECK(commands->pushMidi(rt::makeVoiceControlCommand(0,0,0)));
 CHECK(observedLo==0&&observedHi==0);
 CHECK(consumer.pullMidi(&consumer,packet,3)==0&&observedLo==0xffffffffffffffffull&&observedHi==0xffff);
 CHECK(consumer.pullMidi(&consumer,packet,3)==3&&packet[0]==0x90);
 CHECK(consumer.pullMidi(&consumer,packet,3)==0&&observedLo==0&&observedHi==0);
}
