#include "DosBoxVst3Shell.hpp"
#include "CreativeMidiBankSelect.hpp"
#include "CreativeMidiControllers.hpp"
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
    return kResultOk;
}
tresult PLUGIN_API DosBoxVst3Shell::setActive(TBool state) {
    if(state) {
        if(runtime_)return kResultOk;
        // Host activation is non-RT: check the authentic private Creative
        // executable, private ROM, valid SF2 and compiled DOSBox-X DLL.
        // Never substitute a synthetic renderer or silently ignore assets.
        try {
            auto engine=std::make_unique<integration::DosBoxRuntimeHost>();
            const auto ownBinary=engine->pluginBinaryPath();
            if(ownBinary.empty() || !engine->begin(
                ownBinary,integration::DosBoxRuntimeHost::privateAssetsDir(ownBinary))) {
                guestStatus_.store(-1,std::memory_order_release);
                return kResultFalse;
            }
            runtime_=std::move(engine);
            guestStatus_.store(1,std::memory_order_release);
            initialProgramSent_=false;
            absoluteSamplePosition_=0;
        } catch (...) {
            guestStatus_.store(-1,std::memory_order_release);
            return kResultFalse;
        }
        return SingleComponentEffect::setActive(true);
    }
    if(runtime_) {
        runtime_->stop();
        runtime_.reset();
    }
    guestStatus_.store(0,std::memory_order_release);
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
    if(!runtime_)return kResultFalse;
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
    image[4]=1; // format version 1
    image[5]=bank_.load(std::memory_order_acquire);
    image[6]=program_.load(std::memory_order_acquire);
    const auto level=static_cast<std::uint32_t>(std::lround(
        std::clamp(master_.load(std::memory_order_acquire),0.f,1.f)*10000.));
    image[7]=static_cast<std::uint8_t>(level&255u);
    image[8]=static_cast<std::uint8_t>((level>>8)&255u);
    image[9]=0x5a; image[10]=0xa5;
    int32 written=0;
    return stream->write(image.data(),static_cast<int32>(image.size()),&written)==kResultOk
        &&written==static_cast<int32>(image.size())?kResultOk:kResultFalse;
}
tresult PLUGIN_API DosBoxVst3Shell::setState(IBStream* stream) {
    if(!stream)return kInvalidArgument;
    std::array<std::uint8_t,kStateLength> image{};
    int32 read=0;
    if(stream->read(image.data(),static_cast<int32>(image.size()),&read)!=kResultOk||
       read!=static_cast<int32>(image.size())||
       !std::equal(kSignature.begin(),kSignature.end(),image.begin())||
       image[4]!=1||image[9]!=0x5a||image[10]!=0xa5||
       image[5]>127||image[6]>127)return kResultFalse;
    const unsigned volume=image[7]|(static_cast<unsigned>(image[8])<<8);
    if(volume>10000)return kResultFalse;
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
    return new AWE125::PresetBrowserView({0,0,1000,708},this,
        originalMidiCatalog_,unusedFmCatalog_,std::string{},
        [this](){
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
        [](const std::string&){return false;},
        [](const std::string&){return false;},
        [](){return std::string{};},
        [](){return std::uint64_t{1};},
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
