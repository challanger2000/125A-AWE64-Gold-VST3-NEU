// Audit-only native host: actual VST3 and original DOSBox, synthetic DOS fixtures only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <ole2.h>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "public.sdk/source/common/memorystream.h"
#include <vector>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <thread>
#include <chrono>
#include "pluginterfaces/vst/ivstevents.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
namespace S=Steinberg;
namespace V=Steinberg::Vst;
class MinimalHostApplication final : public V::IHostApplication {
    std::atomic<S::uint32> refs_{1};
public:
    S::tresult PLUGIN_API queryInterface(const S::TUID iid, void** obj) override {
        if(!obj)return S::kInvalidArgument;
        *obj=nullptr;
        if(S::FUnknownPrivate::iidEqual(iid,V::IHostApplication::iid)||
           S::FUnknownPrivate::iidEqual(iid,S::FUnknown::iid)) {
            addRef();
            *obj=static_cast<V::IHostApplication*>(this);
            return S::kResultOk;
        }
        return S::kNoInterface;
    }
    S::uint32 PLUGIN_API addRef() override {return ++refs_;}
    S::uint32 PLUGIN_API release() override {return --refs_;}
    S::tresult PLUGIN_API getName(V::String128 name) override {
        if(!name)return S::kInvalidArgument;
        constexpr char text[]="125A CI Synthetic VST3 Host";
        for(unsigned i=0;i<sizeof(text);++i)
            name[i]=static_cast<V::TChar>(text[i]);
        return S::kResultOk;
    }
    S::tresult PLUGIN_API createInstance(S::TUID,S::TUID,void** obj) override {
        if(obj)*obj=nullptr;
        return S::kNoInterface;
    }
};


class MidiNoteHostEvents final : public V::IEventList {
    std::atomic<S::uint32> refs_{1};
    V::Event note_{};
    bool enabled_{false};
    bool noteOff_{false};
    unsigned count_=1, firstPitch_=60;
    float velocity_=100.f/127.f;
public:
    MidiNoteHostEvents() {
        note_.busIndex=0;
        note_.sampleOffset=0;
        note_.type=V::Event::kNoteOnEvent;
        note_.noteOn.channel=0;
        note_.noteOn.pitch=60;
        note_.noteOn.velocity=100.f/127.f;
        note_.noteOn.tuning=0.f;
        note_.noteOn.length=0;
        note_.noteOn.noteId=-1;
    }
    void setChord(unsigned count,unsigned first,float velocity) noexcept {
        count_=count;firstPitch_=first;velocity_=velocity;
    }
    void setOffset(unsigned offset) noexcept {note_.sampleOffset=static_cast<S::int32>(offset);}
    void setEnabled(bool enabled,bool noteOff=false) noexcept {
        enabled_=enabled;
        noteOff_=noteOff;
        if(!enabled)return;
        // Actual VST3 NoteOn/NoteOff union payload; no private MIDI hook.
        note_.type=noteOff_ ? V::Event::kNoteOffEvent : V::Event::kNoteOnEvent;
        if(noteOff_) {
            note_.noteOff.channel=0;
            note_.noteOff.pitch=60;
            note_.noteOff.velocity=0.f;
            note_.noteOff.noteId=-1;
            note_.noteOff.tuning=0.f;
        }else{
            note_.noteOn.channel=0;
            note_.noteOn.pitch=60;
            note_.noteOn.velocity=100.f/127.f;
            note_.noteOn.tuning=0.f;
            note_.noteOn.length=0;
            note_.noteOn.noteId=-1;
        }
    }
    S::tresult PLUGIN_API queryInterface(const S::TUID iid,void** obj) override {
        if(!obj)return S::kInvalidArgument;
        *obj=nullptr;
        if(S::FUnknownPrivate::iidEqual(iid,V::IEventList::iid) ||
           S::FUnknownPrivate::iidEqual(iid,S::FUnknown::iid)) {
            *obj=static_cast<V::IEventList*>(this);
            addRef();
            return S::kResultOk;
        }
        return S::kNoInterface;
    }
    S::uint32 PLUGIN_API addRef() override {return ++refs_;}
    S::uint32 PLUGIN_API release() override {return --refs_;}
    S::int32 PLUGIN_API getEventCount() override {return enabled_?static_cast<S::int32>(count_):0;}
    S::tresult PLUGIN_API getEvent(S::int32 index,V::Event& event) override {
        if(index<0||static_cast<unsigned>(index)>=count_||!enabled_)return S::kResultFalse;
        event=note_;
        if(noteOff_)event.noteOff.pitch=static_cast<S::int16>(firstPitch_+index);
        else {event.noteOn.pitch=static_cast<S::int16>(firstPitch_+index);event.noteOn.velocity=velocity_;}
        return S::kResultOk;
    }
    S::tresult PLUGIN_API addEvent(V::Event&) override {
        return S::kNotImplemented;
    }
};

