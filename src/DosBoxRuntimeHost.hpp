#pragma once
// In-process Windows-only DOSBox-X-AWE32 guest lifecycle for the VST3 host.
// The synthesizer and MIDI engine remain the ORIGINAL DOSBox + Creative DOS.
// No external process. No ROM/SF2/Creative binary may enter public CI.
#if !defined(_WIN32)
#error Windows-only VST3 runtime adapter.
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include "CreativeSysexRepair.hpp"
#include "DosBoxHostAdapter.hpp"
#include "ExternalSoundFont.hpp"
#include "AWE64_DOSBOX_ABI.h"
#include "MidiTimeline.hpp"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <utility>

namespace awe64::integration {
class DosBoxRuntimeHost final {
    // DOSBox globals are private to each independently mapped image.
    // Use a unique DLL filename AND directory for every activation, never
    // share the in-bundle module's writable globals between components.
    HMODULE module_=nullptr;
    AWE64_RegisterMidiFn registerMidi_=nullptr;
    AWE64_RegisterPCMFn registerPCM_=nullptr;
    AWE64_StartGuestFn startGuest_=nullptr;
    AWE64_RequestStopFn requestStop_=nullptr;
    AWE64_IsGuestMidiReadyFn ready_=nullptr;
    AWE64_SetAbsoluteSendsFn sends_=nullptr;
    AWE64_SetVoiceControlsFn voiceControls_=nullptr;
    std::uint64_t lastVoiceLo_=0;
    unsigned lastVoiceHi_=0;
    AWE64_SetGuestClockPCMFn guestClockPcm_=nullptr;
    AWE64_RegisterClockFn registerClock_=nullptr;
    AWE64_GuestClockFaultFn clockFault_=nullptr;
    unsigned demandLead_=0;
    bool pcmPrimed_=false;
    unsigned lastSends_=0x10000u;
    bool authenticGuest_=false;
    // Opt-in measurement only; no console I/O or file writes on RT thread.
    bool performanceStats_=false, measurementStarted_=false;
    rt::RealtimeQueues::Snapshot measurementStart_{},measurementEnd_{};
    std::uint64_t measuredBlocks_=0,depthTotal_=0;
    std::size_t depthMin_=32768,depthMax_=0,midiDepthMax_=0;
    // Producer-only journal of dropped persistent controller values. Do not
    // resurrect sustain or notes after a fail-safe recovery.
    std::array<rt::MidiEvent,128> lostControllers_{};
    bool haveLostControllers_=false;
    static int persistentControllerSlot(const rt::MidiEvent& e) noexcept {
        if(e.command!=rt::GuestCommand::Midi||e.data1>127||e.data2>127)return -1;
        unsigned index=0;
        switch(e.status&0xf0u){
            case 0xb0: switch(e.data1){
                case 1:index=0;break;case 7:index=1;break;case 10:index=2;break;
                case 11:index=3;break;case 91:index=4;break;case 93:index=5;break;
                default:return -1;
            }break;
            case 0xd0:index=6;break;
            case 0xe0:index=7;break;
            default:return -1;
        }
        return int((e.status&15u)*8+index);
    }
    void rememberLost(const rt::MidiEvent& e) noexcept {
        const int slot=persistentControllerSlot(e);
        if(slot>=0){lostControllers_[static_cast<unsigned>(slot)]=e;haveLostControllers_=true;}
    }
    std::unique_ptr<rt::RealtimeQueues> queues_;
    std::unique_ptr<DosBoxHostAdapter> adapter_;
    std::thread worker_;
    std::vector<std::string> argvStorage_;
    std::atomic<bool> running_{false};
    std::atomic<bool> finished_{false};
    std::atomic<int> guestExitCode_{-1};
    std::filesystem::path stagedDirectory_;

    static bool isUsableFile(const std::filesystem::path& p,
                             std::uintmax_t expectedSize=0) noexcept {
        try {
            if(!std::filesystem::is_regular_file(p))return false;
            const auto n=std::filesystem::file_size(p);
            return n>0 && (expectedSize==0 || n==expectedSize);
        } catch (...) {return false;}
    }

