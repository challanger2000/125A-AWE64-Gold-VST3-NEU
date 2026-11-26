#include "DosBoxVst3Shell.hpp"
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/vsttypes.h"
using namespace Steinberg;
using namespace Steinberg::Vst;
// Separate development-only class identity so a nonfunctional shell cannot
// overwrite any previous owner installation or silently recall old projects.
BEGIN_FACTORY_DEF("125A Systems","","")
    DEF_CLASS2(INLINE_UID(0x125A8600,0xF0074AEE,0xB64A0000,0x00000D01),
        PClassInfo::kManyInstances,kVstAudioEffectClass,
        "125A AWE64 Gold",0,
        PlugType::kInstrumentSynth,"0.0.0-dev",kVstVersionString,
        awe64::DosBoxVst3Shell::createInstance)
END_FACTORY
