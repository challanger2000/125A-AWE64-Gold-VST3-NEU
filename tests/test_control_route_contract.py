"""Portable control-routing gate using product C++ fragments, not a second synth."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PATCHER = ROOT / "integration" / "patch_dosbox_runtime.py"


def between(text, start, end):
    a = text.index(start)
    b = text.index(end, a)
    return text[a:b] + "\n"


def require(condition, label):
    if not condition:
        raise RuntimeError(label)


def check_gui_host_routes():
    gui = (ROOT / "gui/PresetBrowserView.cpp").read_text(encoding="utf-8")
    header = (ROOT / "gui/PresetBrowserView.h").read_text(encoding="utf-8")
    shell = (ROOT / "src/DosBoxVst3Shell.cpp").read_text(encoding="utf-8")
    timed = (ROOT / "src/HostEventPlan.hpp").read_text(encoding="utf-8")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    expected = ["ATTACK", "DECAY", "SUSTAIN", "RELEASE", "ATTACK", "DECAY",
                "SUSTAIN", "RELEASE", "CUTOFF", "RESONANCE", "SEND", "SEND"]
    knobs = re.findall(r'AddKnob\((\d+),\d+,\d+,"([^"]+)"\);', gui)
    require(sorted((int(i), label) for i, label in knobs) == list(enumerate(expected)),
            "physical knob index/label mismatch")
    labels = re.findall(r'"([^"]+)"', between(shell, "kControlLabels{", "};"))
    require(labels == ["AMP ATTACK", "AMP DECAY", "AMP SUSTAIN", "AMP RELEASE",
                       "FILTER ATTACK", "FILTER DECAY", "FILTER SUSTAIN", "FILTER RELEASE",
                       "CUTOFF", "RESONANCE", "REVERB SEND", "CHORUS SEND"], "VST3 labels")
    for text, anchors in [
        (header, ["kGuiFirst=1100,kGuiCount=12", "kMasterVolume=1001",
                  "kReverbType=1200,kChorusType=1201"]),
        (gui, ["AddKnob(10,74,278", "AddKnob(11,258,278", "this,kGuiFirst+index",
               "lastY_-where.y", "where.x-lastX_",
               "for(int i=0;i<kGuiCount;++i)ChangeParameter(kGuiFirst+i,0.);",
               "masterVolume_->setDefaultValue(1.f);"]),
        (cmake, ["for(auto* knob:controls_)if(knob)knob->setMouseEnabled(true);",
                 "if(reset_)reset_->setMouseEnabled(true);",
                 "values_[size_t(i)]->setText(label);",
                 "reverbMenu_->setMouseEnabled(true)", "chorusMenu_->setMouseEnabled(true)"]),
        (shell, ["1100u+static_cast<ParamID>(i)", "kReverbSend=1110u,kChorusSend=1111u",
                 "voiceControls_[i].store(static_cast<std::uint8_t>(std::lround(values[i+1]*255.))",
                 "if(present[11])reverbSend_", "if(present[12])chorusSend_",
                 "lo|=std::uint64_t(voiceControls_[i].load(std::memory_order_acquire))<<(i*8)",
                 "voiceControls_[8].load(std::memory_order_acquire)|(unsigned(voiceControls_[9].load(std::memory_order_acquire))<<8)",
                 "engine.setAbsoluteSends(reverbSend_.load(std::memory_order_acquire),chorusSend_.load(std::memory_order_acquire))",
                 "makeCreativeEffectType(absoluteSamplePosition_,false,reverb)",
                 "makeCreativeEffectType(absoluteSamplePosition_,true,chorus)",
                 "recall->sends[0]/255.", "recall->sends[1]/255.", "recall->voice[i]/255.",
                 "legacyMaster_.apply(left,right)"]),
        (timed, ["state.voice[id-1100]", "id==1110)state.reverbSend", "id==1111)state.chorusSend",
                 "id==1200)state.reverbType", "id==1201)state.chorusType",
                 "lo|=std::uint64_t(state.voice[n])<<(8*n)",
                 "unsigned hi=unsigned(state.voice[8])|(unsigned(state.voice[9])<<8)",
                 "unsigned(state.reverbSend)|(unsigned(state.chorusSend)<<8)",
                 "makeCreativeEffectType(at,false,state.reverbType)",
                 "makeCreativeEffectType(at,true,state.chorusType)"]),
    ]:
        for anchor in anchors:
            require(anchor in text, "control route missing: " + anchor)
    print("GUI_HOST_CONTROL_ROUTE_SOURCE_PASS (static, not native editor)")


def main():
    check_gui_host_routes()
    spec = importlib.util.spec_from_file_location("awe64_control_route_patcher", PATCHER)
    patch = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(patch)
    source = PATCHER.read_text(encoding="utf-8")
    voice = between(source, "    unsigned appliedSends=0x10000u;", "\n#endif\n'''+")
    fixture = (
        "#include <assert.h>\nextern bool enable_slave_pic;\n"
        "mpu.queue[pos]=data;\nret=mpu.queue[mpu.queue_pos];\n"
        "static void ClrQueue(void) {\n"
        "static Bitu MPU401_ReadStatus(Bitu port,Bitu iolen) {\n"
        "if (!MIDI_Available()) return;"
    )
    pump = between(patch.patch_mpu(fixture), "static void AWE64_PumpGuestMidi() {", "\n#endif")
    with tempfile.TemporaryDirectory(prefix="awe64-controls-") as tmp:
        tmp = Path(tmp)
        (tmp / "voice.inc").write_text(voice, encoding="utf-8")
        (tmp / "pump.inc").write_text(pump, encoding="utf-8")
        exe = tmp / "control-route"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++14", "-O2",
                        "-Wall", "-Wextra", "-Werror", "-DNDEBUG",
                        "-I" + str(tmp), "-I" + str(ROOT / "research/p165-control-port"),
                        str(ROOT / "tests/ControlRouteContract.cpp"), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
