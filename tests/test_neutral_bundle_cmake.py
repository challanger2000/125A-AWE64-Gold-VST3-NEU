"""Actual CMake regression for resources and moduleinfo argument boundaries.
Uses only a disposable fake module/tool; not a native VST3 or DSP proof.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="awe64 path spaces ") as tmp:
    source = Path(tmp) / "source dir"
    build = Path(tmp) / "build dir"
    source.mkdir()
    shutil.copyfile(root / "cmake/NeutralBundle.cmake", source / "NeutralBundle.cmake")
    (source / "CMakeLists.txt").write_text(r'''
cmake_minimum_required(VERSION 3.25)
project(QuoteProbe VERSION 0.0.0 LANGUAGES CXX)
include(NeutralBundle.cmake)
add_executable(moduleinfotool tool.cpp)
target_compile_features(moduleinfotool PRIVATE cxx_std_17)
add_library(plugin MODULE dummy.cpp)
set_target_properties(plugin PROPERTIES SMTG_PLUGIN_PACKAGE_PATH "${CMAKE_BINARY_DIR}/VST3/$<CONFIG>/125A AWE64 Gold.vst3")
awe64_stage_bundle_resources(plugin "${CMAKE_CURRENT_SOURCE_DIR}/original faceplate.png" "${CMAKE_CURRENT_SOURCE_DIR}/original.uidesc")
awe64_write_module_info(plugin)
''', encoding="utf-8")
    (source / "tool.cpp").write_text(r'''
#include <filesystem>
#include <fstream>
#include <string>
int main(int argc,char** argv){
  if(argc!=8 || std::string(argv[1])!="-create" || std::string(argv[2])!="-version" || std::string(argv[3])!="0.0.0" || std::string(argv[4])!="-path" || std::string(argv[6])!="-output") return 1;
  const std::filesystem::path p(argv[5]);
  if(p.filename()!="125A AWE64 Gold.vst3" ||
     p/"Contents/Resources/moduleinfo.json"!=std::filesystem::path(argv[7]) ||
     !std::filesystem::is_regular_file(p/"Contents/Resources/original faceplate.png")) return 2;
  std::ofstream out(argv[7]);out<<"fixture";return !out;
}
''', encoding="utf-8")
    (source / "dummy.cpp").write_text("int placeholder(){return 0;}\n", encoding="utf-8")
    assets = {"original faceplate.png": b"dummy-png", "original.uidesc": b"dummy-uidesc"}
    for name, data in assets.items():
        (source / name).write_bytes(data)
    subprocess.run(["cmake", "-S", str(source), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release"], check=True)
    subprocess.run(["cmake", "--build", str(build), "--config", "Release", "--parallel", "2"], check=True)
    resources = build / "VST3/Release/125A AWE64 Gold.vst3/Contents/Resources"
    assert (resources / "moduleinfo.json").read_text() == "fixture"
    for name, data in assets.items():
        assert (resources / name).read_bytes() == data
    # Resource-only updates must not depend on relinking the DLL.
    (source / "original.uidesc").write_bytes(b"changed resource")
    subprocess.run(["cmake", "--build", str(build), "--config", "Release", "--parallel", "2"], check=True)
    assert (resources / "original.uidesc").read_bytes() == b"changed resource"
    assert sorted(p.name for p in (build / "VST3/Release").iterdir()) == ["125A AWE64 Gold.vst3"]
print("NEUTRAL_BUNDLE_SPACE_PATHS_PASS")