    // Check the full original Git blob, not just the instruction anchor.
    // No external process; BCrypt is Windows' built-in cryptographic API.
    static bool authenticDemo(const std::vector<std::uint8_t>& bytes) noexcept {
        if(bytes.size()!=40197)return false;
        constexpr unsigned char expected[20]={0xe1,0x0b,0x87,0x05,0xc6,0xa4,0x64,0x44,0xa9,0x98,0xc4,0xf6,0xaa,0x74,0xa1,0xcd,0xa2,0x08,0x15,0x06};
        unsigned char prefix[]="blob 40197";
        unsigned char digest[20]{};
        BCRYPT_ALG_HANDLE alg=nullptr;
        BCRYPT_HASH_HANDLE hash=nullptr;
        if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA1_ALGORITHM,nullptr,0)<0)return false;
        bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
        if(ok)ok=BCryptHashData(hash,prefix,sizeof(prefix),0)>=0;
        if(ok)ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0;
        if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
        if(hash)BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg,0);
        return ok&&std::memcmp(digest,expected,sizeof(expected))==0;
    }
    bool makeStagingDirectory() {
        if(!stagedDirectory_.empty())return true;
        wchar_t root[MAX_PATH+1]{},name[MAX_PATH+1]{};
        const DWORD n=GetTempPathW(MAX_PATH,root);
        if(!n||n>=MAX_PATH||!GetTempFileNameW(root,L"AWE",0,name))return false;
        if(!DeleteFileW(name)||!CreateDirectoryW(name,nullptr))return false;
        stagedDirectory_=std::filesystem::path(name);
        return true;
    }

    static std::string utf8(const std::filesystem::path& p) {
        // Path encoding is delegated to the original DOSBox-X Windows port.
        return p.string();
    }

    void workerMain() noexcept {
        // argv must remain stable throughout the original DOSBox main().
        try {
            std::vector<char*> args;
            args.reserve(argvStorage_.size()+1);
            for(auto& s:argvStorage_)args.push_back(s.data());
            args.push_back(nullptr);
            running_.store(true,std::memory_order_release);
            const auto code=startGuest_(static_cast<int>(argvStorage_.size()),
                                         args.data());
            guestExitCode_.store(code,std::memory_order_release);
        } catch (...) {
            guestExitCode_.store(-2,std::memory_order_release);
        }
        running_.store(false,std::memory_order_release);
        finished_.store(true,std::memory_order_release);
    }