static int fail(const std::string& m){std::cerr<<"AUDIT_INFRA_FAILURE "<<m<<std::endl;return 2;}
int main(int argc,char** argv){
    std::cout<<std::unitbuf;std::cerr<<std::unitbuf;
    if(argc!=4)return fail("usage: host plugin mode bank.sf2");
    const std::string mode=argv[2], bank=argv[3];
    if(mode!="early"&&mode!="pause"&&mode!="rollback"&&mode!="switch"&&mode!="automation")return fail("unknown mode");
        // Match the SDK's actual Windows hosting module: OLE is initialized
    // on the host UI thread before loading/initializing the plug-in.
    struct OleScope {
        HRESULT status=OleInitialize(nullptr);
        ~OleScope(){if(SUCCEEDED(status))OleUninitialize();}
    } ole;
    if(FAILED(ole.status))return fail("Host OLE initialization failed");
    std::cout<<"HOST_OLE_READY\n";
    std::string filename=argv[1];
    HMODULE module=LoadLibraryExA(filename.c_str(),nullptr,
                                  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|
                                  LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!module)return fail("Windows LoadLibraryEx could not load actual VST3 binary "+
                           std::to_string(GetLastError()));
    using ModuleLifecycle=bool(*)();
    auto initDll=reinterpret_cast<ModuleLifecycle>(GetProcAddress(module,"InitDll"));
    auto exitDll=reinterpret_cast<ModuleLifecycle>(GetProcAddress(module,"ExitDll"));
    std::cout<<"HOST_MODULE_INIT_ENTER\n";
    if(!initDll||!exitDll||!initDll())return fail("Steinberg Windows module initialization failed");
    std::cout<<"HOST_MODULE_INIT_OK\n";
    using GetFactory=S::IPluginFactory* (PLUGIN_API*)();
    auto getFactory=reinterpret_cast<GetFactory>(GetProcAddress(module,"GetPluginFactory"));
    if(!getFactory)return fail("Real Steinberg GetPluginFactory export missing");
    S::IPluginFactory* factory=getFactory();
    if(!factory)return fail("Real VST3 factory returned null");

    S::PClassInfo info{};
    bool found=false;
    for(S::int32 i=0;i<factory->countClasses();++i){
        if(factory->getClassInfo(i,&info)!=S::kResultOk)continue;
        if(std::strcmp(info.category,kVstAudioEffectClass)==0) {
            found=true;break;
        }
    }
    if(!found)return fail("No real VST3 audio component registered");
    V::IComponent* component=nullptr;
    if(factory->createInstance(info.cid,V::IComponent::iid,
                               reinterpret_cast<void**>(&component))!=S::kResultOk||
       !component)return fail("VST3 IComponent creation failed");
    V::IAudioProcessor* processor=nullptr;
    if(component->queryInterface(V::IAudioProcessor::iid,
            reinterpret_cast<void**>(&processor))!=S::kResultOk||
       !processor)return fail("Real component has no VST3 audio processor");
    MinimalHostApplication host;
    if(component->initialize(&host)!=S::kResultOk)
        return fail("VST3 initialize() failed");
    
    V::IEditController* controller=nullptr;
    if(component->queryInterface(V::IEditController::iid,reinterpret_cast<void**>(&controller))!=S::kResultOk||!controller)
        return fail("controller unavailable");

    using Clock=std::chrono::steady_clock;
    using namespace std::chrono;
    if(component->activateBus(V::kAudio,V::kOutput,0,true)!=S::kResultOk||
       component->activateBus(V::kEvent,V::kInput,0,true)!=S::kResultOk)return fail("activate buses");
    V::ProcessSetup config{};
    config.processMode=V::kRealtime;config.symbolicSampleSize=V::kSample32;config.sampleRate=44100;config.maxSamplesPerBlock=512;
    if(processor->setupProcessing(config)!=S::kResultOk)return fail("setupProcessing");
    if(component->setActive(true)!=S::kResultOk||processor->setProcessing(true)!=S::kResultOk)return fail("activate runtime");
    std::array<float,512> left{},right{};float* channels[]={left.data(),right.data()};
    V::AudioBusBuffers output{};output.numChannels=2;output.channelBuffers32=channels;
    V::ProcessData data{};data.processMode=V::kRealtime;data.symbolicSampleSize=V::kSample32;
    data.numSamples=512;data.numOutputs=1;data.outputs=&output;
    MidiNoteHostEvents event;data.inputEvents=&event;
    unsigned unexpected=0,nonfinite=0;
    auto block=[&](bool note=false,V::IParameterChanges* changes=nullptr){
        event.setEnabled(note);data.inputParameterChanges=changes;
        left.fill(123.f);right.fill(123.f);output.silenceFlags=0;
        const auto result=processor->process(data);if(result!=S::kResultOk)++unexpected;
        float peak=0;
        for(unsigned k=0;k<512;++k){if(!std::isfinite(left[k])||!std::isfinite(right[k])||std::abs(left[k])>1.01f||std::abs(right[k])>1.01f)++nonfinite;peak=std::max({peak,std::abs(left[k]),std::abs(right[k])});}
        return peak;
    };
    auto pump=[&](unsigned n,bool note=false){
        float peak=0;const auto start=Clock::now();
        for(unsigned k=0;k<n;++k){peak=std::max(peak,block(note&&k==0));std::this_thread::sleep_until(start+duration_cast<Clock::duration>(duration<double>((k+1)*512./44100.)));}
        return peak;
    };
    unsigned errors=0;
    auto verdict=[&](const char* id,bool pass){std::cout<<id<<" "<<(pass?"PASS":"FAIL")<<std::endl;errors+=!pass;};
    if(mode=="early"){
        // Exactly one note, in the first process callback after activation.
        const auto first=block(true);
        const auto early=std::max(first,pump(220));
        const auto late= pump(70,true);
        std::cout<<"EARLY_NOTE_PEAK="<<early<<" LATE_NOTE_PEAK="<<late<<" PROCESS_ERRORS="<<unexpected<<std::endl;
        if(early<.01f&&late<.01f)return fail("neither early nor late note reached synthetic guest");
        verdict("AUDIT_A09_EARLY_NOTE",early>=.01f||unexpected!=0);
    }else{
        (void)pump(180); // boot warm-up, independently measure post-boot idle
        const float idle=pump(20);
        const auto peak=pump(45,true);
        std::cout<<"POSITIVE_AUDIO_PEAK="<<peak<<" IDLE_PEAK="<<idle<<std::endl;
        if(peak<.01f||idle>.01f||unexpected||nonfinite)return fail("synthetic MIDI/PCM positive control");
        if(mode=="pause"){
            if(processor->setProcessing(false)!=S::kResultOk)return fail("pause");
            std::this_thread::sleep_for(milliseconds(1200));
            if(processor->setProcessing(true)!=S::kResultOk)return fail("resume");
            const auto resumed=pump(20);
            std::cout<<"PAUSE_RESUME_PEAK="<<resumed<<" (queue counters emitted on deactivate)"<<std::endl;
        }else if(mode=="automation"){
            // Existing sustained speaker signal; master must be zero for
            // samples 0..255 and unity for 256..511 (same block).
            V::ParameterChanges changes(1);S::int32 index=0,point=0;
            auto* q=changes.addParameterData(1001u,index);
            if(!q||q->addPoint(0,0.,point)!=S::kResultOk||q->addPoint(255,0.,point)!=S::kResultOk||
               q->addPoint(256,1.,point)!=S::kResultOk||q->addPoint(511,1.,point)!=S::kResultOk)return fail("automation fixture");
            (void)block(false,&changes);
            auto maxabs=[&](unsigned first,unsigned last){float p=0;for(unsigned k=first;k<last;++k)p=std::max({p,std::abs(left[k]),std::abs(right[k])});return p;};
            const auto first=maxabs(0,256),second=maxabs(256,512);
            std::cout<<"MASTER_FIRST_HALF="<<first<<" MASTER_SECOND_HALF="<<second<<std::endl;
            if(second<.01f)return fail("unity second-half control silent");
            verdict("AUDIT_A01_MASTER_SAMPLE_OFFSETS",first==0.f);
        }else{
            // Real active bank switch with a valid synthetic SF2. The old
            // running guest has a private copy of the installed runtime DLL.
            // Remove only this disposable runner's installed DLL, so both
            // new activation and fallback activation fail synchronously.
            auto state=[](const std::string& path){
                const auto n=static_cast<std::uint32_t>(path.size());
                std::vector<std::uint8_t> b{'D','B','X','D',2,0,0,0x10,0x27,0x5a,0xa5,
                    std::uint8_t(n),std::uint8_t(n>>8),std::uint8_t(n>>16),std::uint8_t(n>>24)};
                b.insert(b.end(),path.begin(),path.end());return b;
            };
            auto bytes=state(bank);
            S::MemoryStream prior;if(component->getState(&prior)!=S::kResultOk)return fail("save before rollback");
            auto dll=std::filesystem::path(argv[1]).parent_path()/"AWE64DOSBoxRuntime.dll";
            auto moved=dll;moved+=".audit-hidden";
            std::error_code ec;
            if(mode=="rollback"){std::filesystem::rename(dll,moved,ec);if(ec)return fail("rename installed runtime: "+ec.message());}
            S::MemoryStream incoming(bytes.data(),bytes.size());
            const auto result=component->setState(&incoming);
            if(mode=="rollback"){std::filesystem::rename(moved,dll,ec);if(ec)return fail("restore installed runtime: "+ec.message());}
            S::MemoryStream after;
            const bool unchanged=component->getState(&after)==S::kResultOk&&after.getSize()==prior.getSize()&&
                std::memcmp(after.getData(),prior.getData(),static_cast<std::size_t>(prior.getSize()))==0;
            if(mode=="switch"){
                if(result!=S::kResultOk)return fail("positive active bank switch failed");
                (void)pump(180);const auto changed=pump(45,true);
                std::cout<<"ACTIVE_BANK_SWITCH_POSITIVE_PEAK="<<changed<<std::endl;
                if(changed<.01f||nonfinite||unexpected)return fail("positive active bank switch PCM failed");
                verdict("AUDIT_ACTIVE_BANK_SWITCH_CONTROL",!unchanged);
            }else{
            const auto continued=pump(40);
            std::cout<<"ROLLBACK_SETSTATE_RESULT="<<result<<" STATE_UNCHANGED="<<unchanged<<" CONTINUED_PEAK="<<continued<<std::endl;
            verdict("AUDIT_A06_ACTIVE_ROLLBACK",result!=S::kResultOk&&unchanged&&continued>=.01f);
            }
        }
    }
    if(processor->setProcessing(false)!=S::kResultOk||component->setActive(false)!=S::kResultOk)return fail("deactivate");
    controller->release();processor->release();
    if(component->terminate()!=S::kResultOk)return fail("terminate");
    component->release();factory->release();
    if(!exitDll())return fail("module shutdown");
    FreeLibrary(module);
    std::cout<<"AUDIT_NATIVE_RUNTIME_EXECUTED mode="<<mode<<" failures="<<errors<<" nonfinite_or_unwritten="<<nonfinite<<std::endl;
    return errors||nonfinite?1:0;
}
