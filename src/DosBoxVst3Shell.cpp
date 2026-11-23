#include "DosBoxVst3Shell.hpp"
#include "CreativeMidiBankSelect.hpp"
#include "CreativeMidiControllers.hpp"
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

namespace awe64 {
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
constexpr ParamID kProgram = 1000u, kMaster = 1001u, kBank = 1002u;
constexpr std::array<const char*,12> kControlLabels{
    "AMP ATTACK","AMP DECAY","AMP SUSTAIN","AMP RELEASE",
    "FILTER ATTACK","FILTER DECAY","FILTER SUSTAIN","FILTER RELEASE",
    "CUTOFF","RESONANCE","REVERB SEND","CHORUS SEND"
};
// Serialised state is explicitly marked as DEVelopment only. This avoids
// accidentally claiming bit-identical recall with an old P007 VM version.
constexpr std::array<std::uint8_t,4> kSignature{{'D','B','X','D'}};
constexpr std::size_t kStateLength = 11;
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
    parameters.addParameter(new RangeParameter(STR16("Reverb Type"),1200u,nullptr,0.,7.,2.,7));
    parameters.addParameter(new RangeParameter(STR16("Chorus Type"),1201u,nullptr,0.,7.,3.,7));
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
    originalMidiCatalog_.SetOriginalCreativeMidiNumbers();
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
bool DosBoxVst3Shell::requestSoundFont(const std::string& path) {
    try {
        if(path.size()>32768 || path.find('\0')!=std::string::npos)return false;
        if(!path.empty()) {
            std::vector<std::uint8_t> bytes;bool adapted=false;
            AWE125::PresetCatalog candidate;std::string error;
            if(!guest::readExternalSf2(std::filesystem::u8path(path),bytes,adapted) ||
               !candidate.LoadSoundFont(path,error))return false;
            candidate.KeepMidiBankZero();
            if(candidate.Entries().empty())return false;
        }
        std::lock_guard<std::mutex> lifecycle(runtimeMutex_);
        const auto previous=requestedSoundFont();
        if(previous==path && (!runtime_||!runtime_->finished()))return true;
        if(runtime_) {
            // Exactly one DOSBox instance. Restart the unmodified guest with
            // the new bank. Never call DOSBox on the real-time callback.
            stopRuntimeLocked();
            if(!startRuntimeLocked(path)) {
                (void)startRuntimeLocked(previous);
                return false;
            }
        }
        {
            std::lock_guard<std::mutex> lock(selectionMutex_);
            requestedBank_=path;
        }
        return true;
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
    guestStatus_.store(0,std::memory_order_release);
}
bool DosBoxVst3Shell::startRuntimeLocked(const std::string& selected) {
    try {
        auto engine=std::make_unique<integration::DosBoxRuntimeHost>();
        const auto ownBinary=engine->pluginBinaryPath();
        const auto privateDir=integration::DosBoxRuntimeHost::privateAssetsDir(ownBinary);
        const auto source=selected.empty()?(privateDir/"SYNTHGM.SF2"):std::filesystem::u8path(selected);
        const auto utf8=source.u8string();
        const std::string path(reinterpret_cast<const char*>(utf8.data()),utf8.size());
        AWE125::PresetCatalog candidate;std::string error;
        if(candidate.LoadSoundFont(path,error))candidate.KeepMidiBankZero();
        else if(selected.empty())candidate.SetOriginalCreativeMidiNumbers(); // synthetic fixture only
        if(candidate.Entries().empty()||ownBinary.empty()||!engine->begin(ownBinary,privateDir,selected)) {
            guestStatus_.store(-1,std::memory_order_release);
            return false;
        }
        runtime_=std::move(engine);
        if(!candidate.Find(0,program_.load(std::memory_order_acquire))) {
            const auto program=candidate.Entries().front().program;
            (void)setParamNormalized(kProgram,static_cast<double>(program)/127.);
        }
        (void)setParamNormalized(kBank,0.);
        {
            std::lock_guard<std::mutex> lock(selectionMutex_);
            startedBank_=selected;
            startedCatalog_=std::move(candidate);
            catalogRevision_.fetch_add(1,std::memory_order_release);
        }
        guestStatus_.store(1,std::memory_order_release);
        initialProgramSent_=false;
        absoluteSamplePosition_=0;
        return true;
    } catch (...) {
        stopRuntimeLocked();
        guestStatus_.store(-1,std::memory_order_release);
        return false;
    }
}
tresult PLUGIN_API DosBoxVst3Shell::setActive(TBool state) {
    std::lock_guard<std::mutex> lifecycle(runtimeMutex_);
    if(state) {
        if(runtime_)return kResultOk;
        if(!startRuntimeLocked(requestedSoundFont()))return kResultFalse;
        return SingleComponentEffect::setActive(true);
    }
    stopRuntimeLocked();
    return SingleComponentEffect::setActive(false);
}
tresult PLUGIN_API DosBoxVst3Shell::canProcessSampleSize(int32 size) {
    return size==kSample32?kResultTrue:kResultFalse;
}
tresult PLUGIN_API DosBoxVst3Shell::setProcessing(TBool) {return kResultOk;}
tresult PLUGIN_API DosBoxVst3Shell::setupProcessing(ProcessSetup& setup) {
    // Do not play distorted/pitched audio at a mismatched host sample rate.
    // 48000/96000 Hz are NOT falsely advertised until true resampling exists.
    if(std::abs(setup.sampleRate-44100.0)>0.5)return kResultFalse;
    return SingleComponentEffect::setupProcessing(setup);
}
tresult PLUGIN_API DosBoxVst3Shell::process(ProcessData& data) {
    // No DOSBox/Creative emulator on the DAW realtime callback.
    // Only bounded SPSC MIDI enqueue and actual original mixer PCM dequeue.
    std::unique_lock<std::mutex> lifecycle(runtimeMutex_,std::try_to_lock);
    if(!lifecycle.owns_lock()||!runtime_) {
        // During a non-RT guest restart the DAW receives bounded silence.
        if(data.outputs&&data.numOutputs>0&&data.outputs[0].channelBuffers32) {
            auto& out=data.outputs[0];
            for(int32 ch=0;ch<out.numChannels;++ch)
                if(out.channelBuffers32[ch]&&data.numSamples>0)
                    std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
            out.silenceFlags=out.numChannels>=2?3:1;
        }
        return kResultOk;
    }
    auto& engine=*runtime_;
    if(engine.finished()) {
        guestStatus_.store(-2,std::memory_order_release);
        return kResultFalse; // original DOS guest exited; no fake playback
    }
    if(engine.running())guestStatus_.store(2,std::memory_order_release);

    if(data.inputParameterChanges) {
        for(int32 i=0;i<data.inputParameterChanges->getParameterCount();++i) {
            auto* q=data.inputParameterChanges->getParameterData(i);
            if(!q || q->getPointCount()<1)continue;
            if(q->getParameterId()>=rt::midiControllerBase) {
                // Preserve every mapped controller change in this block.
                for(int32 n=0;n<q->getPointCount();++n) {
                    int32 offset=0;ParamValue v=0.;rt::MidiEvent midi{};
                    if(q->getPoint(n,offset,v)==kResultOk &&
                       rt::makeMappedController(q->getParameterId(),v,
                           absoluteSamplePosition_+static_cast<std::uint64_t>(
                               std::clamp(offset,0,data.numSamples)),midi))
                        (void)engine.sendMidi(midi);
                }
                continue;
            }
            int32 offset=0;ParamValue normalized=0.;
            if(q->getPoint(q->getPointCount()-1,offset,normalized)!=kResultOk)
                continue;
            const double value=std::clamp(normalized,0.,1.);
            switch(q->getParameterId()) {
                case kProgram:
                    program_.store(static_cast<std::uint8_t>(std::lround(value*127.)),
                                   std::memory_order_release);break;
                case kBank:
                    bank_.store(static_cast<std::uint8_t>(std::lround(value*127.)),
                                std::memory_order_release);break;
                case kMaster:
                    master_.store(static_cast<float>(value),std::memory_order_release);
                    break;
                default:break;
            }
        }
    }
    const auto bank=bank_.load(std::memory_order_acquire);
    const auto program=program_.load(std::memory_order_acquire);
    if(!initialProgramSent_||lastBank_!=bank||lastProgram_!=program) {
        // The original Creative DOS demo uses CC0 (BANK MSB), not CC32
        // (BANK LSB), to select an AWE SF2 bank. Keep MIDI routed through
        // the original MPU-401, never synthesize or write EMU registers here.
        bool accepted=true;
        if(!initialProgramSent_||lastBank_!=bank)
            accepted=engine.sendMidi(
                rt::makeCreativeBankSelect(absoluteSamplePosition_,bank))&&accepted;
        accepted=engine.sendMidi(
            rt::makeCreativeProgramChange(absoluteSamplePosition_,program))&&accepted;
        if(accepted) {
            initialProgramSent_=true;lastBank_=bank;lastProgram_=program;
        }
    }
    if(data.inputEvents){
        const int32 count=data.inputEvents->getEventCount();
        for(int32 i=0;i<count;++i){
            Event event{};
            if(data.inputEvents->getEvent(i,event)!=kResultOk)continue;
            std::uint8_t status=0,note=0,velocity=0;
            int32 channel=0,offset=0,pitch=0;
            if(event.type==Event::kNoteOnEvent){
                const auto& on=event.noteOn;
                channel=on.channel;offset=event.sampleOffset;pitch=on.pitch;
                status=0x90u;
                velocity=static_cast<std::uint8_t>(std::lround(
                    std::clamp(double(on.velocity),0.,1.)*127.));
            } else if(event.type==Event::kNoteOffEvent){
                const auto& off=event.noteOff;
                channel=off.channel;offset=event.sampleOffset;pitch=off.pitch;
                status=0x80u;
                velocity=static_cast<std::uint8_t>(std::lround(
                    std::clamp(double(off.velocity),0.,1.)*127.));
            } else continue;
            if(channel<0||channel>15||pitch<0||pitch>127)continue;
            note=static_cast<std::uint8_t>(pitch);
            const auto when=absoluteSamplePosition_+
                static_cast<std::uint64_t>(std::clamp(offset,0,data.numSamples));
            (void)engine.sendMidi({when,static_cast<std::uint8_t>(status|channel),
                                   note,velocity,3});
        }
    }
    if(data.outputs&&data.numOutputs>0&&data.outputs[0].channelBuffers32&&
       data.outputs[0].numChannels>=2&&data.numSamples>0){
        auto* left=data.outputs[0].channelBuffers32[0];
        auto* right=data.outputs[0].channelBuffers32[1];
        if(left&&right){
            // Initial version: the original DOSBox mixer runs at 44100 Hz.
            // setupProcessing rejects other rates until verified resampling.
            engine.readAudio(left,right,static_cast<std::size_t>(data.numSamples));
            const float gain=master_.load(std::memory_order_acquire);
            for(int32 i=0;i<data.numSamples;i++){
                left[i]*=gain;right[i]*=gain;
            }
        }
    }
    if(data.numSamples>0)
        absoluteSamplePosition_+=static_cast<std::uint64_t>(data.numSamples);
    return kResultOk;
}
tresult PLUGIN_API DosBoxVst3Shell::setParamNormalized(ParamID id,ParamValue value) {
    if(!std::isfinite(value)||value<0.||value>1.)return kInvalidArgument;
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
    return kResultOk;
}
tresult PLUGIN_API DosBoxVst3Shell::getState(IBStream* stream) {
    if(!stream)return kInvalidArgument;
    std::array<std::uint8_t,kStateLength> image{};
    for(unsigned i=0;i<4;i++)image[i]=kSignature[i];
    const auto selected=requestedSoundFont();
    image[4]=selected.empty()?1:2; // v2 adds length-prefixed UTF-8 source path
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
    if(selected.empty())return kResultOk;
    const auto n=static_cast<std::uint32_t>(selected.size());
    std::array<std::uint8_t,4> length{std::uint8_t(n),std::uint8_t(n>>8),std::uint8_t(n>>16),std::uint8_t(n>>24)};
    if(stream->write(length.data(),4,&written)!=kResultOk||written!=4)return kResultFalse;
    return stream->write(const_cast<char*>(selected.data()),static_cast<int32>(n),&written)==kResultOk &&
           written==static_cast<int32>(n)?kResultOk:kResultFalse;
}
tresult PLUGIN_API DosBoxVst3Shell::setState(IBStream* stream) {
    if(!stream)return kInvalidArgument;
    std::array<std::uint8_t,kStateLength> image{};
    int32 read=0;
    if(stream->read(image.data(),static_cast<int32>(image.size()),&read)!=kResultOk||
       read!=static_cast<int32>(image.size())||
       !std::equal(kSignature.begin(),kSignature.end(),image.begin())||
       (image[4]!=1&&image[4]!=2)||image[9]!=0x5a||image[10]!=0xa5||
       image[5]>127||image[6]>127)return kResultFalse;
    const unsigned volume=image[7]|(static_cast<unsigned>(image[8])<<8);
    if(volume>10000)return kResultFalse;
    std::string selected;
    if(image[4]==2){
        std::array<std::uint8_t,4> length{};
        if(stream->read(length.data(),4,&read)!=kResultOk||read!=4)return kResultFalse;
        const auto n=std::uint32_t(length[0])|(std::uint32_t(length[1])<<8)|
                     (std::uint32_t(length[2])<<16)|(std::uint32_t(length[3])<<24);
        if(n==0||n>32768)return kResultFalse;
        selected.resize(n);
        if(stream->read(selected.data(),static_cast<int32>(n),&read)!=kResultOk||
           read!=static_cast<int32>(n))return kResultFalse;
    }
    if(!requestSoundFont(selected))return kResultFalse;
    (void)setParamNormalized(kBank,static_cast<double>(image[5])/127.);
    (void)setParamNormalized(kProgram,static_cast<double>(image[6])/127.);
    (void)setParamNormalized(kMaster,static_cast<double>(volume)/10000.);
    return kResultOk;
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
            return s==2 ? std::string("DOSBOX-X: EMULATOR AKTIV, CREATIVE-STATUS UNGEPRUEFT") :
                   s==1 ? std::string("DOSBOX-X: ORIGINALER CREATIVE-DOS-GAST STARTET") :
                   s==-1 ? std::string("DOSBOX-X: ORIGINAL-DATEIEN ODER DLL FEHLEN") :
                   s==-2 ? std::string("DOSBOX-X: GAST IST BEENDET") :
                           std::string("DOSBOX-X: GAST NICHT GESTARTET");
        },
        [this](){
            const int s=guestStatus_.load(std::memory_order_acquire);
            return s==2 ? std::string("ORIGINAL DOSBOX-X AKTIV | CREATIVE NICHT VERIFIZIERT") :
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
