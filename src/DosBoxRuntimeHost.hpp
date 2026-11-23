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
#include <windows.h>
#include "DosBoxHostAdapter.hpp"
#include "AWE64_DOSBOX_ABI.h"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <utility>

namespace awe64::integration {
class DosBoxRuntimeHost final {
    // Original DOSBox-X source is single-instance global state. Never run
    // two emulated PCs concurrently inside one DAW process.
    static inline std::atomic<bool> ownership_{false};
    HMODULE module_=nullptr;
    AWE64_RegisterMidiFn registerMidi_=nullptr;
    AWE64_RegisterPCMFn registerPCM_=nullptr;
    AWE64_StartGuestFn startGuest_=nullptr;
    AWE64_RequestStopFn requestStop_=nullptr;
    std::unique_ptr<rt::RealtimeQueues> queues_;
    std::unique_ptr<DosBoxHostAdapter> adapter_;
    std::thread worker_;
    std::vector<std::string> argvStorage_;
    std::atomic<bool> running_{false};
    std::atomic<bool> finished_{false};
    std::atomic<int> guestExitCode_{-1};
    bool owns_=false;

    static bool isUsableFile(const std::filesystem::path& p,
                             std::uintmax_t expectedSize=0) noexcept {
        try {
            if(!std::filesystem::is_regular_file(p))return false;
            const auto n=std::filesystem::file_size(p);
            return n>0 && (expectedSize==0 || n==expectedSize);
        } catch (...) {return false;}
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
               const std::filesystem::path& privateDir) {
        if(module_ || worker_.joinable())return false;
        // Real original owner material ONLY. Do not start a fake engine.
        const auto rom=privateDir/"awe32.raw";
        const auto guest=privateDir/"DEMO.EXE";
        const auto sf2=privateDir/"SYNTHGM.SF2";
        if(!isUsableFile(rom,1048576)||!isUsableFile(guest)||
           !isUsableFile(sf2))return false;
        const auto dll=binary.parent_path()/"AWE64DOSBoxRuntime.dll";
        if(!isUsableFile(dll))return false;
        bool none=false;
        if(!ownership_.compare_exchange_strong(none,true,
                std::memory_order_acq_rel))return false;
        owns_=true;
        module_=LoadLibraryExW(dll.wstring().c_str(),nullptr,
                   LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if(!module_){stop();return false;}
        registerMidi_=reinterpret_cast<AWE64_RegisterMidiFn>(
                          GetProcAddress(module_,"AWE64_HostRegisterMidi"));
        registerPCM_=reinterpret_cast<AWE64_RegisterPCMFn>(
                          GetProcAddress(module_,"AWE64_HostRegisterPCM"));
        startGuest_=reinterpret_cast<AWE64_StartGuestFn>(
                          GetProcAddress(module_,"AWE64_StartGuest"));
        requestStop_=reinterpret_cast<AWE64_RequestStopFn>(
                          GetProcAddress(module_,"AWE64_RequestStop"));
        if(!registerMidi_||!registerPCM_||!startGuest_||!requestStop_) {
            stop();return false;
        }
        try {
            queues_=std::make_unique<rt::RealtimeQueues>();
            adapter_=std::make_unique<DosBoxHostAdapter>(*queues_);
            argvStorage_={
                "AWE64DOSBoxRuntime", "-noconfig", "-silent",
                "-set", "sblaster sbtype=sbawe",
                "-set", "sblaster awe32ram=28672",
                "-set", "sblaster awe32rom="+utf8(rom),
                "-set", "midi mpu401=uart",
                "-set", "mixer rate=44100",
                "-c", "mount c \""+utf8(privateDir)+"\"",
                "-c", "c:",
                "-c", "DEMO.EXE /SF2:SYNTHGM.SF2",
            };
            // Two output destinations: original MPU-401 guest input and
            // original DOSBox audio mixer. Do NOT register BEE0 MIDI consumer.
            registerMidi_(adapter_.get(),&DosBoxHostAdapter::pullMidi);
            registerPCM_(adapter_.get(),&DosBoxHostAdapter::receivePCM);
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
            if(requestStop_)requestStop_();
            // Cooperative join requires original DOSBox to poll GFX_Events;
            // a permanently blocked guest remains an unresolved risk.
            worker_.join();
        }
        if(registerMidi_)registerMidi_(nullptr,nullptr);
        if(registerPCM_)registerPCM_(nullptr,nullptr);
        adapter_.reset();
        queues_.reset();
        if(module_)FreeLibrary(module_);
        module_=nullptr;
        registerMidi_=nullptr;registerPCM_=nullptr;
        startGuest_=nullptr;requestStop_=nullptr;
        argvStorage_.clear();
        running_.store(false,std::memory_order_release);
        if(owns_) {
            ownership_.store(false,std::memory_order_release);
            owns_=false;
        }
    }
    [[nodiscard]] bool running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool finished() const noexcept {
        return finished_.load(std::memory_order_acquire);
    }
    [[nodiscard]] int guestExitCode() const noexcept {
        return guestExitCode_.load(std::memory_order_acquire);
    }
    bool sendMidi(const rt::MidiEvent& e) noexcept {
        return queues_ && queues_->pushMidi(e);
    }
    std::size_t readAudio(float* left,float* right,std::size_t n) noexcept {
        return queues_?queues_->readAudio(left,right,n):0u;
    }
    std::uint64_t underruns() const noexcept {
        return queues_?queues_->audioUnderruns():0;
    }
};
} // namespace awe64::integration
