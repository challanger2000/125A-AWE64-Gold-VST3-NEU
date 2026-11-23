#pragma once
// PRE-ENGINE development component: loadable VST3 shell and exact P165 GUI.
// NO DOSBOX guest has been connected, so activation MUST fail (not fake sound).
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "PresetCatalog.h"
#include "RealtimeQueues.hpp"
#include <atomic>
#include <cstdint>

namespace awe64 {
class DosBoxVst3Shell final : public Steinberg::Vst::SingleComponentEffect,
                              public VSTGUI::VST3EditorDelegate {
    AWE125::PresetCatalog originalMidiCatalog_{};
    AWE125::PresetCatalog unusedFmCatalog_{};
    rt::RealtimeQueues queues_{};
    std::atomic<std::uint8_t> program_{0};
    std::atomic<std::uint8_t> bank_{0};
    std::atomic<float> master_{1.f};
public:
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new DosBoxVst3Shell());
    }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
    Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
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
