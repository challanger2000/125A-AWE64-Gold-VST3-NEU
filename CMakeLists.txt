cmake_minimum_required(VERSION 3.25)
project(AWE64_Gold_DOSBoxX VERSION 0.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

# Transport tests are portable and do not pretend that DOSBox-X is connected.
if(BUILD_TESTING)
  find_package(Threads REQUIRED)
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

  # The DEV shell intentionally cannot activate without a real DOS guest.
  # Its validator FAIL is expected; compile/package it but never mark it as
  # Steinberg-validated or publish it as a usable instrument.
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
  set(AWE64_GUI_COMPAT_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/PresetBrowserViewSdkCompat.cpp")
  file(WRITE "${AWE64_GUI_COMPAT_SOURCE}" "${AWE64_GUI_SRC}")

  smtg_add_vst3plugin(AWE64GoldDOSBoxDev
    src/DosBoxVst3Shell.hpp
    src/DosBoxVst3Shell.cpp
    src/DosBoxVst3Factory.cpp
    src/RealtimeQueues.hpp
    gui/PresetBrowserView.h "${AWE64_GUI_COMPAT_SOURCE}"
    gui/PresetCatalog.h gui/PresetBrowserModel.h gui/IbkBank.h
    "${AWE64_VST3SDK}/public.sdk/source/vst/vstsinglecomponenteffect.cpp"
    "${AWE64_VST3SDK}/public.sdk/source/main/dllmain.cpp")
  target_include_directories(AWE64GoldDOSBoxDev PRIVATE
    "${AWE64_VST3SDK}" "${CMAKE_CURRENT_SOURCE_DIR}/gui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_link_libraries(AWE64GoldDOSBoxDev PRIVATE sdk vstgui_support comdlg32)
  target_compile_features(AWE64GoldDOSBoxDev PRIVATE cxx_std_20)
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
