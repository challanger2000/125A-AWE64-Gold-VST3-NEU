#pragma once
// PRE-ENGINE development component: loadable VST3 shell and exact P165 GUI.
// NO DOSBOX guest has been connected, so activation MUST fail (not fake sound).
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "PresetCatalog.h"
#include "DosBoxRuntimeHost.hpp"
#include "RealtimeQueues.hpp"
#include "HostRateConverter.hpp"
#include "HostDemandSession.hpp"
#include "MasterGainPlan.hpp"
#include <memory>
#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <string>

namespace awe64 {
class DosBoxVst3Shell final : public Steinberg::Vst::SingleComponentEffect,
                              public VSTGUI::VST3EditorDelegate,
                              public Steinberg::Vst::IMidiMapping {
    AWE125::PresetCatalog originalMidiCatalog_{};
    AWE125::PresetCatalog unusedFmCatalog_{};
    // The worker owns all true original DOSBox-X guest audio/MIDI queues.
    // Constructed on the non-real-time VST3 activation thread.
    std::shared_ptr<rt::LegacyCaptureGate> captureGate_=std::make_shared<rt::LegacyCaptureGate>();
    std::atomic<bool> legacyProcessing_{true};
    std::unique_ptr<integration::DosBoxRuntimeHost> runtime_;
    std::atomic<int> guestStatus_{0};
    std::uint64_t absoluteSamplePosition_=0;
    bool initialProgramSent_=false;
    std::uint8_t lastBank_=0,lastProgram_=0;
    std::atomic<std::uint8_t> program_{0};
    std::atomic<std::uint8_t> bank_{0};
    std::atomic<float> master_{1.f};
    std::atomic<std::uint8_t> reverbType_{4},chorusType_{2};
    std::atomic<std::uint8_t> reverbSend_{0},chorusSend_{0};
    std::array<std::atomic<std::uint8_t>,10> voiceControls_{};
    std::uint8_t lastReverbType_=4,lastChorusType_=2;
    rt::HostRateConverter rateConverter_{};
    rt::HostDemandSession hostDemand_{};
    rt::MasterGainPlan legacyMaster_{};
    // Experimental clock path only; native acceptance required before default.
    bool demandRequested_=false;
    std::atomic<unsigned> latencySamples_{0};
    std::atomic<bool> demandActive_{false},timedProcessing_{false};
    Steinberg::tresult processTimed(Steinberg::Vst::ProcessData&,integration::DosBoxRuntimeHost&);
    // Lifecycle changes on the host/editor thread. The audio callback uses
    // try_lock, never waits for the emulator or a disk operation.
    // Serialize non-RT lifecycle changes even while a replacement boots
    // outside runtimeMutex_. The audio thread never touches this mutex.
    std::mutex transitionMutex_;
    std::mutex runtimeMutex_;
    std::mutex selectionMutex_;
    AWE125::PresetCatalog startedCatalog_{};
    std::uint64_t displayedRevision_=0;
    bool startRuntimeLocked(std::unique_lock<std::mutex>&, const std::string&, const std::vector<std::uint8_t>* = nullptr,
                            AWE125::PresetCatalog* = nullptr);
    std::vector<std::uint8_t> requestedBankBytes_; // protected by runtimeMutex_
    std::uint64_t refreshBrowserCatalog();
    void stopRuntimeLocked() noexcept;
    std::string requestedBank_;
    std::string startedBank_;
    std::atomic<std::uint64_t> catalogRevision_{1};
    bool loadCatalog(const std::string&,AWE125::PresetCatalog&,
                     const std::vector<std::uint8_t>* = nullptr) const;
    void publishCatalogLocked(AWE125::PresetCatalog&&);
    bool requestSoundFont(const std::string&,int expectedProgram=-1);
    std::string requestedSoundFont();
    std::string soundFontStatus();
public:
    ~DosBoxVst3Shell() override = default;
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new DosBoxVst3Shell());
    }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,void** obj) override;
    Steinberg::uint32 PLUGIN_API addRef() override {return SingleComponentEffect::addRef();}
    Steinberg::uint32 PLUGIN_API release() override {return SingleComponentEffect::release();}
    Steinberg::tresult PLUGIN_API getMidiControllerAssignment(
        Steinberg::int32 busIndex,Steinberg::int16 channel,
        Steinberg::Vst::CtrlNumber number,Steinberg::Vst::ParamID& id) override;
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
    Steinberg::int32 PLUGIN_API getBusCount(Steinberg::Vst::MediaType,Steinberg::Vst::BusDirection) override;
    Steinberg::tresult PLUGIN_API getBusInfo(Steinberg::Vst::MediaType,Steinberg::Vst::BusDirection,
        Steinberg::int32,Steinberg::Vst::BusInfo&) override;
    Steinberg::tresult PLUGIN_API activateBus(Steinberg::Vst::MediaType,Steinberg::Vst::BusDirection,
        Steinberg::int32,Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API getBusArrangement(Steinberg::Vst::BusDirection,
        Steinberg::int32,Steinberg::Vst::SpeakerArrangement&) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*,Steinberg::int32,
        Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
    Steinberg::uint32 PLUGIN_API getLatencySamples() override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
    Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID) override;
    Steinberg::tresult PLUGIN_API setParamNormalized(
        Steinberg::Vst::ParamID,Steinberg::Vst::ParamValue) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) override;
    VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr,
        const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
        VSTGUI::VST3Editor*) override;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString) override;
};
} // namespace awe64
