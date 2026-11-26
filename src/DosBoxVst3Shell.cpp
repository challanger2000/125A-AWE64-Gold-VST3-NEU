#include "DosBoxVst3Shell.hpp"
#include "CreativeMidiBankSelect.hpp"
#include "CreativeMidiControllers.hpp"
#include "CreativeEffectMidi.hpp"
#include "ExternalSoundFont.hpp"
#include "PresetBrowserView.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdlib>

namespace awe64 {
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
constexpr ParamID kProgram = 1000u, kMaster = 1001u, kBank = 1002u;
constexpr ParamID kReverbType=1200u,kChorusType=1201u;
constexpr ParamID kReverbSend=1110u,kChorusSend=1111u;
constexpr std::array<const char*,12> kControlLabels{
    "AMP ATTACK","AMP DECAY","AMP SUSTAIN","AMP RELEASE",
    "FILTER ATTACK","FILTER DECAY","FILTER SUSTAIN","FILTER RELEASE",
    "CUTOFF","RESONANCE","REVERB SEND","CHORUS SEND"
};
// Serialised state is explicitly marked as DEVelopment only. This avoids
// accidentally claiming bit-identical recall with an old P007 VM version.
constexpr std::array<std::uint8_t,4> kSignature{{'D','B','X','D'}};
constexpr std::size_t kStateLength = 11;
// Non-realtime: identity of the exact accepted guest bytes, not metadata.
bool hashBank(const std::vector<std::uint8_t>& bytes,std::array<std::uint8_t,32>& digest) noexcept {
    if(bytes.empty()||bytes.size()>32ull*1024*1024)return false;
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0;
    if(ok)ok=BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg,0);
    return ok;
}
} // namespace

