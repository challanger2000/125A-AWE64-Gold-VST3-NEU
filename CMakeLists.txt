cmake_minimum_required(VERSION 3.25)
project(AWE64_Gold_DOSBoxX VERSION 0.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

# Transport tests are portable and do not pretend that DOSBox-X is connected.
if(BUILD_TESTING)
  find_package(Threads REQUIRED)
  add_executable(awe64_creative_sysex_repair_test tests/CreativeSysexRepairTest.cpp)
  target_include_directories(awe64_creative_sysex_repair_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME AWE64_CreativeSysexRepair COMMAND awe64_creative_sysex_repair_test)
  add_executable(awe64_host_rate_converter_test tests/HostRateConverterTest.cpp)
  target_include_directories(awe64_host_rate_converter_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME AWE64_HostRateConverter COMMAND awe64_host_rate_converter_test)
  add_executable(awe64_realtime_queues_test tests/RealtimeQueuesTest.cpp)
  target_include_directories(awe64_realtime_queues_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_link_libraries(awe64_realtime_queues_test PRIVATE Threads::Threads)
  add_test(NAME AWE64_RealtimeQueues COMMAND awe64_realtime_queues_test)
  add_executable(awe64_sf2_compatibility_test tests/Sf2CompatibilityTest.cpp)
  target_include_directories(awe64_sf2_compatibility_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME AWE64_Sf2Compatibility COMMAND awe64_sf2_compatibility_test)
  add_executable(awe64_creative_dos_mailbox_test tests/CreativeDosMailboxTest.cpp)
  target_include_directories(awe64_creative_dos_mailbox_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME AWE64_CreativeDosMailbox COMMAND awe64_creative_dos_mailbox_test)
  add_executable(awe64_creative_dos_dispatch_test tests/CreativeDosDispatchTest.c)
  target_include_directories(awe64_creative_dos_dispatch_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/guest")
  set_target_properties(awe64_creative_dos_dispatch_test PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
  add_test(NAME AWE64_CreativeDosDispatch COMMAND awe64_creative_dos_dispatch_test)
  add_executable(awe64_creative_midi_bank_select_test tests/CreativeMidiBankSelectTest.cpp)
  target_include_directories(awe64_creative_midi_bank_select_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME AWE64_CreativeMidiBankSelect COMMAND awe64_creative_midi_bank_select_test)
  add_executable(awe64_dosbox_host_adapter_test tests/DosBoxHostAdapterTest.cpp)
  target_include_directories(awe64_dosbox_host_adapter_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/integration")
  add_test(NAME AWE64_DosBoxHostAdapter COMMAND awe64_dosbox_host_adapter_test)
endif()

# The DEV plugin is deliberately not a product release: VST3 activation
# reports a missing DOSBox runtime until that actual embedded backend exists.
option(AWE64_BUILD_VST3_DEV "Build Windows VST3 UI/host-integration scaffolding (no guest runtime)" OFF)
if(AWE64_BUILD_VST3_DEV)
  if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Windows x64 is required for the AWE64 VST3.")
  endif()
  set(AWE64_VST3SDK "" CACHE PATH "Absolute path to pinned Steinberg VST3 SDK 3.7.12")
  if(NOT EXISTS "${AWE64_VST3SDK}/public.sdk/source/vst/vstsinglecomponenteffect.cpp")
    message(FATAL_ERROR "Steinberg VST3 SDK 3.7.12 not found: ${AWE64_VST3SDK}")
  endif()
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gui/resource/125A_P165_Original_Faceplate_1000x708.png")
    message(FATAL_ERROR "Original P165 frontplate missing; refuse substitution.")
  endif()
  # Never fetch arbitrary artwork during builds. Verify locally committed blob.
  execute_process(
    COMMAND git hash-object "gui/resource/125A_P165_Original_Faceplate_1000x708.png"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE AWE64_FACEPLATE_BLOB
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE AWE64_FACEPLATE_GIT_RESULT)
  if(NOT AWE64_FACEPLATE_GIT_RESULT EQUAL 0 OR
     NOT AWE64_FACEPLATE_BLOB STREQUAL "7e57ef8e0c90d84fa4c951bea7815b0233762ca7")
    message(FATAL_ERROR "Original owner-approved faceplate hash mismatch.")
  endif()

  # Do not run the validator automatically during link: the DOSBox DLL and
  # synthetic guest assets are staged afterwards. CI explicitly runs the
  # full pinned official SDK validator on the actual staged VST3.
  set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
  set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
  add_subdirectory("${AWE64_VST3SDK}" "${CMAKE_BINARY_DIR}/vst3sdk")

  # Preserve byte-identical owner GUI in Git. Old SDK VSTGUI 4 lacks
  # getVisibleClientRect(), so build ONLY an ephemeral compatibility copy.
  execute_process(
    COMMAND git hash-object "gui/PresetBrowserView.cpp"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE AWE64_APPROVED_GUI_BLOB
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT AWE64_APPROVED_GUI_BLOB STREQUAL "922250ed10e864d3fc7eeabf261ff29f5cbe2040")
    message(FATAL_ERROR "Owner P007Z GUI changed unexpectedly; verify before continuing.")
  endif()
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/gui/PresetBrowserView.cpp" AWE64_GUI_SRC)
  set(AWE64_OLD_VIEWPORT "browser->getVisibleClientRect().getWidth()")
  set(AWE64_NEW_VIEWPORT "(browser->getViewSize().getWidth() - ((browser->getActiveScrollbars() & VSTGUI::CScrollView::kVerticalScrollbar) ? browser->getScrollbarWidth() : 0.))")
  string(FIND "${AWE64_GUI_SRC}" "${AWE64_OLD_VIEWPORT}" AWE64_VIEWPORT_INDEX)
  if(AWE64_VIEWPORT_INDEX EQUAL -1)
    message(FATAL_ERROR "Owner GUI viewport compat anchor missing.")
  endif()
  string(REPLACE "${AWE64_OLD_VIEWPORT}" "${AWE64_NEW_VIEWPORT}"
    AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  # Original P165 GUI source stays byte-identical in Git. Fix only legacy
  # P007 Win95/86Box messages in the compiler's temporary compatibility copy.
  string(FIND "${AWE64_GUI_SRC}" "Creative-Windows-95-SoundFont-Laden" AWE64_OLD_W95)
  string(FIND "${AWE64_GUI_SRC}" "automatisch durch 86Box" AWE64_OLD_86BOX)
  if(AWE64_OLD_W95 EQUAL -1 OR AWE64_OLD_86BOX EQUAL -1)
    message(FATAL_ERROR "Original P165 Win95/86Box dialog anchors not found")
  endif()
  # Only replace the original nonfunctional dialog body, not P165 graphics.
  string(FIND "${AWE64_GUI_SRC}" "void PresetBrowserView::ChooseSf2() {" AWE64_SF2_BEGIN)
  string(FIND "${AWE64_GUI_SRC}" "void PresetBrowserView::ChooseRom() {" AWE64_SF2_END)
  if(AWE64_SF2_BEGIN LESS 0 OR AWE64_SF2_END LESS AWE64_SF2_BEGIN)
    message(FATAL_ERROR "Approved SF2 dialog anchors missing")
  endif()
  string(SUBSTRING "${AWE64_GUI_SRC}" 0 ${AWE64_SF2_BEGIN} AWE64_GUI_BEFORE)
  string(SUBSTRING "${AWE64_GUI_SRC}" ${AWE64_SF2_END} -1 AWE64_GUI_AFTER)
  set(AWE64_SF2_DIALOG [=[
void PresetBrowserView::ChooseSf2() {
#if defined(_WIN32)
    wchar_t filename[32768]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize=sizeof(dialog);
    dialog.hwndOwner=GetForegroundWindow();
    dialog.lpstrFilter=L"SoundFont (*.sf2)\0*.sf2\0\0";
    dialog.lpstrFile=filename;
    dialog.nMaxFile=32768;
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    if(!GetOpenFileNameW(&dialog))return;
    const int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,filename,-1,nullptr,0,nullptr,nullptr);
    if(n<=1)return;
    std::string path(static_cast<size_t>(n), '\0');
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,filename,-1,path.data(),n,nullptr,nullptr))return;
    path.pop_back();
    if(!sf2LoadRequest_||!sf2LoadRequest_(path)) {
        MessageBoxW(dialog.hwndOwner,L"SoundFont ungueltig oder vom Creative-DOS-Format nicht unterstuetzt.",
            L"125A AWE64 Gold",MB_OK|MB_ICONERROR);
        return;
    }
    lastStatus_.clear();SyncSf2Status();
#endif
}
]=])
  set(AWE64_GUI_SRC "${AWE64_GUI_BEFORE}${AWE64_SF2_DIALOG}${AWE64_GUI_AFTER}")
  string(REPLACE [=[L"Das originale AWE-ROM liegt bereits im privaten VST3 und wird "]=]
      [=[L"Das AWE-Wave-ROM muss im privaten VST3-Ordner liegen. "]=]
      AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  string(REPLACE [=[L"automatisch durch 86Box verwendet. Ein Austausch des ROMs "]=]
      [=[L"Es wird beim DOSBox-X-Start geladen. Ein ROM-Wechsel "]=]
      AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  string(REPLACE [=[L"im laufenden Windows-95-Gast ist noch nicht implementiert."]=]
      [=[L"waehrend des DOS-Gastbetriebs ist noch nicht implementiert."]=]
      AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  # ROM button restores the original ROM-backed GM bank via the same
  # non-RT guest restart, without changing the approved faceplate.
  string(FIND "${AWE64_GUI_SRC}" "void PresetBrowserView::ChooseRom() {" AWE64_ROM_BEGIN)
  string(FIND "${AWE64_GUI_SRC}" "void PresetBrowserView::valueChanged(" AWE64_ROM_END)
  if(AWE64_ROM_BEGIN LESS 0 OR AWE64_ROM_END LESS AWE64_ROM_BEGIN)
    message(FATAL_ERROR "Approved ROM dialog anchors missing")
  endif()
  string(SUBSTRING "${AWE64_GUI_SRC}" 0 ${AWE64_ROM_BEGIN} AWE64_GUI_BEFORE)
  string(SUBSTRING "${AWE64_GUI_SRC}" ${AWE64_ROM_END} -1 AWE64_GUI_AFTER)
  set(AWE64_ROM_DIALOG [=[
void PresetBrowserView::ChooseRom() {
#if defined(_WIN32)
    if(!romLoadRequest_||!romLoadRequest_(std::string{}))
        MessageBoxW(GetForegroundWindow(),L"Originale ROM-Bank konnte nicht gestartet werden.",
            L"125A AWE64 Gold",MB_OK|MB_ICONERROR);
    lastStatus_.clear();SyncSf2Status();
#endif
}
]=])
  set(AWE64_GUI_SRC "${AWE64_GUI_BEFORE}${AWE64_ROM_DIALOG}${AWE64_GUI_AFTER}")
  # Original effect-type menus now route documented SysEx through the
  # unchanged Creative guest. No modifications to the approved artwork.
  string(REPLACE "reverbMenu_->setMouseEnabled(false)" "reverbMenu_->setMouseEnabled(true)" AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  string(REPLACE "chorusMenu_->setMouseEnabled(false)" "chorusMenu_->setMouseEnabled(true)" AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  string(REPLACE "    // Retain the disabled historic presets without claiming DAW recall." [=[
    if(reverbMenu_)reverbMenu_->setValue(static_cast<float>(std::lround(controller_->getParamNormalized(kReverbType)*7.)));
    if(chorusMenu_)chorusMenu_->setValue(static_cast<float>(std::lround(controller_->getParamNormalized(kChorusType)*7.)));
]=] AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  # Two original send knobs now control real EMU8000 voice-send registers.
  string(REPLACE "for(auto* knob:controls_)if(knob)knob->setMouseEnabled(false);" "for(auto* knob:controls_)if(knob)knob->setMouseEnabled(false);\n    if(controls_[10])controls_[10]->setMouseEnabled(true);\n    if(controls_[11])controls_[11]->setMouseEnabled(true);" AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  string(REPLACE [=[values_[size_t(i)]->setText("--"); // original Creative bridge unimplemented]=] [=[values_[size_t(i)]->setText(i>=10?label:"--");]=] AWE64_GUI_SRC "${AWE64_GUI_SRC}")
  set(AWE64_GUI_COMPAT_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/PresetBrowserViewSdkCompat.cpp")
  file(WRITE "${AWE64_GUI_COMPAT_SOURCE}" "${AWE64_GUI_SRC}")

  smtg_add_vst3plugin(AWE64GoldDOSBoxDev
    src/DosBoxVst3Shell.hpp
    src/DosBoxVst3Shell.cpp
    src/DosBoxVst3Factory.cpp
    src/RealtimeQueues.hpp
    src/CreativeMidiBankSelect.hpp
    src/DosBoxRuntimeHost.hpp
    integration/DosBoxHostAdapter.hpp
    integration/AWE64_DOSBOX_ABI.h
    src/CreativeDosMailbox.hpp
    gui/PresetBrowserView.h "${AWE64_GUI_COMPAT_SOURCE}"
    gui/PresetCatalog.h gui/PresetBrowserModel.h gui/IbkBank.h
    "${AWE64_VST3SDK}/public.sdk/source/vst/vstsinglecomponenteffect.cpp"
    "${AWE64_VST3SDK}/public.sdk/source/main/dllmain.cpp")
  target_include_directories(AWE64GoldDOSBoxDev PRIVATE
    "${AWE64_VST3SDK}" "${CMAKE_CURRENT_SOURCE_DIR}/gui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration")
  target_link_libraries(AWE64GoldDOSBoxDev PRIVATE sdk vstgui_support comdlg32 bcrypt)
  target_compile_features(AWE64GoldDOSBoxDev PRIVATE cxx_std_20)
  # The real Steinberg factory/process() is exercised on Windows with a
  # PUBLIC synthetic DOS executable: NO original Creative assets in CI.
  # This is a transport gate, never a substitute EMU8000/Creative synth.
  add_executable(awe64_vst3_host_pcm_smoke tests/WindowsVst3HostPcmSmoke.cpp
    "${AWE64_VST3SDK}/public.sdk/source/common/memorystream.cpp")
  target_include_directories(awe64_vst3_host_pcm_smoke PRIVATE "${AWE64_VST3SDK}")
  target_link_libraries(awe64_vst3_host_pcm_smoke PRIVATE sdk base user32 ole32)
  target_compile_features(awe64_vst3_host_pcm_smoke PRIVATE cxx_std_20)

  smtg_target_configure_version_file(AWE64GoldDOSBoxDev)
  smtg_target_add_plugin_resources(AWE64GoldDOSBoxDev RESOURCES
    gui/resource/p007_p165.uidesc
    gui/resource/125A_P165_Original_Faceplate_1000x708.png
    gui/resource/125A_Button_Atlas_Hardware_Normal_Pressed.png
    gui/resource/125A_Knob_Standard_33frames_58px.png
    gui/resource/125A_Knob_Master_33frames_134px.png)
  # Preserve the original hardware sprites for the VSTGUI CBitmap loader.
  set(AWE64_RESOURCE_RC "${CMAKE_CURRENT_BINARY_DIR}/awe64_p165_assets.rc")
  file(WRITE "${AWE64_RESOURCE_RC}" "")
  foreach(AWE64_ASSET IN ITEMS
      "125A_P165_Original_Faceplate_1000x708.png"
      "125A_Button_Atlas_Hardware_Normal_Pressed.png"
      "125A_Knob_Standard_33frames_58px.png"
      "125A_Knob_Master_33frames_134px.png")
    file(APPEND "${AWE64_RESOURCE_RC}"
      "\"${AWE64_ASSET}\" PNG DISCARDABLE \"${CMAKE_CURRENT_SOURCE_DIR}/gui/resource/${AWE64_ASSET}\"\n")
  endforeach()
  target_sources(AWE64GoldDOSBoxDev PRIVATE "${AWE64_RESOURCE_RC}")
endif()
