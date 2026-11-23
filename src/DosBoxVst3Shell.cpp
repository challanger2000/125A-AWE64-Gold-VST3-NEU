#include "DosBoxVst3Shell.hpp"
#include "PresetBrowserView.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/vsttypes.h"
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

tresult PLUGIN_API DosBoxVst3Shell::initialize(FUnknown* context) {
    if(SingleComponentEffect::initialize(context)!=kResultOk)return kResultFalse;
    addEventInput(STR16("Creative MIDI (16 channels)"),16);
    addAudioOutput(STR16("AWE64 Stereo (not yet connected)"),SpeakerArr::kStereo);
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
    originalMidiCatalog_.SetOriginalCreativeMidiNumbers();
    return kResultOk;
}
tresult PLUGIN_API DosBoxVst3Shell::setActive(TBool state) {
    if(state) {
        // Guardrail: not a functioning plugin until original DOSBox-X and
        // Creative DOS execution are REALLY implemented.
        return kResultFalse;
    }
    return SingleComponentEffect::setActive(false);
}
tresult PLUGIN_API DosBoxVst3Shell::canProcessSampleSize(int32 size) {
    return size==kSample32?kResultTrue:kResultFalse;
}
tresult PLUGIN_API DosBoxVst3Shell::setProcessing(TBool) {return kResultOk;}
tresult PLUGIN_API DosBoxVst3Shell::process(ProcessData& data) {
    // This method is NOT a synthesizer. It cannot claim AWE/EMU8000 audio.
    // Do not submit this development binary for end-user installation.
    if(data.outputs)for(int32 i=0;i<data.numOutputs;i++){
        const auto& out=data.outputs[i];
        if(!out.channelBuffers32)continue;
        for(int32 ch=0;ch<out.numChannels;ch++)
            if(out.channelBuffers32[ch] && data.numSamples>0)
                std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
    }
    return kResultFalse;
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
        [](){return std::string("DOSBOX-X: ORIGINALER CREATIVE-DOS-TREIBER NICHT GESTARTET");},
        [](){return std::string("ENTWICKLUNGSBUILD | AUDIOENGINE FEHLT");},
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