tresult PLUGIN_API DosBoxVst3Shell::queryInterface(const TUID iid,void** obj) {
    if(!obj)return kInvalidArgument;
    if(FUnknownPrivate::iidEqual(iid,IMidiMapping::iid)) {
        *obj=static_cast<IMidiMapping*>(this);addRef();return kResultOk;
    }
    return SingleComponentEffect::queryInterface(iid,obj);
}
tresult PLUGIN_API DosBoxVst3Shell::getMidiControllerAssignment(
    int32 bus,int16 channel,CtrlNumber number,ParamID& id) {
    if(bus!=0||channel<0||channel>=16)return kResultFalse;
    const int slot=rt::midiControllerIndex(static_cast<unsigned>(number));
    if(slot<0)return kResultFalse;
    id=rt::midiControllerBase+static_cast<unsigned>(slot)*rt::midiControllerStride+
        static_cast<unsigned>(channel);
    return kResultTrue;
}
tresult PLUGIN_API DosBoxVst3Shell::initialize(FUnknown* context) {
    if(SingleComponentEffect::initialize(context)!=kResultOk)return kResultFalse;
    addEventInput(STR16("Creative MIDI (16 channels)"),16);
    addAudioOutput(STR16("AWE64 Stereo"),SpeakerArr::kStereo);
    parameters.addParameter(new RangeParameter(STR16("MIDI Program"),kProgram,nullptr,0.,127.,0.,127));
    parameters.addParameter(new RangeParameter(STR16("MIDI Bank"),kBank,nullptr,0.,127.,0.,127));
    parameters.addParameter(new RangeParameter(STR16("Master Volume"),kMaster,STR16("%"),0.,100.,100.));
    for(std::size_t i=0;i<kControlLabels.size();++i) {
        String128 name{};
        UString(name,128).fromAscii(kControlLabels[i]);
        parameters.addParameter(new RangeParameter(name,1100u+static_cast<ParamID>(i),
            STR16("%"),0.,100.,0.));
    }
    parameters.addParameter(new RangeParameter(STR16("Reverb Type"),kReverbType,nullptr,0.,7.,4.,7));
    parameters.addParameter(new RangeParameter(STR16("Chorus Type"),kChorusType,nullptr,0.,7.,2.,7));
    // Host MIDI CC routing via the standard IMidiMapping interface.
    // Defaults are metadata only; never send CCs at boot that would alter
    // the original Creative bank's initial controller state.
    for(unsigned i=0;i<rt::midiControllerSpecs.size();++i) {
        const auto& spec=rt::midiControllerSpecs[i];
        for(unsigned ch=0;ch<16;++ch) {
            String128 label{};
            const auto title=std::string(spec.name)+" CH "+std::to_string(ch+1);
            UString(label,128).fromAscii(title.c_str());
            parameters.addParameter(new RangeParameter(label,
                rt::midiControllerBase+i*rt::midiControllerStride+ch,
                nullptr,0.,1.,spec.initial,0,ParameterInfo::kCanAutomate|ParameterInfo::kIsHidden));
        }
    }
    const auto binary=integration::DosBoxRuntimeHost::pluginBinaryPath();
    const auto bankPath=integration::DosBoxRuntimeHost::privateAssetsDir(binary)/"SYNTHGM.SF2";
    if(guest::readDefaultBankSnapshot(bankPath,requestedBankBytes_)){
        if(!hashBank(requestedBankBytes_,requestedBankDigest_))return kResultFalse;
        requestedBankDigestValid_=true;
        if(!loadCatalog({},originalMidiCatalog_,&requestedBankBytes_))
            originalMidiCatalog_.SetOriginalCreativeMidiNumbers();
    }else originalMidiCatalog_.SetOriginalCreativeMidiNumbers();
    startedCatalog_=originalMidiCatalog_;
    return kResultOk;
}
std::string DosBoxVst3Shell::requestedSoundFont() {
    std::lock_guard<std::mutex> lock(selectionMutex_);
    return requestedBank_;
}
std::string DosBoxVst3Shell::soundFontStatus() {
    std::lock_guard<std::mutex> lock(selectionMutex_);
    if(requestedBank_!=startedBank_)
        return "SF2 vorgemerkt: beim naechsten Aktivieren laden.";
    return {};
}
bool DosBoxVst3Shell::loadCatalog(const std::string& selected,AWE125::PresetCatalog& candidate,
                                   const std::vector<std::uint8_t>* validatedBank) const {
    using Host=integration::DosBoxRuntimeHost;
    const auto binary=Host::pluginBinaryPath();
    const auto source=selected.empty()?(Host::privateAssetsDir(binary)/"SYNTHGM.SF2"):std::filesystem::u8path(selected);
    const auto utf8=source.u8string();
    const std::string path(reinterpret_cast<const char*>(utf8.data()),utf8.size());
    std::string error;
    if(validatedBank?candidate.LoadSoundFontBytes(*validatedBank,error):candidate.LoadSoundFont(path,error))candidate.KeepMidiBankZero();
    else if(selected.empty())candidate.SetOriginalCreativeMidiNumbers(); // no real names claimed for synthetic fixture
    return !candidate.Entries().empty();
}
void DosBoxVst3Shell::publishCatalogLocked(AWE125::PresetCatalog&& candidate) {
    // Caller holds lifecycle lock. Controller writes and UI snapshot publish
    // share the selection lock; audio reads only under lifecycle lock.
    std::lock_guard<std::mutex> lock(selectionMutex_);
    startedCatalog_=std::move(candidate);
    if(!startedCatalog_.Find(0,program_.load(std::memory_order_acquire))) {
        const auto program=startedCatalog_.Entries().front().program;
        program_.store(program,std::memory_order_release);
        (void)SingleComponentEffect::setParamNormalized(kProgram,program/127.);
    }
    bank_.store(0,std::memory_order_release);
    (void)SingleComponentEffect::setParamNormalized(kBank,0.);
    catalogRevision_.fetch_add(1,std::memory_order_release);
}
bool DosBoxVst3Shell::requestSoundFont(const std::string& path,int expectedProgram,
                                      const BankDigest* expectedDigest,const RecallControls* recall) {
    try {
        if(path.size()>32768 || path.find('\0')!=std::string::npos)return false;
        std::vector<std::uint8_t> bytes;bool adapted=false;
        bool haveBytes=false;
        if(path.empty()){
            const auto binary=integration::DosBoxRuntimeHost::pluginBinaryPath();
            haveBytes=guest::readDefaultBankSnapshot(integration::DosBoxRuntimeHost::privateAssetsDir(binary)/"SYNTHGM.SF2",bytes);
        }else{
            if(!guest::readExternalSf2(std::filesystem::u8path(path),bytes,adapted))return false;
            haveBytes=true;
        }
        BankDigest digest{};
        if(haveBytes&&!hashBank(bytes,digest))return false;
        if(expectedDigest&&(!haveBytes||digest!=*expectedDigest))return false;
        AWE125::PresetCatalog candidate;
        if(haveBytes){
            if(!loadCatalog(path,candidate,&bytes))return false;
        }else candidate.SetOriginalCreativeMidiNumbers();
        if(expectedProgram>=0 && (expectedProgram>127||!candidate.Find(0,static_cast<std::uint8_t>(expectedProgram))))return false;
        std::string acceptedPath=path;
        std::lock_guard<std::mutex> transition(transitionMutex_);
        // Release runtimeMutex_ before retiring old callbacks/worker.
        std::unique_ptr<integration::DosBoxRuntimeHost> retired;
        std::unique_lock<std::mutex> lifecycle(runtimeMutex_);
        if(!haveBytes&&(runtime_||!requestedBankBytes_.empty()))return false;
        auto applyRecall=[&](){
            if(!recall)return true;
            if(setParamNormalized(kBank,0.)!=kResultOk||
               setParamNormalized(kProgram,static_cast<double>(expectedProgram)/127.)!=kResultOk)return false;
            (void)setParamNormalized(kMaster,static_cast<double>(recall->volume)/10000.);
            (void)setParamNormalized(kReverbType,recall->effects[0]/7.);
            (void)setParamNormalized(kChorusType,recall->effects[1]/7.);
            (void)setParamNormalized(kReverbSend,recall->sends[0]/255.);
            (void)setParamNormalized(kChorusSend,recall->sends[1]/255.);
            for(unsigned i=0;i<10;++i)(void)setParamNormalized(1100u+i,recall->voice[i]/255.);
            return true;
        };
        bool sameSelection=false;
        {
            std::lock_guard<std::mutex> lock(selectionMutex_);
            sameSelection=requestedBank_==path&&requestedBankDigestValid_==haveBytes&&
                          (!haveBytes||requestedBankDigest_==digest);
        }
        if(sameSelection&&(!runtime_||!runtime_->finished()))return applyRecall();
        if(runtime_) {
            if(!startRuntimeLocked(lifecycle,path,&bytes,&candidate,&retired))return false;
        } else publishCatalogLocked(std::move(candidate));
        requestedBankBytes_.swap(bytes);
        {
            std::lock_guard<std::mutex> lock(selectionMutex_);
            requestedBank_.swap(acceptedPath);
            requestedBankDigest_=digest;requestedBankDigestValid_=haveBytes;
        }
        const bool applied=applyRecall();
        lifecycle.unlock();
        retired.reset();
        return applied;
    } catch (...) {return false;}
}
std::uint64_t DosBoxVst3Shell::refreshBrowserCatalog() {
    // Only the UI thread writes originalMidiCatalog_. The audio thread never
    // reads it. Activation publishes a separate immutable-by-lock snapshot.
    std::lock_guard<std::mutex> lock(selectionMutex_);
    const auto revision=catalogRevision_.load(std::memory_order_relaxed);
    if(revision!=displayedRevision_) {
        originalMidiCatalog_=startedCatalog_;
        displayedRevision_=revision;
    }
    return revision;
}
void DosBoxVst3Shell::stopRuntimeLocked() noexcept {
    if(runtime_) {
        runtime_->stop();
        runtime_.reset();
    }
    demandActive_.store(false,std::memory_order_release);
    timedProcessing_.store(false,std::memory_order_release);
    hostDemand_.reset();
    guestStatus_.store(0,std::memory_order_release);
}
bool DosBoxVst3Shell::startRuntimeLocked(std::unique_lock<std::mutex>& lifecycle,const std::string& selected,
                                        const std::vector<std::uint8_t>* validatedBank,
                                        AWE125::PresetCatalog* preparedCatalog,
                                        std::unique_ptr<integration::DosBoxRuntimeHost>* retired) {
    try {
        auto engine=std::make_unique<integration::DosBoxRuntimeHost>();
        const auto ownBinary=engine->pluginBinaryPath();
        const auto privateDir=integration::DosBoxRuntimeHost::privateAssetsDir(ownBinary);
        std::vector<std::uint8_t> bytes;
        bool adapted=false;
        const bool loadFromDisk=!validatedBank;
        if(loadFromDisk) {
            if(selected.empty()){
                if(!guest::readDefaultBankSnapshot(privateDir/"SYNTHGM.SF2",bytes))return false;
            }else if(!guest::readExternalSf2(std::filesystem::u8path(selected),bytes,adapted))return false;
            validatedBank=&bytes;
        }
        BankDigest diskDigest{};
        if(loadFromDisk&&!hashBank(*validatedBank,diskDigest))return false;
        AWE125::PresetCatalog candidate;
        if(preparedCatalog)candidate=std::move(*preparedCatalog);
        else if(!loadCatalog(selected,candidate,validatedBank))return false;
        std::string acceptedStarted=selected;
        const bool replacing=bool(runtime_);
        if(replacing&&!retired)return false;
        const auto lead=demandRequested_?hostDemand_.lead():0u;
        // All non-RT lifecycle transitions are serialized by transitionMutex_.
        // Let the old engine process normally while the candidate stages/boots.
        // UART readiness is a necessary startup proof, not a soundfont
        // semantic or original-audio proof. Initial activation stays asynchronous.
        if(replacing)lifecycle.unlock();
        bool started=!ownBinary.empty()&&engine->begin(ownBinary,privateDir,selected,lead,validatedBank,captureGate_,replacing);
        if(started&&replacing)started=engine->waitForGuestMidiReady();
        if(!started){
            engine->stop();
            if(replacing)lifecycle.lock();
            if(!runtime_)guestStatus_.store(-1,std::memory_order_release);
            return false;
        }
        if(replacing)lifecycle.lock();
        // Only replace the previous guest after the new UART is live.
        const bool wasProcessing=timedProcessing_.load(std::memory_order_acquire);
        if(engine->clocked()&&!hostDemand_.activate()){
            lifecycle.unlock();engine->stop();lifecycle.lock();return false;
        }
        if(!engine->clocked())hostDemand_.reset();
        if(runtime_)*retired=std::move(runtime_);
        runtime_=std::move(engine);
        if(loadFromDisk)requestedBankBytes_.swap(bytes);
        timedProcessing_.store(wasProcessing,std::memory_order_release);
        demandActive_.store(runtime_->clocked(),std::memory_order_release);
        publishCatalogLocked(std::move(candidate));
        {
            std::lock_guard<std::mutex> lock(selectionMutex_);
            startedBank_.swap(acceptedStarted);
            if(loadFromDisk){requestedBankDigest_=diskDigest;requestedBankDigestValid_=true;}
        }
        guestStatus_.store(1,std::memory_order_release);
        initialProgramSent_=false;
        absoluteSamplePosition_=0;
        rateConverter_.reset();
        lastReverbType_=4;lastChorusType_=2;
        return true;
    } catch (...) {
        if(!lifecycle.owns_lock())lifecycle.lock();
        if(!runtime_)guestStatus_.store(-1,std::memory_order_release);
        return false;
    }
}
int32 PLUGIN_API DosBoxVst3Shell::getBusCount(MediaType type,BusDirection dir) {
    if((type!=kAudio&&type!=kEvent)||(dir!=kInput&&dir!=kOutput))return 0;
    return SingleComponentEffect::getBusCount(type,dir);
}
tresult PLUGIN_API DosBoxVst3Shell::getBusInfo(MediaType type,BusDirection dir,int32 index,BusInfo& info) {
    if(index<0||index>=getBusCount(type,dir))return kInvalidArgument;
    return SingleComponentEffect::getBusInfo(type,dir,index,info);
}
tresult PLUGIN_API DosBoxVst3Shell::activateBus(MediaType type,BusDirection dir,int32 index,TBool state) {
    if(index<0||index>=getBusCount(type,dir))return kInvalidArgument;
    return SingleComponentEffect::activateBus(type,dir,index,state);
}
tresult PLUGIN_API DosBoxVst3Shell::getBusArrangement(BusDirection dir,int32 index,SpeakerArrangement& arr) {
    if(index<0||index>=getBusCount(kAudio,dir))return kInvalidArgument;
    return SingleComponentEffect::getBusArrangement(dir,index,arr);
}
tresult PLUGIN_API DosBoxVst3Shell::setActive(TBool state) {
    std::lock_guard<std::mutex> transition(transitionMutex_);
    std::unique_lock<std::mutex> lifecycle(runtimeMutex_);
    if(state) {
        if(runtime_)return kResultOk;
        const auto selected=requestedSoundFont();
        // Keep accepted validated bytes across deactivate/reactivate.
        const auto* snapshot=!requestedBankBytes_.empty()?&requestedBankBytes_:nullptr;
        if(!startRuntimeLocked(lifecycle,selected,snapshot))return kResultFalse;
        return SingleComponentEffect::setActive(true);
    }
    stopRuntimeLocked();
    return SingleComponentEffect::setActive(false);
}
tresult PLUGIN_API DosBoxVst3Shell::setBusArrangements(SpeakerArrangement* inputs,int32 numIns,
    SpeakerArrangement* outputs,int32 numOuts) {
    // Native mixer and every process path are stereo-only. Decline unsupported
    // requests without changing the bus metadata or later process buffer shape.
    if(numIns!=0||numOuts!=1||!outputs||outputs[0]!=SpeakerArr::kStereo)return kResultFalse;
    return SingleComponentEffect::setBusArrangements(inputs,numIns,outputs,numOuts);
}
tresult PLUGIN_API DosBoxVst3Shell::canProcessSampleSize(int32 size) {
    return size==kSample32?kResultTrue:kResultFalse;
}
tresult PLUGIN_API DosBoxVst3Shell::setProcessing(TBool state) {
    // VST3 permits this on the processing thread. Never restart a guest,
    // wait, or allocate here. A processing suspension is a virtual pause:
    // the host does not publish a new demand horizon, so the guest reaches
    // the bounded existing horizon and holds there. Preserve the audio,
    // pending MIDI, resampler phase and host timeline for continuation.
    // Full deactivate/reactivate is a separate reset; this is not a seek.
    if(demandActive_.load(std::memory_order_acquire))
        timedProcessing_.store(state!=0,std::memory_order_release);
    else {
        // The legacy guest stays on its original asynchronous timebase.
        // Do not accumulate PCM when the host requests no callbacks.
        if(!state)captureGate_->suspend();
        legacyProcessing_.store(state!=0,std::memory_order_release);
        // Resume/reprime only on the first actual process callback.
    }
    return kResultOk;
}
uint32 PLUGIN_API DosBoxVst3Shell::getLatencySamples() {
    // Converter delay only; asynchronous guest/transport latency still
    // needs end-to-end measurement. No claim of sample-accurate rendering.
    return latencySamples_.load(std::memory_order_acquire);
}
tresult PLUGIN_API DosBoxVst3Shell::setupProcessing(ProcessSetup& setup) {
    std::lock_guard<std::mutex> transition(transitionMutex_);
    std::lock_guard<std::mutex> lifecycle(runtimeMutex_);
    if(runtime_&&runtime_->clocked())return kResultFalse;
    if(!rateConverter_.configure(setup.sampleRate))return kResultFalse;
    const char* requested=std::getenv("AWE64_TEST_HOST_DEMAND_CLOCK");
    demandRequested_=requested&&std::strcmp(requested,"1")==0;
    if(demandRequested_&&(setup.maxSamplesPerBlock<=0||!hostDemand_.configure(
            static_cast<unsigned>(std::lround(setup.sampleRate)),static_cast<unsigned>(setup.maxSamplesPerBlock))))return kResultFalse;
    latencySamples_.store(rateConverter_.latencySamples()+(demandRequested_?hostDemand_.latencySamples():0u),std::memory_order_release);
    return SingleComponentEffect::setupProcessing(setup);
}
tresult PLUGIN_API DosBoxVst3Shell::process(ProcessData& data) {
    // No DOSBox/Creative emulator on the DAW realtime callback.
    // Only bounded SPSC MIDI enqueue and actual original mixer PCM dequeue.
    auto silence=[&]() noexcept {
        if(data.symbolicSampleSize!=kSample32||!data.outputs||data.numOutputs<1||!data.outputs[0].channelBuffers32)return;
        auto& out=data.outputs[0];
        for(int32 ch=0;ch<std::min<int32>(out.numChannels,2);++ch)
            if(out.channelBuffers32[ch]&&data.numSamples>0)
                std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
        out.silenceFlags=out.numChannels>=2?3:1;
    };
    if(data.symbolicSampleSize!=kSample32||data.numSamples<0)return kResultFalse;
    std::unique_lock<std::mutex> lifecycle(runtimeMutex_,std::try_to_lock);
    if(!lifecycle.owns_lock()||!runtime_) {
        silence();
        // Do not acknowledge lost musical events during a lifecycle transition.
        return data.processMode==kOffline||
            (data.inputEvents&&data.inputEvents->getEventCount()>0)||
            (data.inputParameterChanges&&data.inputParameterChanges->getParameterCount()>0)?kResultFalse:kResultOk;
    }
    auto& engine=*runtime_;
    if(engine.clocked())return processTimed(data,engine);
    if(engine.finished()) {
        guestStatus_.store(engine.startupTimedOut()?-3:-2,std::memory_order_release);
        silence();
        return kResultFalse; // original DOS guest exited; no fake playback
    }
    if(data.numSamples>0&&!legacyProcessing_.load(std::memory_order_acquire)){
        silence();return kResultFalse;
    }
    if(data.numSamples>0&&(captureGate_->epoch()&1u)){
        engine.discardLegacyAudio();
        rateConverter_.reset();
        captureGate_->resume();
    }
    // Only the first actual nonempty host block after replacement opens
    // capture. Until then, even a ready candidate cannot build latency.
    if(data.numSamples>0)engine.startReplacementCapture();
    const auto hostBlock=static_cast<std::size_t>(data.numSamples);
    const bool offline=data.processMode==kOffline;
    bool ready=engine.ready(hostBlock);
    // Offline may wait for the native guest; realtime must never wait.
    if(offline&&hostBlock&&!ready){
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        while(!ready&&!engine.finished()&&std::chrono::steady_clock::now()<until){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ready=engine.ready(hostBlock);
        }
        if(!ready){
            if(engine.finished())guestStatus_.store(engine.startupTimedOut()?-3:-2,std::memory_order_release);
            silence();return kResultFalse;
        }
    }
    if(engine.finished()){guestStatus_.store(engine.startupTimedOut()?-3:-2,std::memory_order_release);silence();return kResultFalse;}
    if(ready){
        guestStatus_.store(2,std::memory_order_release);
        if(data.numSamples>0)engine.startLegacyMidi();
    }
    if(data.numSamples>0&&(!data.outputs||data.numOutputs!=1||data.outputs[0].numChannels!=2||
       !data.outputs[0].channelBuffers32||!data.outputs[0].channelBuffers32[0]||!data.outputs[0].channelBuffers32[1])){
        silence();return kResultFalse;
    }
    // Stage every point and MIDI event before committing any parameter or
    // publishing any guest message. A malformed block cannot partly change
    // controller state or enqueue its valid prefix. Fixed storage only.
    auto rejectBlock=[&]() noexcept -> tresult {
        legacyMidi_.reset();engine.requestMidiRecovery();silence();return kResultFalse;
    };
    legacyMaster_.begin(data.numSamples,master_.load(std::memory_order_acquire));
    legacyMidi_.reset();
    // Program points are time-ordered MIDI setup; other controls remain blockwise.
    const auto initialProgram=program_.load(std::memory_order_acquire);
    std::array<double,15> values{};
    std::array<bool,15> present{};
    auto slot=[](ParamID id) noexcept -> int {
        if(id==kProgram)return 0;
        if(id>=1100u&&id<=1111u)return 1+static_cast<int>(id-1100u);
        if(id==kReverbType)return 13;
        if(id==kChorusType)return 14;
        return -1;
    };
    unsigned total=0;
    if(data.inputParameterChanges){
        const auto count=data.inputParameterChanges->getParameterCount();
        if(count<0||count>4096)return rejectBlock();
        for(int32 i=0;i<count;++i){
            auto* q=data.inputParameterChanges->getParameterData(i);
            if(!q)return rejectBlock();
            const auto points=q->getPointCount();
            if(points<0||static_cast<unsigned>(points)>4096-total)return rejectBlock();
            total+=static_cast<unsigned>(points);
            const auto id=q->getParameterId();
            int32 previous=-1;
            for(int32 j=0;j<points;++j){
                int32 offset=0;ParamValue v=0.;
                if(q->getPoint(j,offset,v)!=kResultOk||!std::isfinite(v)||
                   offset<0||offset<previous||(data.numSamples?offset>=data.numSamples:offset!=0))
                    return rejectBlock();
                previous=offset;
                if(id==kMaster){
                    if(!legacyMaster_.add(offset,v))return rejectBlock();
                }else if(id==kProgram){
                    const auto p=static_cast<std::uint8_t>(std::lround(std::clamp(v,0.,1.)*127.));
                    if(startedCatalog_.Find(0,p)){
                        if(!legacyMidi_.add(rt::makeCreativeProgramChange(absoluteSamplePosition_+static_cast<std::uint64_t>(offset),p),rt::MidiPhase::Setup))return rejectBlock();
                        values[0]=p/127.;present[0]=true;
                    }
                }else if(id>=rt::midiControllerBase){
                    rt::MidiEvent midi{};
                    if(!rt::makeMappedController(id,v,absoluteSamplePosition_+static_cast<std::uint64_t>(offset),midi)||
                       !legacyMidi_.add(midi,rt::MidiPhase::Controller))return rejectBlock();
                }else{
                    const int index=slot(id);
                    if(index>=0){
                        values[static_cast<unsigned>(index)]=std::clamp(v,0.,1.);
                        present[static_cast<unsigned>(index)]=true;
                    }
                    // Bank zero and unknown regular IDs retain previous policy.
                }
            }
        }
    }
    if(data.inputEvents){
        const int32 count=data.inputEvents->getEventCount();
        if(count<0||static_cast<unsigned>(count)>4096-total)return rejectBlock();
        for(int32 i=0;i<count;++i){
            Event event{};
            if(data.inputEvents->getEvent(i,event)!=kResultOk){return rejectBlock();}
            std::uint8_t status=0,note=0,velocity=0;
            int32 channel=0,offset=0,pitch=0;
            if(event.type==Event::kNoteOnEvent){
                const auto& on=event.noteOn;
                channel=on.channel;offset=event.sampleOffset;pitch=on.pitch;
                status=0x90u;
                if(!std::isfinite(on.velocity)){return rejectBlock();}
                velocity=static_cast<std::uint8_t>(std::lround(
                    std::clamp(double(on.velocity),0.,1.)*127.));
            } else if(event.type==Event::kNoteOffEvent){
                const auto& off=event.noteOff;
                channel=off.channel;offset=event.sampleOffset;pitch=off.pitch;
                status=0x80u;
                if(!std::isfinite(off.velocity)){return rejectBlock();}
                velocity=static_cast<std::uint8_t>(std::lround(
                    std::clamp(double(off.velocity),0.,1.)*127.));
            } else continue;
            if(event.busIndex!=0||channel<0||channel>15||pitch<0||pitch>127||
               offset<0||(data.numSamples?offset>=data.numSamples:offset!=0)){
                return rejectBlock();
            }
            note=static_cast<std::uint8_t>(pitch);
            const auto when=absoluteSamplePosition_+
                static_cast<std::uint64_t>(std::clamp(offset,0,data.numSamples));
            if(!legacyMidi_.add({when,static_cast<std::uint8_t>(status|channel),note,velocity,3},rt::MidiPhase::Note))return rejectBlock();
        }
    }
    // No invalid inputs remain. Preserve existing blockwise semantics.
    bool accepted=true;
    master_.store(legacyMaster_.last(),std::memory_order_release);
    if(present[0]){
        const auto p=static_cast<std::uint8_t>(std::lround(values[0]*127.));
        if(startedCatalog_.Find(0,p))program_.store(p,std::memory_order_release);
    }
    for(unsigned i=0;i<10;++i)if(present[i+1])
        voiceControls_[i].store(static_cast<std::uint8_t>(std::lround(values[i+1]*255.)),std::memory_order_release);
    if(present[11])reverbSend_.store(static_cast<std::uint8_t>(std::lround(values[11]*255.)),std::memory_order_release);
    if(present[12])chorusSend_.store(static_cast<std::uint8_t>(std::lround(values[12]*255.)),std::memory_order_release);
    if(present[13])reverbType_.store(static_cast<std::uint8_t>(std::lround(values[13]*7.)),std::memory_order_release);
    if(present[14])chorusType_.store(static_cast<std::uint8_t>(std::lround(values[14]*7.)),std::memory_order_release);
    {
        std::uint64_t lo=0;
        for(unsigned i=0;i<8;++i)lo|=std::uint64_t(voiceControls_[i].load(std::memory_order_acquire))<<(i*8);
        const unsigned hi=voiceControls_[8].load(std::memory_order_acquire)|(unsigned(voiceControls_[9].load(std::memory_order_acquire))<<8);
        accepted=engine.setVoiceControls(lo,hi)&&accepted;
    }
    const auto bank=bank_.load(std::memory_order_acquire);
    const auto program=initialProgram;
    // The patched MPU pump waits for the original guest UART handshake.
    // Queue setup and notes even while PCM is still priming.
    if(!initialProgramSent_||lastBank_!=bank||lastProgram_!=program) {
        // The original Creative DOS demo uses CC0 (BANK MSB), not CC32
        // (BANK LSB), to select an AWE SF2 bank. Keep MIDI routed through
        // the original MPU-401, never synthesize or write EMU registers here.
        bool setupAccepted=true;
        if(!initialProgramSent_||lastBank_!=bank)
            setupAccepted=engine.sendMidi(rt::makeCreativeBankSelect(absoluteSamplePosition_,bank));
        setupAccepted=engine.sendMidi(rt::makeCreativeProgramChange(absoluteSamplePosition_,program))&&setupAccepted;
        if(setupAccepted){initialProgramSent_=true;lastBank_=bank;lastProgram_=program;}
        accepted=setupAccepted&&accepted;
    }
    accepted=engine.setAbsoluteSends(reverbSend_.load(std::memory_order_acquire),chorusSend_.load(std::memory_order_acquire))&&accepted;
    const auto reverb=reverbType_.load(std::memory_order_acquire);
    const auto chorus=chorusType_.load(std::memory_order_acquire);
    if(reverb!=lastReverbType_){
        if(engine.sendMidi(rt::makeCreativeEffectType(absoluteSamplePosition_,false,reverb)))lastReverbType_=reverb;
        else accepted=false;
    }
    if(chorus!=lastChorusType_){
        if(engine.sendMidi(rt::makeCreativeEffectType(absoluteSamplePosition_,true,chorus)))lastChorusType_=chorus;
        else accepted=false;
    }
    if(!legacyMidi_.submitLegacy([&](const rt::MidiEvent& e) noexcept {
        const bool programChange=e.command==rt::GuestCommand::Midi&&e.status==0xc0u;
        // Never skip a setup during recovery; lastProgram_ tracks actual
        // accepted FIFO messages, not the last automation point in the block.
        if(programChange&&initialProgramSent_&&lastProgram_==e.data1&&!engine.midiRecoveryPending())return true;
        const bool sent=engine.sendMidi(e);
        if(programChange&&sent)lastProgram_=e.data1;
        return sent;
    }))accepted=false;
    if(data.outputs&&data.numOutputs>0&&data.outputs[0].channelBuffers32&&
       data.outputs[0].numChannels>=2&&data.numSamples>0){
        auto* left=data.outputs[0].channelBuffers32[0];
        auto* right=data.outputs[0].channelBuffers32[1];
        if(left&&right){
            const auto need=rateConverter_.requiredInputFrames(hostBlock);
            if(offline&&(!ready||!engine.waitForAudio(need,true))){
                engine.requestMidiRecovery();silence();return kResultFalse;
            }
            engine.measureQueueBeforeBlock();
            // Preserve native mixer samples at 44.1 kHz. For higher host
            // rates convert the original final PCM, not the chip clock.
            if(!ready){std::fill_n(left,data.numSamples,0.f);std::fill_n(right,data.numSamples,0.f);}
            else if(rateConverter_.bypass()){
                const auto read=engine.readAudio(left,right,hostBlock);
                if(offline&&read!=hostBlock){engine.requestMidiRecovery();silence();return kResultFalse;}
            }else{
                bool full=true;
                rateConverter_.render(left,right,hostBlock,[&engine,&full]() noexcept {
                    rt::StereoFrame f{};if(engine.readAudio(&f.left,&f.right,1)!=1)full=false;return f;
                });
                if(offline&&!full){engine.requestMidiRecovery();silence();return kResultFalse;}
            }
            engine.measureQueueAfterBlock();
            legacyMaster_.apply(left,right);
            auto& out=data.outputs[0];
            out.silenceFlags=0;
            if(std::all_of(left,left+data.numSamples,[](float x){return x==0.f;}))out.silenceFlags|=1;
            if(std::all_of(right,right+data.numSamples,[](float x){return x==0.f;}))out.silenceFlags|=2;
        }
    }
    if(data.numSamples>0)
        absoluteSamplePosition_+=static_cast<std::uint64_t>(data.numSamples);
    if(engine.finished()){guestStatus_.store(engine.startupTimedOut()?-3:-2,std::memory_order_release);silence();return kResultFalse;}
    if(!accepted){engine.requestMidiRecovery();return kResultFalse;}
    return kResultOk;
}
tresult DosBoxVst3Shell::processTimed(ProcessData& data,integration::DosBoxRuntimeHost& engine) {
    // The lifecycle mutex is already owned by process(). Neither this
    // function nor HostDemandSession calls DOSBox or waits on the RT path.
    auto silence=[&]() noexcept {
        if(!data.outputs||data.numOutputs<1||!data.outputs[0].channelBuffers32)return;
        auto& out=data.outputs[0];
        for(int32 ch=0;ch<out.numChannels;++ch)
            if(out.channelBuffers32[ch]&&data.numSamples>0)
                std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
        out.silenceFlags=out.numChannels>=2?3:1;
    };
    auto fault=[&]() noexcept -> tresult {
        hostDemand_.fail();engine.requestMidiRecovery();
        guestStatus_.store(engine.startupTimedOut()?-3:-2,std::memory_order_release);silence();return kResultFalse;
    };
    if(hostDemand_.faulted()||engine.finished()||data.numSamples<0)return fault();
    if(!timedProcessing_.load(std::memory_order_acquire)) {
        // A parameter-only flush may precede setProcessing(true).
        if(data.numSamples>0){silence();return kResultFalse;}
    }
    if(!engine.ready(static_cast<std::size_t>(data.numSamples))) {
        if(engine.finished())return fault();
        // Native boot proof is still a gate. Do not silently discard an
        // early MIDI event or claim a parameter change was accepted.
        silence();
        if((data.inputEvents&&data.inputEvents->getEventCount()>0)||
           (data.inputParameterChanges&&data.inputParameterChanges->getParameterCount()>0))return fault();
        return kResultOk;
    }
    guestStatus_.store(2,std::memory_order_release);
    if(data.numSamples>0&&(!data.outputs||data.numOutputs!=1||data.outputs[0].numChannels!=2||
       !data.outputs[0].channelBuffers32||!data.outputs[0].channelBuffers32[0]||!data.outputs[0].channelBuffers32[1]))
        return fault();
    if(!hostDemand_.begin(static_cast<unsigned>(data.numSamples)))return fault();
    // A snapshot is the baseline for this block. Only changed members are
    // published below. Transactional save/recall still requires separate QA.
    rt::HostControlState prior{};
    prior.program=program_.load(std::memory_order_acquire);
    prior.master=master_.load(std::memory_order_acquire);
    prior.reverbType=reverbType_.load(std::memory_order_acquire);
    prior.chorusType=chorusType_.load(std::memory_order_acquire);
    prior.reverbSend=reverbSend_.load(std::memory_order_acquire);
    prior.chorusSend=chorusSend_.load(std::memory_order_acquire);
    for(unsigned i=0;i<10;++i)prior.voice[i]=voiceControls_[i].load(std::memory_order_acquire);
    auto next=prior;
    // Limit work even for a hostile host supplying enormous point counts.
    // The portable planner also enforces aggregate capacity.
    unsigned inputs=0;
    constexpr unsigned maxInputs=4096;
    bool valid=true;
    if(data.inputParameterChanges) {
        const auto count=data.inputParameterChanges->getParameterCount();
        if(count<0||count>static_cast<int32>(maxInputs))valid=false;
        for(int32 i=0;valid&&i<count;++i) {
            auto* q=data.inputParameterChanges->getParameterData(i);
            if(!q){valid=false;break;}
            const auto points=q->getPointCount();
            if(points<0||static_cast<unsigned>(points)>maxInputs-inputs){valid=false;break;}
            inputs+=static_cast<unsigned>(points);
            for(int32 j=0;j<points&&valid;++j) {
                int32 offset=0;ParamValue value=0.;
                valid=q->getPoint(j,offset,value)==kResultOk&&hostDemand_.parameter(offset,q->getParameterId(),value);
            }
        }
    }
    if(valid&&data.inputEvents) {
        const auto count=data.inputEvents->getEventCount();
        if(count<0||static_cast<unsigned>(count)>maxInputs-inputs)valid=false;
        for(int32 i=0;valid&&i<count;++i) {
            Event event{};
            if(data.inputEvents->getEvent(i,event)!=kResultOk){valid=false;break;}
            if(event.type==Event::kNoteOnEvent) {
                const auto& n=event.noteOn;
                valid=event.busIndex==0&&hostDemand_.note(event.sampleOffset,n.channel,n.pitch,n.velocity,true);
            } else if(event.type==Event::kNoteOffEvent) {
                const auto& n=event.noteOff;
                valid=event.busIndex==0&&hostDemand_.note(event.sampleOffset,n.channel,n.pitch,n.velocity,false);
            }
        }
    }
    if(!valid)hostDemand_.reject();
    const auto result=hostDemand_.submit(engine,next,[this](unsigned p) noexcept {
        return startedCatalog_.Find(0,static_cast<std::uint8_t>(p))!=nullptr;
    });
    if(result==rt::HostDemandSession::Result::Fault)return fault();
    // An overflowing batch requests recovery. Authoritative changed controls
    // are reasserted after the recovery epoch; lost sustain is not re-engaged.
    if(next.program!=prior.program)program_.store(next.program,std::memory_order_release);
    if(next.master!=prior.master)master_.store(next.master,std::memory_order_release);
    if(next.reverbType!=prior.reverbType)reverbType_.store(next.reverbType,std::memory_order_release);
    if(next.chorusType!=prior.chorusType)chorusType_.store(next.chorusType,std::memory_order_release);
    if(next.reverbSend!=prior.reverbSend)reverbSend_.store(next.reverbSend,std::memory_order_release);
    if(next.chorusSend!=prior.chorusSend)chorusSend_.store(next.chorusSend,std::memory_order_release);
    for(unsigned i=0;i<10;++i)if(next.voice[i]!=prior.voice[i])voiceControls_[i].store(next.voice[i],std::memory_order_release);
    if(!data.numSamples)return kResultOk;
    const auto frames=static_cast<std::size_t>(data.numSamples);
    const auto need=rateConverter_.requiredInputFrames(frames);
    // In realtime, fail closed rather than advancing an audio clock on
    // artificial samples. In offline mode a bounded non-RT wait is allowed.
    if(!engine.waitForAudio(need,data.processMode==kOffline))return fault();
    auto& out=data.outputs[0];auto* l=out.channelBuffers32[0];auto* r=out.channelBuffers32[1];
    engine.measureQueueBeforeBlock();
    bool full=true;
    if(rateConverter_.bypass())full=engine.readAudio(l,r,frames)==frames;
    else rateConverter_.render(l,r,frames,[&]() noexcept {
        rt::StereoFrame f{};if(engine.readAudio(&f.left,&f.right,1)!=1)full=false;return f;
    });
    engine.measureQueueAfterBlock();
    if(!full||engine.finished())return fault();
    hostDemand_.applyMaster(l,r,prior.master);
    // Truthful silence flags for both channels.
    out.silenceFlags=0;
    if(std::all_of(l,l+frames,[](float x){return x==0.f;}))out.silenceFlags|=1;
    if(std::all_of(r,r+frames,[](float x){return x==0.f;}))out.silenceFlags|=2;
    return kResultOk;
}

