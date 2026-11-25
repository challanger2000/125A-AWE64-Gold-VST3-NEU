// Mock VST3 host types and actual portable runtime bridge. Compiles the exact
// proposed processTimed() member body, not a real Steinberg/Windows VST3.
#include "HostDemandSession.hpp"
#include "HostRateConverter.hpp"
#include "DosBoxHostAdapter.hpp"
#include <memory>
#include <vector>
#include <cstdio>
using int32=int;using tresult=int;using ParamValue=double;using TBool=bool;
#define PLUGIN_API
constexpr int kResultOk=0,kResultFalse=1,kOffline=2;
struct Point {int offset;double value;};
struct Queue {unsigned id;std::vector<Point> points;
    int getPointCount()const{return points.size();}unsigned getParameterId()const{return id;}
    int getPoint(int n,int& offset,double& value){offset=points[n].offset;value=points[n].value;return kResultOk;}};
struct Changes {std::vector<Queue> q;int getParameterCount()const{return q.size();}Queue* getParameterData(int i){return &q[i];}};
struct Note {int channel,pitch;double velocity;};
struct Event {enum {kNoteOnEvent,kNoteOffEvent};int type=0,busIndex=0,sampleOffset=0;Note noteOn{},noteOff{};};
struct Events {std::vector<Event> e;int getEventCount()const{return e.size();}int getEvent(int i,Event& out){out=e[i];return kResultOk;}};
struct Output {int numChannels=2;float** channelBuffers32=nullptr;unsigned silenceFlags=0;};
struct ProcessData {Output* outputs=nullptr;int numOutputs=1,numSamples=0,processMode=0;Events* inputEvents=nullptr;Changes* inputParameterChanges=nullptr;};
namespace awe64::integration {
struct DosBoxRuntimeHost {
    rt::RealtimeQueues queue;DosBoxHostAdapter adapter{queue,nullptr,true};bool boot=true,dead=false,forceShort=false,allowOffline=true;bool lastOffline=false;
    std::uint64_t submitted=0,published=0,horizon=0;unsigned before=0,after=0,requests=0;
    bool ready(std::size_t)const{return boot;}bool finished()const{return dead;}
    bool submitTimed(rt::MidiBlockBatch& b,const rt::MidiTimeline& t) noexcept{++submitted;return b.submit(t,queue);}
    bool publishDemand(std::uint64_t n) noexcept{published=submitted;horizon=n;return adapter.publishDemand(n);}
    auto midiRecoveryEpoch()const noexcept{return queue.midiRecoveryEpoch();}void requestMidiRecovery()noexcept{++requests;queue.requestMidiRecovery();}
    bool waitForAudio(std::size_t n,bool offline) noexcept{lastOffline=offline;if(queue.snapshot().audioFrames>=n)return true;if(!offline||!allowOffline)return false;while(queue.snapshot().audioFrames<n)queue.pushAudio({.5f,-.5f});return true;}
    std::size_t readAudio(float* l,float* r,std::size_t n) noexcept{if(forceShort)return 0;return queue.readAudio(l,r,n);}
    void measureQueueBeforeBlock(){++before;}void measureQueueAfterBlock(){++after;}
};
}
namespace awe64 {
struct Catalog {const void* Find(unsigned bank,std::uint8_t p)const noexcept{return bank==0&&(p==0||p==17)?this:nullptr;}};
class DosBoxVst3Shell {public:
    rt::HostDemandSession hostDemand_;rt::HostRateConverter rateConverter_;Catalog startedCatalog_;
    std::atomic<bool> timedProcessing_{true},demandActive_{true};std::atomic<int> guestStatus_{0};
    std::atomic<std::uint8_t> program_{0},reverbType_{4},chorusType_{2},reverbSend_{0},chorusSend_{0};
    std::array<std::atomic<std::uint8_t>,10> voiceControls_{};std::atomic<float> master_{1.f};
    tresult setProcessing(TBool);
    tresult processTimed(ProcessData&,integration::DosBoxRuntimeHost&);
};
#include "ProcessTimed.inc"
#include "SetProcessing.inc"
}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
int main(){using namespace awe64;using namespace awe64::integration;
    auto shell=std::make_unique<DosBoxVst3Shell>();auto e=std::make_unique<DosBoxRuntimeHost>();
    CHECK(shell->hostDemand_.configure(44100,128));CHECK(shell->hostDemand_.activate());CHECK(shell->rateConverter_.configure(44100));
    float l[128]{},r[128]{};float* channels[]={l,r};Output out{2,channels,0};ProcessData d{&out,1,128};
    Changes params{{{1100,{{64,1.},{0,.25}}},{1000,{{64,17./127.}}},{1001,{{0,0.},{127,1.}}}}};
    Events events{{{Event::kNoteOnEvent,0,64,{0,62,1.},{}},{Event::kNoteOnEvent,0,0,{1,60,1.},{}}}};
    d.inputParameterChanges=&params;d.inputEvents=&events;
    for(unsigned i=0;i<128;++i)CHECK(e->queue.pushAudio({.5f,-.5f}));
    CHECK(shell->processTimed(d,*e)==kResultOk);CHECK(shell->hostDemand_.position()==128);CHECK(e->submitted==e->published);CHECK(shell->program_==17&&shell->voiceControls_[0]==255);
    for(unsigned i=0;i<128;++i)CHECK(std::abs(l[i]-.5f*i/127)<1e-6&&r[i]==-l[i]);
    CHECK(out.silenceFlags==0&&e->before==1&&e->after==1);
    rt::MidiEvent m{};unsigned notes=0;std::uint64_t last=0;while(e->queue.popMidi(m)){CHECK(m.guestFrame>=last);last=m.guestFrame;if(m.status==0x90||m.status==0x91)++notes;}CHECK(notes==2);
    // Parameter-only flush with no output; no virtual time advancement.
    d.numSamples=0;d.outputs=nullptr;d.numOutputs=0;d.inputEvents=nullptr;params.q={{1001,{{0,.5}}}};
    CHECK(shell->processTimed(d,*e)==kResultOk);CHECK(shell->master_==.5f&&shell->hostDemand_.position()==128);
    // Exact setProcessing body: pausing may occur on the RT thread and
    // may be repeated with no intervening callback. Preserve pending PCM,
    // MIDI and converter phase; first resumed block must not lose its notes.
    rt::MidiEvent pending{};CHECK(e->queue.peekMidi(pending)==false);
    for(unsigned i=0;i<128;++i)CHECK(e->queue.pushAudio({.25f,-.25f}));
    const auto horizon=e->horizon;
    CHECK(shell->setProcessing(false)==kResultOk);CHECK(shell->setProcessing(false)==kResultOk);
    CHECK(!shell->timedProcessing_&&shell->hostDemand_.position()==128&&e->horizon==horizon);
    CHECK(e->queue.snapshot().audioFrames==128);
    d.inputParameterChanges=nullptr;d.outputs=&out;d.numOutputs=1;d.numSamples=128;d.processMode=0;
    CHECK(shell->processTimed(d,*e)==kResultFalse);CHECK(!shell->hostDemand_.faulted());
    CHECK(shell->hostDemand_.position()==128&&e->queue.snapshot().audioFrames==128&&e->horizon==horizon);
    CHECK(shell->setProcessing(true)==kResultOk);CHECK(shell->setProcessing(true)==kResultOk);
    events.e={{Event::kNoteOnEvent,0,37,{0,65,1.},{}}};d.inputEvents=&events;
    CHECK(shell->processTimed(d,*e)==kResultOk);CHECK(shell->hostDemand_.position()==256);
    CHECK(l[0]==.125f&&l[127]==.125f);CHECK(e->queue.popMidi(pending));
    CHECK(pending.status==0x90&&pending.data1==65&&pending.guestFrame==shell->hostDemand_.lead()+128+37);
    CHECK(!e->queue.popMidi(pending));
    d.inputEvents=nullptr;
    // Offline consumer can request a bounded wait; RT does not wait and
    // returns sticky error instead of consuming fake PCM on underrun.
    d.inputParameterChanges=nullptr;d.outputs=&out;d.numOutputs=1;d.numSamples=128;d.processMode=kOffline;
    CHECK(shell->processTimed(d,*e)==kResultOk&&e->lastOffline);CHECK(l[0]==.25f&&l[127]==.25f);
    d.processMode=0;CHECK(shell->processTimed(d,*e)==kResultFalse&&!e->lastOffline);CHECK(out.silenceFlags==3&&shell->hostDemand_.faulted());
    CHECK(std::all_of(l,l+128,[](float f){return f==0.f;}));CHECK(shell->processTimed(d,*e)==kResultFalse);
    // Unready guest cannot silently acknowledge actual MIDI input.
    auto boot=std::make_unique<DosBoxVst3Shell>();auto stalled=std::make_unique<DosBoxRuntimeHost>();stalled->boot=false;
    CHECK(boot->hostDemand_.configure(44100,128)&&boot->hostDemand_.activate());d.inputEvents=nullptr;
    CHECK(boot->processTimed(d,*stalled)==kResultOk&&boot->hostDemand_.position()==0);
    d.inputEvents=&events;CHECK(boot->processTimed(d,*stalled)==kResultFalse&&boot->hostDemand_.faulted());
    std::puts("Exact candidate processTimed body: mock host offset routing, master PCM, flush, repeated pause/resume, first resumed note and PCM, offline flag, RT underrun, early-input rejection PASS; native Windows NOT RUN");
}