public:
    DosBoxRuntimeHost()=default;
    DosBoxRuntimeHost(DosBoxRuntimeHost const&)=delete;
    DosBoxRuntimeHost& operator=(DosBoxRuntimeHost const&)=delete;
    ~DosBoxRuntimeHost(){stop();}

    static std::filesystem::path pluginBinaryPath() noexcept {
        static const int addressAnchor=0;
        MEMORY_BASIC_INFORMATION mbi{};
        if(!VirtualQuery(&addressAnchor,&mbi,sizeof(mbi)))return {};
        std::vector<wchar_t> buffer(32768,0);
        DWORD n=GetModuleFileNameW(static_cast<HMODULE>(mbi.AllocationBase),
                                   buffer.data(),static_cast<DWORD>(buffer.size()));
        if(n==0 || n>=buffer.size())return {};
        return std::filesystem::path(std::wstring(buffer.data(),n));
    }
    static std::filesystem::path privateAssetsDir(
        const std::filesystem::path& binary) {
        // .vst3/Contents/x86_64-win/Plugin.vst3 -> Contents/Resources/PRIVATE
        return binary.parent_path().parent_path()/"Resources"/"PRIVATE";
    }

    bool begin(const std::filesystem::path& binary,
               const std::filesystem::path& privateDir,
               const std::string& externalBank={},unsigned demandLead=0,
               const std::vector<std::uint8_t>* validatedBank=nullptr) {
        if(module_ || worker_.joinable())return false;
        if(demandLead && (demandLead<=2*rt::GuestDemandClock::maxTickFrames ||
                         demandLead>=rt::GuestDemandClock::ringFrames/2))return false;
        demandLead_=demandLead;
        // Real original owner material ONLY. Do not start a fake engine.
        const auto rom=privateDir/"awe32.raw";
        const auto guest=privateDir/"DEMO.EXE";
        const auto sf2=privateDir/"SYNTHGM.SF2";
        if(!isUsableFile(rom,1048576)||!isUsableFile(guest)||
           !isUsableFile(sf2))return false;
        const auto dll=binary.parent_path()/"AWE64DOSBoxRuntime.dll";
        if(!isUsableFile(dll))return false;
        // Non-RT: copy the installed original runtime into this activation's
        // existing private staging directory. Fail rather than reuse a copy.
        // Windows may match a loaded DLL by basename: make that unique too.
        try {
            if(!makeStagingDirectory())return false;
            const auto isolated=stagedDirectory_/(stagedDirectory_.filename().wstring()+L".dll");
            if(!CopyFileW(dll.c_str(),isolated.c_str(),TRUE)||
               GetModuleHandleW(isolated.c_str())) {
                stop();return false;
            }
            module_=LoadLibraryExW(isolated.c_str(),nullptr,
                       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        } catch (...) {
            stop();return false;
        }
        if(!module_){stop();return false;}
        registerMidi_=reinterpret_cast<AWE64_RegisterMidiFn>(
                          GetProcAddress(module_,"AWE64_HostRegisterMidi"));
        registerPCM_=reinterpret_cast<AWE64_RegisterPCMFn>(
                          GetProcAddress(module_,"AWE64_HostRegisterPCM"));
        startGuest_=reinterpret_cast<AWE64_StartGuestFn>(
                          GetProcAddress(module_,"AWE64_StartGuest"));
        requestStop_=reinterpret_cast<AWE64_RequestStopFn>(
                          GetProcAddress(module_,"AWE64_RequestStop"));
        ready_=reinterpret_cast<AWE64_IsGuestMidiReadyFn>(GetProcAddress(module_,"AWE64_IsGuestMidiReady"));
        sends_=reinterpret_cast<AWE64_SetAbsoluteSendsFn>(GetProcAddress(module_,"AWE64_SetAbsoluteSends"));
        guestClockPcm_=reinterpret_cast<AWE64_SetGuestClockPCMFn>(GetProcAddress(module_,"AWE64_SetGuestClockPCM"));
        voiceControls_=reinterpret_cast<AWE64_SetVoiceControlsFn>(GetProcAddress(module_,"AWE64_SetVoiceControls"));
        registerClock_=reinterpret_cast<AWE64_RegisterClockFn>(GetProcAddress(module_,"AWE64_HostRegisterClock"));
        clockFault_=reinterpret_cast<AWE64_GuestClockFaultFn>(GetProcAddress(module_,"AWE64_GuestClockFault"));
        if(!registerMidi_||!registerPCM_||!startGuest_||!requestStop_||!ready_||!sends_||!guestClockPcm_||!voiceControls_||
           (demandLead_&&(!registerClock_||!clockFault_))) {
            stop();return false;
        }
        try {
            std::string guestBank="SYNTHGM.SF2";
            std::string guestProgram="DEMO.EXE";
            // Original files remain untouched. Only the exact proven original
            // Creative executable gets a disposable one-byte compatibility fix.
            if(isUsableFile(guest,40197)) {
                std::vector<std::uint8_t> image(40197);
                std::ifstream input(guest,std::ios::binary);
                if(!input.read(reinterpret_cast<char*>(image.data()),image.size())||
                   !authenticDemo(image)||!awe64::guest::repairCreativeSysexCompare(image)||
                   !makeStagingDirectory()||
                   !awe64::guest::writeExternalSf2(stagedDirectory_/"FXDEMO.EXE",image)) {
                    stop();return false;
                }
                guestProgram="D:\\FXDEMO.EXE";
                authenticGuest_=true;
            }
            if(!externalBank.empty()) {
                std::vector<std::uint8_t> bytes;
                bool adapted=false;
                if(!validatedBank&&!awe64::guest::readExternalSf2(std::filesystem::u8path(externalBank),bytes,adapted)) {
                    stop();return false;
                }
                // Unique disposable directory, outside the installed bundle.
                if(!makeStagingDirectory()){stop();return false;}
                if(!awe64::guest::writeExternalSf2(stagedDirectory_/"BANK.SF2",validatedBank?*validatedBank:bytes)){
                    stop();return false;
                }
                guestBank="D:\\BANK.SF2";
            }
            const char* perf=std::getenv("AWE64_TEST_PERFORMANCE");
            performanceStats_=perf&&std::strcmp(perf,"1")==0;
            measurementStarted_=false;measuredBlocks_=depthTotal_=0;
            depthMin_=32768;depthMax_=midiDepthMax_=0;
            lostControllers_.fill({});haveLostControllers_=false;
            queues_=std::make_unique<rt::RealtimeQueues>();
            if(demandLead_&&!authenticGuest_){stop();return false;}
            adapter_=std::make_unique<DosBoxHostAdapter>(*queues_,ready_,demandLead_!=0,voiceControls_,sends_);
            if(demandLead_ && !adapter_->publishDemand(demandLead_-2*rt::GuestDemandClock::maxTickFrames)) {
                stop();return false;
            }
            // Original DOSBox-X sblaster.cpp forces SBT_NONE if opt_silent.
            // A sounding AWE64 must NEVER include the -silent flag.
            // -nogui and -nomenu suppress UI without disabling hardware.
            argvStorage_={
                "AWE64DOSBoxRuntime", "-noconfig", "-nogui", "-nomenu",
                "-set", "sblaster sbtype=sbawe",
                "-set", "sblaster awe32ram=28672",
                "-set", "sblaster awe32rom="+utf8(rom),
                "-set", "midi mpu401=intelligent",
                "-set", "mixer rate=44100",
            };
            // In the PRIVATE, SHA-verified original-Creative acceptance only,
            // switch OFF other emulated sound generators. Otherwise a guest
            // PC-speaker/OPL tone could falsely pass as AWE/EMU8000 audio.
            // The normal VST3 configuration remains unchanged; crucially
            // the public MIDI->PC-speaker->VST3 roundtrip still tests sound.
            const char* creativeOnly=std::getenv("AWE64_ORIGINAL_CREATIVE_ONLY");
            if(creativeOnly && std::strcmp(creativeOnly,"1")==0) {
                argvStorage_.push_back("-set");
                argvStorage_.push_back("speaker pcspeaker=false");
                argvStorage_.push_back("-set");
                argvStorage_.push_back("sblaster oplmode=none");
            }
            argvStorage_.push_back("-c");
            argvStorage_.push_back("mount c \""+utf8(privateDir)+"\"");
            argvStorage_.push_back("-c");
            argvStorage_.push_back("c:");
            if(!stagedDirectory_.empty()) {
                argvStorage_.push_back("-c");
                argvStorage_.push_back("mount d \""+utf8(stagedDirectory_)+"\"");
            }
            argvStorage_.push_back("-c");
            // Private diagnostic of the UNMODIFIED Creative DOS program.
            // Redirection is disabled for the normal product/positive CI.
            // A per-run, validated DOS 8.3 log prevents stale results or
            // overwriting files in the owner's private VST3 directory.
            if(creativeOnly && std::strcmp(creativeOnly,"1")==0) {
                std::string bootLog="D:\\AWEBOOT.LOG"; // per-activation fallback; no shared log
                if(const char* requested=std::getenv("AWE64_CREATIVE_BOOT_LOG")) {
                    if(std::strlen(requested)!=12 || requested[0]!='B' ||
                       requested[8]!='.' || std::strcmp(requested+9,"LOG")!=0) {
                        stop();return false;
                    }
                    for(std::size_t k=1;k<8;k++) {
                        const char c=requested[k];
                        if(!((c>='0'&&c<='9')||(c>='A'&&c<='F'))) {
                            stop();return false;
                        }
                    }
                    bootLog=requested;
                }
                argvStorage_.push_back(guestProgram+" /SBK:"+guestBank+" > "+bootLog);
            } else {
                argvStorage_.push_back(guestProgram+" /SBK:"+guestBank);
            }
            // Two output destinations: original MPU-401 guest input and
            // original DOSBox audio mixer. Do NOT register BEE0 MIDI consumer.
            sends_(0x10000u);lastSends_=0x10000u;
            voiceControls_(0,0);lastVoiceLo_=0;lastVoiceHi_=0;
            pcmPrimed_=false;
            guestClockPcm_(authenticGuest_?1u:0u);
            registerMidi_(adapter_.get(),&DosBoxHostAdapter::pullMidi);
            registerPCM_(adapter_.get(),&DosBoxHostAdapter::receivePCM);
            if(demandLead_)registerClock_(adapter_.get(),&DosBoxHostAdapter::tickDecision);
            finished_.store(false,std::memory_order_release);
            guestExitCode_.store(-1,std::memory_order_release);
            worker_=std::thread([this]{workerMain();});
            return true;
        } catch (...) {
            stop();
            return false;
        }
    }

    void stop() noexcept {
        // Called ONLY during non-RT VST3 deactivation or destruction.
        if(worker_.joinable()) {
            // An immediate host deactivate (SDK state transition) can arrive
            // before BIOS/DOS initialization. Prefer the actual UART-ready
            // boundary rather than aborting a half-booted emulated PC.
            // Bounded non-RT wait; no added delay for an already ready guest.
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            while(ready_&&!ready_()&&!finished_.load(std::memory_order_acquire)&&
                  std::chrono::steady_clock::now()<deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if(requestStop_)requestStop_();
            // Cooperative join requires original DOSBox to poll GFX_Events;
            // a permanently blocked guest remains an unresolved risk.
            worker_.join();
            const int code=guestExitCode_.load(std::memory_order_acquire);
            if(code!=0) {
                std::fprintf(stderr,"AWE64_RUNTIME_GUEST_EXIT=%d\n",code);
                std::fflush(stderr);
            }
        }
        if(performanceStats_&&measurementStarted_&&measuredBlocks_) {
            // Snapshot was taken at the last audio callback, before the
            // worker stops; do not count shutdown as a runtime failure.
            std::printf("PERF_QUEUE_BLOCKS=%llu AUDIO_DROPPED_FRAMES=%llu MIDI_DROPPED_EVENTS=%llu UNDERRUN_FRAMES=%llu\n",
                static_cast<unsigned long long>(measuredBlocks_),
                static_cast<unsigned long long>(measurementEnd_.audioDrops-measurementStart_.audioDrops),
                static_cast<unsigned long long>(measurementEnd_.midiDrops-measurementStart_.midiDrops),
                static_cast<unsigned long long>(measurementEnd_.underruns-measurementStart_.underruns));
            std::printf("PERF_QUEUE_DEPTH_MIN=%zu MAX=%zu MEAN=%.2f MIDI_DEPTH_MAX=%zu\n",
                depthMin_,depthMax_,double(depthTotal_)/double(measuredBlocks_),midiDepthMax_);
            std::fflush(stdout);
        }
        performanceStats_=measurementStarted_=false;
        if(sends_)sends_(0x10000u);
        if(voiceControls_)voiceControls_(0,0);
        if(guestClockPcm_)guestClockPcm_(0u);
        pcmPrimed_=false;
        authenticGuest_=false;lastSends_=0x10000u;
        if(registerMidi_)registerMidi_(nullptr,nullptr);
        if(registerPCM_)registerPCM_(nullptr,nullptr);
        if(registerClock_)registerClock_(nullptr,nullptr);
        adapter_.reset();
        queues_.reset();
        if(module_) {
            // Diagnose unexpected retained loader references; never force
            // unload a module owned by another Windows subsystem.
            const auto handle=module_;
            FreeLibrary(module_);
            HMODULE stillLoaded=nullptr;
            if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(handle),&stillLoaded)) {
                std::fprintf(stderr,"AWE64_RUNTIME_MODULE_STILL_MAPPED_AFTER_STOP\n");
                std::fflush(stderr);
            }
        }
        module_=nullptr;
        registerMidi_=nullptr;registerPCM_=nullptr;
        startGuest_=nullptr;requestStop_=nullptr;ready_=nullptr;sends_=nullptr;guestClockPcm_=nullptr;voiceControls_=nullptr;
        registerClock_=nullptr;clockFault_=nullptr;demandLead_=0;
        argvStorage_.clear();
        if(!stagedDirectory_.empty()) {
            std::error_code ec;std::filesystem::remove_all(stagedDirectory_,ec);
            stagedDirectory_.clear();
        }
        running_.store(false,std::memory_order_release);
    }
    [[nodiscard]] bool ready(std::size_t hostBlock=0) noexcept {
        if(!ready_||!ready_()||(clockFault_&&clockFault_()))return false;
        if(!authenticGuest_||pcmPrimed_)return true;
        // Add 192 native frames (4.35 ms) of initial reserve after the
        // observed 48-kHz soak underrun. Demand clock stays unchanged.
        const auto target=demandLead_?static_cast<std::size_t>(demandLead_-2*rt::GuestDemandClock::maxTickFrames):
            std::max<std::size_t>(576,std::min<std::size_t>(hostBlock,8192)+320);
        if(!queues_||queues_->snapshot().audioFrames<target)return false;
        pcmPrimed_=true;
        return true;
    }
    [[nodiscard]] bool running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool finished() const noexcept {
        return finished_.load(std::memory_order_acquire)||(clockFault_&&clockFault_());
    }
    [[nodiscard]] int guestExitCode() const noexcept {
        return guestExitCode_.load(std::memory_order_acquire);
    }
    void setVoiceControls(std::uint64_t lo,unsigned hi) noexcept {
        if(authenticGuest_&&voiceControls_&&(lo!=lastVoiceLo_||hi!=lastVoiceHi_)){
            voiceControls_(lo,hi);lastVoiceLo_=lo;lastVoiceHi_=hi;
        }
    }
    void setAbsoluteSends(unsigned reverb,unsigned chorus) noexcept {
        const unsigned packed=(reverb&255u)|((chorus&255u)<<8);
        if(authenticGuest_&&sends_&&packed!=lastSends_) {
            sends_(packed);lastSends_=packed;
        }
    }
    [[nodiscard]] bool clocked() const noexcept {return demandLead_!=0;}
    [[nodiscard]] unsigned demandLead() const noexcept {return demandLead_;}
    [[nodiscard]] std::uint64_t guestFrames() const noexcept {return adapter_?adapter_->guestFrames():0;}
    [[nodiscard]] bool publishDemand(std::uint64_t horizon) noexcept {
        return adapter_&&adapter_->publishDemand(horizon);
    }
    [[nodiscard]] bool submitTimed(rt::MidiBlockBatch& batch,const rt::MidiTimeline& timeline) noexcept {
        if(!demandLead_||!queues_)return false;
        if(performanceStats_&&!measurementStarted_&&batch.hasNoteOn()) {
            measurementStart_=measurementEnd_=queues_->snapshot();
            measurementStarted_=true;
        }
        return batch.submit(timeline,*queues_);
    }
    void requestMidiRecovery() noexcept {if(queues_)queues_->requestMidiRecovery();}
    std::size_t audioFramesAvailable() const noexcept {return queues_?queues_->snapshot().audioFrames:0;}
    bool waitForAudio(std::size_t frames,bool offline) noexcept {
        if(audioFramesAvailable()>=frames)return true;
        if(!offline)return false; // NEVER wait on realtime path
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(!finished()&&std::chrono::steady_clock::now()<until) {
            if(audioFramesAvailable()>=frames)return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
    [[nodiscard]] std::uint64_t midiRecoveryEpoch() const noexcept {return queues_?queues_->midiRecoveryEpoch():0;}
    [[nodiscard]] bool midiRecoveryPending() const noexcept {return queues_&&queues_->midiRecoveryPending();}
    bool sendMidi(const rt::MidiEvent& e) noexcept {
        if(!queues_)return false;
        if(performanceStats_&&!measurementStarted_&&(e.status&0xf0u)==0x90u&&e.data2) {
            // First actual NoteOn after warm-up establishes the baseline.
            measurementStart_=measurementEnd_=queues_->snapshot();
            measurementStarted_=true;
        }
        // Legacy FIFO recovery drains accepted messages, then explicit
        // note-off/panic. Replay only actually rejected persistent controls
        // before the next admitted event, using its current timestamp.
        if(!demandLead_){
            if(queues_->midiRecoveryPending()){
                rememberLost(e);
                queues_->countRejectedMidi();
                return false;
            }
            if((e.status&0xf0u)==0xb0&&e.data1==121){
                const unsigned first=(e.status&15u)*8;
                for(unsigned i=0;i<8;++i)lostControllers_[first+i]={};
            }
            if(haveLostControllers_)for(auto& lost:lostControllers_)if(lost.status){
                auto replay=lost;replay.absoluteHostSample=e.absoluteHostSample;
                replay.guestFrame=e.guestFrame;
                if(!queues_->pushMidi(replay)){rememberLost(e);return false;}
                lost={};
            }
            haveLostControllers_=false;
        }
        if(queues_->pushMidi(e))return true;
        if(!demandLead_)rememberLost(e);
        return false;
    }
    void measureQueueBeforeBlock() noexcept {
        if(!measurementStarted_||!queues_)return;
        const auto sample=queues_->snapshot();
        depthMin_=std::min(depthMin_,sample.audioFrames);
        depthMax_=std::max(depthMax_,sample.audioFrames);
        midiDepthMax_=std::max(midiDepthMax_,sample.midiEvents);
        depthTotal_+=sample.audioFrames;
        ++measuredBlocks_;
    }
    void measureQueueAfterBlock() noexcept {
        if(measurementStarted_&&queues_)measurementEnd_=queues_->snapshot();
    }
    std::size_t readAudio(float* left,float* right,std::size_t n) noexcept {
        return queues_?queues_->readAudio(left,right,n):0u;
    }
    std::uint64_t underruns() const noexcept {
        return queues_?queues_->audioUnderruns():0;
    }
};
} // namespace awe64::integration