ParamValue PLUGIN_API DosBoxVst3Shell::getParamNormalized(ParamID id) {
    if(id==kProgram)return program_.load(std::memory_order_acquire)/127.;
    if(id==kBank)return 0.;
    if(id==kMaster)return master_.load(std::memory_order_acquire);
    if(id>=1100u&&id<1110u)return voiceControls_[id-1100u].load(std::memory_order_acquire)/255.;
    if(id==kReverbSend)return reverbSend_.load(std::memory_order_acquire)/255.;
    if(id==kChorusSend)return chorusSend_.load(std::memory_order_acquire)/255.;
    if(id==kReverbType)return reverbType_.load(std::memory_order_acquire)/7.;
    if(id==kChorusType)return chorusType_.load(std::memory_order_acquire)/7.;
    return SingleComponentEffect::getParamNormalized(id);
}
tresult PLUGIN_API DosBoxVst3Shell::setParamNormalized(ParamID id,ParamValue value) {
    if(!std::isfinite(value)||value<0.||value>1.)return kInvalidArgument;
    std::unique_lock<std::mutex> selection(selectionMutex_,std::defer_lock);
    if(id==kBank && std::lround(value*127.)!=0)return kResultFalse;
    if(id==kProgram) {
        selection.lock();
        if(!startedCatalog_.Find(0,static_cast<std::uint8_t>(std::lround(value*127.))))return kResultFalse;
    }
    const auto result=SingleComponentEffect::setParamNormalized(id,value);
    if(result!=kResultOk)return result;
    if(id==kProgram)
        program_.store(static_cast<std::uint8_t>(std::lround(127.*value)),
                       std::memory_order_release);
    else if(id==kBank)
        bank_.store(static_cast<std::uint8_t>(std::lround(127.*value)),
                    std::memory_order_release);
    else if(id==kMaster)
        master_.store(static_cast<float>(value),std::memory_order_release);
    else if(id>=1100u&&id<1110u)
        voiceControls_[id-1100u].store(static_cast<std::uint8_t>(std::lround(value*255.)),std::memory_order_release);
    else if(id==kReverbSend)
        reverbSend_.store(static_cast<std::uint8_t>(std::lround(value*255.)),std::memory_order_release);
    else if(id==kChorusSend)
        chorusSend_.store(static_cast<std::uint8_t>(std::lround(value*255.)),std::memory_order_release);
    else if(id==kReverbType)
        reverbType_.store(static_cast<std::uint8_t>(std::lround(value*7.)),std::memory_order_release);
    else if(id==kChorusType)
        chorusType_.store(static_cast<std::uint8_t>(std::lround(value*7.)),std::memory_order_release);
    return kResultOk;
}
tresult PLUGIN_API DosBoxVst3Shell::getState(IBStream* stream) {
    if(!stream)return kInvalidArgument;
    std::array<std::uint8_t,kStateLength> image{};
    for(unsigned i=0;i<4;i++)image[i]=kSignature[i];
    std::string selected;BankDigest digest{};bool identified=false;
    {std::lock_guard<std::mutex> lock(selectionMutex_);selected=requestedBank_;digest=requestedBankDigest_;identified=requestedBankDigestValid_;}
    const std::array<std::uint8_t,2> effects{reverbType_.load(std::memory_order_acquire),chorusType_.load(std::memory_order_acquire)};
    std::array<std::uint8_t,2> sends{reverbSend_.load(std::memory_order_acquire),chorusSend_.load(std::memory_order_acquire)};
    std::array<std::uint8_t,10> controls{};
    for(unsigned i=0;i<10;++i)controls[i]=voiceControls_[i].load(std::memory_order_acquire);
    const bool edited=std::any_of(controls.begin(),controls.end(),[](auto v){return v!=0;});
    // v6 = complete v5 controls + path + SHA-256 of exact guest bytes.
    image[4]=identified?6:(edited?5:((sends[0]||sends[1])?4:((effects[0]!=4||effects[1]!=2)?3:(selected.empty()?1:2))));
    image[5]=bank_.load(std::memory_order_acquire);
    image[6]=program_.load(std::memory_order_acquire);
    const auto level=static_cast<std::uint32_t>(std::lround(
        std::clamp(master_.load(std::memory_order_acquire),0.f,1.f)*10000.));
    image[7]=static_cast<std::uint8_t>(level&255u);
    image[8]=static_cast<std::uint8_t>((level>>8)&255u);
    image[9]=0x5a; image[10]=0xa5;
    int32 written=0;
    if(stream->write(image.data(),static_cast<int32>(image.size()),&written)!=kResultOk ||
       written!=static_cast<int32>(image.size()))return kResultFalse;
    if(image[4]>=3){
        if(stream->write(const_cast<std::uint8_t*>(effects.data()),2,&written)!=kResultOk||written!=2)return kResultFalse;
        if(image[4]>=4&&(stream->write(sends.data(),2,&written)!=kResultOk||written!=2))return kResultFalse;
        if(image[4]>=5&&(stream->write(controls.data(),10,&written)!=kResultOk||written!=10))return kResultFalse;
    } else if(selected.empty())return kResultOk;
    const auto n=static_cast<std::uint32_t>(selected.size());
    std::array<std::uint8_t,4> length{std::uint8_t(n),std::uint8_t(n>>8),std::uint8_t(n>>16),std::uint8_t(n>>24)};
    if(stream->write(length.data(),4,&written)!=kResultOk||written!=4)return kResultFalse;
    if(n&&(stream->write(const_cast<char*>(selected.data()),static_cast<int32>(n),&written)!=kResultOk||
           written!=static_cast<int32>(n)))return kResultFalse;
    if(identified&&(stream->write(digest.data(),static_cast<int32>(digest.size()),&written)!=kResultOk||
                   written!=static_cast<int32>(digest.size())))return kResultFalse;
    return kResultOk;
}
tresult PLUGIN_API DosBoxVst3Shell::setState(IBStream* stream) {
    if(!stream)return kInvalidArgument;
    std::array<std::uint8_t,kStateLength> image{};
    int32 read=0;
    if(stream->read(image.data(),static_cast<int32>(image.size()),&read)!=kResultOk||
       read!=static_cast<int32>(image.size())||
       !std::equal(kSignature.begin(),kSignature.end(),image.begin())||
       (image[4]<1||image[4]>6)||image[9]!=0x5a||image[10]!=0xa5||
       image[5]!=0||image[6]>127)return kResultFalse;
    const unsigned volume=image[7]|(static_cast<unsigned>(image[8])<<8);
    if(volume>10000)return kResultFalse;
    std::array<std::uint8_t,2> effects{4,2};
    if(image[4]>=3&&(stream->read(effects.data(),2,&read)!=kResultOk||read!=2||effects[0]>7||effects[1]>7))return kResultFalse;
    std::array<std::uint8_t,2> sends{};
    if(image[4]>=4&&(stream->read(sends.data(),2,&read)!=kResultOk||read!=2))return kResultFalse;
    std::array<std::uint8_t,10> controls{};
    if(image[4]>=5&&(stream->read(controls.data(),10,&read)!=kResultOk||read!=10))return kResultFalse;
    std::string selected;
    if(image[4]>=2){
        std::array<std::uint8_t,4> length{};
        if(stream->read(length.data(),4,&read)!=kResultOk||read!=4)return kResultFalse;
        const auto n=std::uint32_t(length[0])|(std::uint32_t(length[1])<<8)|
                     (std::uint32_t(length[2])<<16)|(std::uint32_t(length[3])<<24);
        if((n==0&&image[4]==2)||n>32768)return kResultFalse;
        selected.resize(n);
        if(n&&(stream->read(selected.data(),static_cast<int32>(n),&read)!=kResultOk||
           read!=static_cast<int32>(n)))return kResultFalse;
    }
    BankDigest expected{};
    if(image[4]==6&&(stream->read(expected.data(),static_cast<int32>(expected.size()),&read)!=kResultOk||
                     read!=static_cast<int32>(expected.size())))return kResultFalse;
    const RecallControls recall{volume,effects,sends,controls};
    return requestSoundFont(selected,image[6],image[4]==6?&expected:nullptr,&recall)?kResultOk:kResultFalse;
}
tresult PLUGIN_API DosBoxVst3Shell::setComponentState(IBStream* stream) {
    return setState(stream);
}
VSTGUI::CView* DosBoxVst3Shell::createCustomView(
    VSTGUI::UTF8StringPtr name,const VSTGUI::UIAttributes&,
    const VSTGUI::IUIDescription*,VSTGUI::VST3Editor*) {
    if(!name||std::strcmp(name,"P007P165Panel")!=0)return nullptr;
    (void)refreshBrowserCatalog();
    return new AWE125::PresetBrowserView({0,0,1000,708},this,
        originalMidiCatalog_,unusedFmCatalog_,std::string{},
        [this](){
            const auto selection=soundFontStatus();
            if(!selection.empty())return selection;
            const int s=guestStatus_.load(std::memory_order_acquire);
            return s==2 ? std::string("DOSBOX-X: MIDI-GAST BEREIT") :
                   s==1 ? std::string("DOSBOX-X: ORIGINALER CREATIVE-DOS-GAST STARTET") :
                   s==-1 ? std::string("DOSBOX-X: ORIGINAL-DATEIEN ODER DLL FEHLEN") :
                   s==-3 ? std::string("DOSBOX-X: STARTZEIT UEBERSCHRITTEN") :
                   s==-2 ? std::string("DOSBOX-X: GAST IST BEENDET") :
                           std::string("DOSBOX-X: GAST NICHT GESTARTET");
        },
        [this](){
            const int s=guestStatus_.load(std::memory_order_acquire);
            return s==2 ? std::string("ORIGINAL DOSBOX-X | MIDI-GAST BEREIT") :
                   s==1 ? std::string("ORIGINAL DOSBOX-X STARTET") :
                           std::string("AWE64 GOLD | KEIN AUDIO-GAST");
        },
        [this](const std::string&){return requestSoundFont({});},
        [this](const std::string& path){return requestSoundFont(path);},
        [this](){return requestedSoundFont();},
        [this](){return refreshBrowserCatalog();},
        [](std::uint16_t bank){return bank==0;});
}
IPlugView* PLUGIN_API DosBoxVst3Shell::createView(FIDString name) {
    if(name&&std::strcmp(name,ViewType::kEditor)==0){
        auto* editor=new VSTGUI::VST3Editor(this,"view","p007_p165.uidesc");
        editor->setDelegate(this);
        return editor;
    }
    return nullptr;
}
} // namespace awe64
