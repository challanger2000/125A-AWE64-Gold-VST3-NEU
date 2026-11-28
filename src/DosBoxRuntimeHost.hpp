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
#include <process.h>
#include "CreativeSysexRepair.hpp"
#include "ResidentBankMailbox.hpp"
#include "ResidentMidiState.hpp"
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
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <utility>

namespace awe64::integration {
class DosBoxRuntimeCore final {
    // DOSBox globals are private to each independently mapped image.
    // Use a unique DLL filename AND directory for every activation, never
    // share the in-bundle module's writable globals between components.
    HMODULE module_=nullptr;
    AWE64_RegisterMidiFn registerMidi_=nullptr;
    AWE64_RegisterIOFn registerIO_=nullptr;
    bool residentMode_=false;
    guest::ResidentBankMailbox residentMailbox_;
    guest::ResidentMidiState residentMidi_; // serialized host producer only
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
    // Consumer-only boot deadline. No sleep, allocation or logging in ready().
    // Set during begin(), immutable once worker/callbacks can access it.
    unsigned bootTimeoutSeconds_=30;
    bool boostBankLoad_=false;
    std::chrono::steady_clock::time_point startupDeadline_{};
    std::atomic<bool> startupTimedOut_{false};
    bool firstPrimeCompleted_=false;
    bool startupSawUart_=false;
    std::size_t startupMaxPcm_=0;
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
    std::shared_ptr<rt::LegacyCaptureGate> captureGate_;
    std::unique_ptr<rt::RealtimeQueues> queues_;
    std::unique_ptr<DosBoxHostAdapter> adapter_;
    std::thread worker_;
    std::vector<std::string> argvStorage_;
    std::atomic<bool> running_{false};
    std::atomic<bool> finished_{false};
    std::atomic<int> guestExitCode_{-1};
    std::filesystem::path stagedDirectory_;
    HANDLE stagedRomLock_=INVALID_HANDLE_VALUE;

    static bool sha256Matches(const std::filesystem::path& p,const char* expected) {
        std::ifstream input(p,std::ios::binary);
        if(!input)return false;
        BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
        if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
        bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
        std::array<unsigned char,16384> bytes{};
        unsigned char digest[32]{};
        while(ok&&input){
            input.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
            const auto n=input.gcount();
            if(n>0)ok=BCryptHashData(hash,bytes.data(),static_cast<ULONG>(n),0)>=0;
        }
        ok=ok&&input.eof()&&!input.bad();
        if(ok)ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
        if(hash)BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg,0);
        constexpr char hex[]="0123456789abcdef";
        for(unsigned i=0;ok&&i<32;++i)
            ok=expected[2*i]==hex[digest[i]>>4]&&expected[2*i+1]==hex[digest[i]&15u];
        return ok;
    }
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
    // The original ROM is immutable for the lifetime of this guest.
    // Copy and validate before entering DOSBox; hold a read-only handle
    // denying writes/deletion until the actual worker has terminated.
    bool stageRom(const std::filesystem::path& source) {
        const auto target=stagedDirectory_/"awe32.raw";
        if(!CopyFileW(source.c_str(),target.c_str(),TRUE))return false;
        stagedRomLock_=CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(stagedRomLock_==INVALID_HANDLE_VALUE)return false;
        LARGE_INTEGER size{};
        if(!GetFileSizeEx(stagedRomLock_,&size)||size.QuadPart!=1048576)return false;
        // Reject lossy narrow-path conversion instead of letting upstream
        // search arbitrary fallback directories or display a download prompt.
        const auto narrow=target.string();
        if(!std::filesystem::equivalent(std::filesystem::path(narrow),target))return false;
        std::ifstream probe(narrow,std::ios::binary);
        return static_cast<bool>(probe);
    }
    bool authenticRom() noexcept {
        // For the exact verified Creative guest, require the same original
        // ROM hash as private package preparation. Synthetic ABI/PCM test
        // guests remain distinguishable and may use their own fake ROM.
        constexpr unsigned char expected[32]={0x4e,0x14,0x3b,0x94,0xf7,0x58,0x73,0x4f,0x59,0x4d,0xed,0x78,0xf4,0xe5,0x11,0x56,0x35,0x97,0x5c,0x16,0xfa,0x37,0xfa,0xbe,0xae,0x0b,0xaa,0x49,0x38,0xca,0x71,0x0e};
        BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
        if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
        bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
        std::array<unsigned char,16384> block{};
        unsigned char digest[32]{};
        for(unsigned i=0;ok&&i<64;++i){
            DWORD n=0;
            ok=ReadFile(stagedRomLock_,block.data(),static_cast<DWORD>(block.size()),&n,nullptr)&&
               n==block.size()&&BCryptHashData(hash,block.data(),n,0)>=0;
        }
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
    DosBoxRuntimeCore()=default;
    DosBoxRuntimeCore(DosBoxRuntimeCore const&)=delete;
    DosBoxRuntimeCore& operator=(DosBoxRuntimeCore const&)=delete;
    ~DosBoxRuntimeCore(){(void)stop(false);}

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
        // .vst3/Contents/x86_64-win/Plugin.vst3 -> Contents/Resources/ROM
        return binary.parent_path().parent_path()/"Resources"/"ROM";
    }

    bool begin(const std::filesystem::path& binary,
               const std::filesystem::path& privateDir,
               const std::string& externalBank={},unsigned demandLead=0,
               const std::vector<std::uint8_t>* validatedBank=nullptr,
               std::shared_ptr<rt::LegacyCaptureGate> captureGate={},bool deferCapture=false) {
        if(module_ || worker_.joinable())return false;
        captureGate_=demandLead?nullptr:std::move(captureGate);
        if(demandLead && (demandLead<=2*rt::GuestDemandClock::maxTickFrames ||
                         demandLead>=rt::GuestDemandClock::ringFrames/2))return false;
        demandLead_=demandLead;
        const char* residentRequested=std::getenv("AWE64_TEST_RESIDENT_BANK");
        residentMode_=residentRequested&&std::strcmp(residentRequested,"1")==0;
        if(residentMode_&&demandLead_)return false; // legacy asynchronous path only
        // Real original owner material ONLY. Do not start a fake engine.
        const auto rom=privateDir/"awe32.raw";
        const auto guest=privateDir/"Driver.exe";
        const auto sf2=privateDir/"SYNTHGM.SF2";
        if(!isUsableFile(rom,1048576)||!isUsableFile(guest)||
           !isUsableFile(sf2))return false;
        const auto dll=binary.parent_path()/"Engine.dll";
        if(!isUsableFile(dll))return false;
        // Non-RT: copy the installed original runtime into this activation's
        // existing private staging directory. Fail rather than reuse a copy.
        // Windows may match a loaded DLL by basename: make that unique too.
        try {
            if(!makeStagingDirectory())return false;
            if(!stageRom(rom)){stop();return false;}
            const auto isolated=stagedDirectory_/(stagedDirectory_.filename().wstring()+L".dll");
            if(!CopyFileW(dll.c_str(),isolated.c_str(),TRUE)||
               GetModuleHandleW(isolated.c_str())) {
                stop();return false;
            }
            if(residentMode_&&!sha256Matches(isolated,"4167cbb8ff8ee7cd344574316589466e76d3927f5f7dc1abcd6da6b3abd1c1c9")){
                stop();return false;
            }
            module_=LoadLibraryExW(isolated.c_str(),nullptr,
                       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        } catch (...) {
            stop();return false;
        }
        if(!module_){stop();return false;}
        if(residentMode_){
            registerIO_=reinterpret_cast<AWE64_RegisterIOFn>(GetProcAddress(module_,"AWE64_HostRegisterIO"));
            if(!registerIO_){stop();return false;}
        }
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
            std::string guestBank="D:\\SYNTHGM.SF2";
            std::string guestProgram="Driver.exe";
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
                if(!authenticRom()){stop();return false;}
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
                // Actual original-driver transfer measurements show a 23.4 MB
                // bank can require 66 s even at 24000 cycles/ms on CI.
                // Keep the existing 30 s bound for small/ROM/synthetic boots.
                boostBankLoad_=authenticGuest_&&(validatedBank?validatedBank->size():bytes.size())>1024u*1024u;
                bootTimeoutSeconds_=boostBankLoad_?180u:30u;
            } else {
                // Use the very same default-bank snapshot as the VST3
                // catalog. Preserve bytes, do not adapt the original bank.
                std::vector<std::uint8_t> bytes;
                if(!validatedBank&&!awe64::guest::readDefaultBankSnapshot(sf2,bytes)){
                    stop();return false;
                }
                const auto& original=validatedBank?*validatedBank:bytes;
                if(original.empty()||original.size()>32ull*1024*1024||
                   !awe64::guest::writeExternalSf2(stagedDirectory_/"SYNTHGM.SF2",original)){
                    stop();return false;
                }
            }
            if(residentMode_){
                // Privately supplied, exact tested bytes only. Both fixed
                // DOS slot names exist before mounting the staging drive.
                const auto resident=stagedDirectory_/"RESIDENT.EXE";
                const auto extender=stagedDirectory_/"DOS4GW.EXE";
                std::vector<std::uint8_t> initial;
                if(!authenticGuest_||
                   !CopyFileW((privateDir/"Resident.exe").c_str(),resident.c_str(),TRUE)||
                   !CopyFileW((privateDir/"DOS4GW.EXE").c_str(),extender.c_str(),TRUE)||
                   !sha256Matches(resident,"fd0c30c0aa8c6651a5e08c715189bcd9966be713f60790159ef332a8060772f5")||
                   !sha256Matches(extender,"b8265123ac8a189637448618409ef3ecd2e9f3e1a47062c685a02240f688dec1")||
                   !guest::readDefaultBankSnapshot(stagedDirectory_/(externalBank.empty()?"SYNTHGM.SF2":"BANK.SF2"),initial)||
                   !guest::writeExternalSf2(stagedDirectory_/"BANKA.SF2",initial)||
                   !guest::writeExternalSf2(stagedDirectory_/"BANKB.SF2",initial)){
                    stop();return false;
                }
                guestProgram="D:\\RESIDENT.EXE";guestBank="D:\\BANKA.SF2";
                boostBankLoad_=false; // resident restores its own clock
            }
            const char* perf=std::getenv("AWE64_TEST_PERFORMANCE");
            performanceStats_=perf&&std::strcmp(perf,"1")==0;
            measurementStarted_=false;measuredBlocks_=depthTotal_=0;
            depthMin_=32768;depthMax_=midiDepthMax_=0;
            lostControllers_.fill({});haveLostControllers_=false;
            queues_=std::make_unique<rt::RealtimeQueues>();
            if(demandLead_&&!authenticGuest_){stop();return false;}
            adapter_=std::make_unique<DosBoxHostAdapter>(*queues_,ready_,demandLead_!=0,voiceControls_,sends_,captureGate_,deferCapture,true,
                                                          residentMode_?&residentMailbox_:nullptr);
            if(demandLead_ && !adapter_->publishDemand(demandLead_-2*rt::GuestDemandClock::maxTickFrames)) {
                stop();return false;
            }
            // Original DOSBox-X sblaster.cpp forces SBT_NONE if opt_silent.
            // A sounding AWE64 must NEVER include the -silent flag.
            // -nogui and -nomenu suppress UI without disabling hardware.
            argvStorage_={
                "Engine", "-noconfig", "-nogui", "-nomenu",
                "-set", "sblaster sbtype=sbawe",
                "-set", "sblaster awe32ram=28672",
                "-set", "sblaster awe32rom="+utf8(stagedDirectory_/"awe32.raw"),
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
            if(residentMode_){argvStorage_.push_back("-c");argvStorage_.push_back("d:");}
            if(boostBankLoad_){
                argvStorage_.push_back("-awe64-boot-boost");
                argvStorage_.push_back("-c");
                argvStorage_.push_back("config -set \"cpu cycles=fixed 24000\"");
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
            startupDeadline_={};startupTimedOut_.store(false,std::memory_order_release);
            firstPrimeCompleted_=false;startupSawUart_=false;startupMaxPcm_=0;
            guestClockPcm_(authenticGuest_?1u:0u);
            if(registerIO_)registerIO_(adapter_.get(),&DosBoxHostAdapter::readPort,&DosBoxHostAdapter::writePort);
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

    bool stop(bool bounded=true) noexcept {
        // Non-RT only. An OS-signalled thread handle proves that it is
        // safe to join/unregister callbacks/unload the original DLL.
        if(worker_.joinable()) {
            using Clock=std::chrono::steady_clock;
            using namespace std::chrono;
            const auto deadline=Clock::now()+milliseconds(2500);
            if(bounded){
                const auto bootDeadline=Clock::now()+milliseconds(1500);
                while(ready_&&!ready_()&&!finished_.load(std::memory_order_acquire)&&Clock::now()<bootDeadline)
                    std::this_thread::sleep_for(milliseconds(5));
            }
            if(requestStop_)requestStop_();
            if(bounded){
                const auto remaining=duration_cast<milliseconds>(deadline-Clock::now()).count();
                const DWORD waitMs=static_cast<DWORD>(std::max<std::int64_t>(0,remaining));
                if(WaitForSingleObject(worker_.native_handle(),waitMs)!=WAIT_OBJECT_0)return false;
            }
            // Unbounded wait only in the reaper or fail-closed OS failure path.
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
        if((performanceStats_||startupTimedOut())&&adapter_){
            std::printf("AWE64_STARTUP_DIAGNOSTIC uart=%d max_pcm=%zu primed=%d timeout=%d pulled_notes=%llu\n",
                        int(startupSawUart_),startupMaxPcm_,int(firstPrimeCompleted_),int(startupTimedOut()),
                        static_cast<unsigned long long>(adapter_->pulledNoteOns()));
            std::fflush(stdout);
        }
        if(performanceStats_&&adapter_){
            std::printf("PERF_SUSPENDED_FRAMES=%llu\n",static_cast<unsigned long long>(adapter_->suspendedFrames()));
            std::printf("PERF_REPLACEMENT_SUPPRESSED_FRAMES=%llu\n",
                        static_cast<unsigned long long>(adapter_->replacementSuppressedFrames()));
            std::printf("PERF_ADAPTER_PULLED_NOTE_ONS=%llu\n",
                        static_cast<unsigned long long>(adapter_->pulledNoteOns()));
        }
        performanceStats_=measurementStarted_=false;
        if(sends_)sends_(0x10000u);
        if(voiceControls_)voiceControls_(0,0);
        if(guestClockPcm_)guestClockPcm_(0u);
        pcmPrimed_=false;
        authenticGuest_=false;lastSends_=0x10000u;
        if(registerIO_)registerIO_(nullptr,nullptr,nullptr);
        if(registerMidi_)registerMidi_(nullptr,nullptr);
        if(registerPCM_)registerPCM_(nullptr,nullptr);
        if(registerClock_)registerClock_(nullptr,nullptr);
        adapter_.reset();
        queues_.reset();
        captureGate_.reset();
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
        registerIO_=nullptr;residentMode_=false;
        registerMidi_=nullptr;registerPCM_=nullptr;
        startGuest_=nullptr;requestStop_=nullptr;ready_=nullptr;sends_=nullptr;guestClockPcm_=nullptr;voiceControls_=nullptr;
        registerClock_=nullptr;clockFault_=nullptr;demandLead_=0;
        argvStorage_.clear();
        if(stagedRomLock_!=INVALID_HANDLE_VALUE){
            CloseHandle(stagedRomLock_);stagedRomLock_=INVALID_HANDLE_VALUE;
        }
        if(!stagedDirectory_.empty()) {
            std::error_code ec;std::filesystem::remove_all(stagedDirectory_,ec);
            stagedDirectory_.clear();
        }
        running_.store(false,std::memory_order_release);
        return true;
    }
    // Non-RT candidate-bank preparation only: do not prime/dequeue PCM
    // or alter host timing while the former runtime is still processing.
    bool waitForGuestMidiReady(const std::atomic<bool>* cancel=nullptr) noexcept {
        // Same bounded budget as initial activation and offline startup.
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(bootTimeoutSeconds_);
        while(std::chrono::steady_clock::now()<deadline){
            if(cancel&&cancel->load(std::memory_order_acquire))return false;
            if(finished()){
                std::fprintf(stderr,"AWE64_CANDIDATE_BOOT_EXIT=%d\n",guestExitCode());
                return false;
            }
            if(ready_&&ready_()&&!finished()&&(!residentMode_||residentMailbox_.transportOpen()))return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::fprintf(stderr,"AWE64_CANDIDATE_BOOT_TIMEOUT_MS=%u\n",bootTimeoutSeconds_*1000u);
        return false;
    }
    void startReplacementCapture() noexcept {if(adapter_)adapter_->startReplacementCapture();}
    void startLegacyMidi() noexcept {if(adapter_)adapter_->startLegacyMidi();}
    using ResidentState=guest::ResidentBankMailbox::State;
    bool residentBankChanging() const noexcept {
        if(!residentMode_)return false;
        const auto s=residentMailbox_.state();
        return s!=ResidentState::Booting&&s!=ResidentState::Idle&&s!=ResidentState::Failed;
    }
    bool residentReloadAvailable() const noexcept {
        return residentMode_&&residentMailbox_.transportOpen()&&!finished()&&ready_&&ready_();
    }
    // Serialized non-RT transition owner stages only the inactive slot.
    // DOS never opens it before the later mailbox command.
    std::uint8_t stageResidentBank(const std::vector<std::uint8_t>& bytes) {
        if(!residentReloadAvailable()||bytes.empty()||bytes.size()>32ull*1024*1024)return 0;
        const auto slot=residentMailbox_.current()==guest::ResidentBankMailbox::bankA?
            guest::ResidentBankMailbox::bankB:guest::ResidentBankMailbox::bankA;
        const auto path=stagedDirectory_/(slot==guest::ResidentBankMailbox::bankA?"BANKA.SF2":"BANKB.SF2");
        if(!std::filesystem::is_regular_file(path))return 0;
        std::ofstream out(path,std::ios::binary|std::ios::trunc);
        if(!out)return 0;
        out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
        out.close();
        std::vector<std::uint8_t> staged;
        return out&&guest::readDefaultBankSnapshot(path,staged)&&staged==bytes?slot:0;
    }
    bool beginResidentBank(std::uint8_t slot) noexcept {
        return residentReloadAvailable()&&residentMailbox_.begin(slot);
    }
    // Resolve a destructive transfer or fail closed, never pretend to cancel
    // it. The transition owner holds lifetime, not the audio lock.
    ResidentState waitResidentBank(bool largeTransfer,const std::atomic<bool>* cancel=nullptr) noexcept {
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(largeTransfer?180u:30u);
        while(std::chrono::steady_clock::now()<until){
            const auto s=residentMailbox_.waitResult(finished(),cancel&&cancel->load(std::memory_order_acquire));
            if(s==ResidentState::Loaded||s==ResidentState::Rejected||s==ResidentState::Restored)return s;
            if(s==ResidentState::Failed)break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Never expose unknown bank bytes, even if a late ACK arrives.
        // On host cancellation, the superseding recall/deactivation can
        // retire this stopped core instead of waiting the full load budget.
        failResidentBank();return ResidentState::Failed;
    }
    void failResidentBank() noexcept {
        residentMailbox_.fail();
        if(requestStop_)requestStop_();
    }
    bool finishResidentBank() noexcept {
        const auto s=residentMailbox_.state();
        if(s!=ResidentState::Loaded&&s!=ResidentState::Rejected&&s!=ResidentState::Restored)return false;
        // Sole host consumer under lifecycle lock; guest PCM remains gated.
        discardLegacyAudio();
        if(adapter_)adapter_->resetResidentTransport();
        firstPrimeCompleted_=false;startupSawUart_=false;startupMaxPcm_=0;
        lastVoiceHi_=0x10000u;lastSends_=0x10000u;
        lostControllers_.fill({});haveLostControllers_=false;
        // No audio producer can enter under runtimeMutex_. Queue the
        // persistent controller snapshot before any new host NoteOn.
        if(finished()||!queues_||!residentMidi_.replay([this](const rt::MidiEvent& e) noexcept {
            return queues_->pushMidi(e);
        }))return false;
        return residentMailbox_.finish();
    }
    [[nodiscard]] bool ready(std::size_t hostBlock=0) noexcept {
        if(finished()||(residentMode_&&!residentMailbox_.transportOpen()))return false;
        const bool uart=ready_&&ready_();
        if(finished())return false;
        if(pcmPrimed_)return uart;
        // Zero-sample queries do not start a deadline or open legacy MIDI.
        if(!authenticGuest_&&hostBlock==0)return uart;
        const auto target=demandLead_?static_cast<std::size_t>(demandLead_-2*rt::GuestDemandClock::maxTickFrames):
            std::max<std::size_t>(576,std::min<std::size_t>(hostBlock,8192)+320);
        const auto available=queues_?queues_->snapshot().audioFrames:0u;
        if(!firstPrimeCompleted_){
            startupSawUart_|=uart;
            startupMaxPcm_=std::max(startupMaxPcm_,available);
        }
        // Check real readiness first: a long idle period alone must not
        // fault an already initialized guest. Same 30-s bound as candidate
        // bank preparation and offline startup; no retries-to-green.
        if(uart&&available>=target){
            pcmPrimed_=true;
            firstPrimeCompleted_=true;
            startupDeadline_={};
            return !finished();
        }
        if(hostBlock&&!firstPrimeCompleted_){
            const auto now=std::chrono::steady_clock::now();
            if(startupDeadline_==std::chrono::steady_clock::time_point{})
                startupDeadline_=now+std::chrono::seconds(bootTimeoutSeconds_);
            else if(now>=startupDeadline_)
                startupTimedOut_.store(true,std::memory_order_release);
        }
        return false;
    }
    [[nodiscard]] bool running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }
    unsigned bootTimeoutSeconds() const noexcept {return bootTimeoutSeconds_;}
    [[nodiscard]] bool startupTimedOut() const noexcept {return startupTimedOut_.load(std::memory_order_acquire);}
    [[nodiscard]] bool finished() const noexcept {
        return startupTimedOut()||finished_.load(std::memory_order_acquire)||(clockFault_&&clockFault_())||
               (residentMode_&&residentMailbox_.state()==ResidentState::Failed);
    }
    [[nodiscard]] int guestExitCode() const noexcept {
        return guestExitCode_.load(std::memory_order_acquire);
    }
    bool setVoiceControls(std::uint64_t lo,unsigned hi) noexcept {
        if(authenticGuest_&&voiceControls_&&(lo!=lastVoiceLo_||hi!=lastVoiceHi_)){
            // Guest thread consumes the complete 10-byte control packet.
            if(!sendMidi(rt::makeVoiceControlCommand(0,lo,hi)))return false;
            lastVoiceLo_=lo;lastVoiceHi_=hi;
        }
        return true;
    }
    bool setAbsoluteSends(unsigned reverb,unsigned chorus) noexcept {
        const unsigned packed=(reverb&255u)|((chorus&255u)<<8);
        if(authenticGuest_&&sends_&&packed!=lastSends_) {
            if(!sendMidi(rt::makeAbsoluteSendCommand(0,packed)))return false;
            lastSends_=packed;
        }
        return true;
    }
    [[nodiscard]] bool clocked() const noexcept {return demandLead_!=0;}
    [[nodiscard]] unsigned demandLead() const noexcept {return demandLead_;}
    [[nodiscard]] std::uint64_t guestFrames() const noexcept {return adapter_?adapter_->guestFrames():0;}
    [[nodiscard]] bool publishDemand(std::uint64_t horizon) noexcept {
        return !finished()&&adapter_&&adapter_->publishDemand(horizon);
    }
    [[nodiscard]] bool submitTimed(rt::MidiBlockBatch& batch,const rt::MidiTimeline& timeline) noexcept {
        if(finished()||!demandLead_||!queues_)return false;
        if(performanceStats_&&!measurementStarted_&&batch.hasNoteOn()) {
            measurementStart_=measurementEnd_=queues_->snapshot();
            measurementStarted_=true;
        }
        return batch.submit(timeline,*queues_);
    }
    void requestMidiRecovery() noexcept {if(queues_)queues_->requestMidiRecovery();}
    std::size_t audioFramesAvailable() const noexcept {return queues_?queues_->snapshot().audioFrames:0;}
    bool waitForAudio(std::size_t frames,bool offline) noexcept {
        if(finished())return false;
        if(audioFramesAvailable()>=frames)return !finished();
        if(!offline)return false; // NEVER wait on realtime path
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        while(!finished()&&std::chrono::steady_clock::now()<until) {
            if(audioFramesAvailable()>=frames)return !finished();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }
    [[nodiscard]] std::uint64_t midiRecoveryEpoch() const noexcept {return queues_?queues_->midiRecoveryEpoch():0;}
    [[nodiscard]] bool midiRecoveryPending() const noexcept {return queues_&&queues_->midiRecoveryPending();}
    bool sendMidi(const rt::MidiEvent& e) noexcept {
        if(finished()||!queues_||residentBankChanging())return false;
        if(residentMode_)residentMidi_.remember(e);
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
    // Consumer only, while capture is still suspended. Old-generation
    // frames from an in-flight callback are rejected by the epoch check.
    void discardLegacyAudio() noexcept {if(queues_)queues_->discardAudio();pcmPrimed_=false;startupDeadline_={};}
    std::size_t readAudio(float* left,float* right,std::size_t n) noexcept {
        if(finished()||residentBankChanging()){
            if(left)std::fill_n(left,n,0.f);
            if(right)std::fill_n(right,n,0.f);
            return 0;
        }
        return queues_?queues_->readAudio(left,right,n,captureGate_?captureGate_->epoch():0u):0u;
    }
    std::uint64_t underruns() const noexcept {
        return queues_?queues_->audioUnderruns():0;
    }
};

// Exceptional shutdown: retain the entire core, including callbacks,
// queues, thread, staged DLL and paths, until actual thread termination.
// Pin this VST3 module on timeout only so its code cannot unload while
// the reaper or original guest calls it. No forced thread termination.
// This is bounded-worker shutdown containment, not arbitrary crash isolation.
class DosBoxRuntimeHost final {
    std::unique_ptr<DosBoxRuntimeCore> core_;
    inline static std::atomic<unsigned> quarantined_{0};
    static unsigned __stdcall reap(void* ptr) noexcept {
        auto* core=static_cast<DosBoxRuntimeCore*>(ptr);
        (void)core->stop(false);
        delete core;
        quarantined_.fetch_sub(1,std::memory_order_acq_rel);
        return 0;
    }
    static bool pinContainingModule() noexcept {
        static const int anchor=0;
        HMODULE module=nullptr;
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
                                  GET_MODULE_HANDLE_EX_FLAG_PIN,
                                  reinterpret_cast<LPCWSTR>(&anchor),&module)!=0;
    }
public:
    DosBoxRuntimeHost()=default;
    DosBoxRuntimeHost(const DosBoxRuntimeHost&)=delete;
    DosBoxRuntimeHost& operator=(const DosBoxRuntimeHost&)=delete;
    ~DosBoxRuntimeHost(){stop();}
    static std::filesystem::path pluginBinaryPath() noexcept {return DosBoxRuntimeCore::pluginBinaryPath();}
    static std::filesystem::path privateAssetsDir(const std::filesystem::path& p){return DosBoxRuntimeCore::privateAssetsDir(p);}
    static unsigned quarantinedRuntimeCount() noexcept {return quarantined_.load(std::memory_order_acquire);}
    bool begin(const std::filesystem::path& binary,const std::filesystem::path& dir,
               const std::string& bank={},unsigned demandLead=0,
               const std::vector<std::uint8_t>* validatedBank=nullptr,
               std::shared_ptr<rt::LegacyCaptureGate> captureGate={},bool deferCapture=false) {
        if(core_||quarantinedRuntimeCount())return false;
        try{
            auto core=std::make_unique<DosBoxRuntimeCore>();
            if(!core->begin(binary,dir,bank,demandLead,validatedBank,std::move(captureGate),deferCapture))return false;
            core_=std::move(core);return true;
        }catch(...){return false;}
    }
    void stop() noexcept {
        if(!core_)return;
        if(core_->stop()){core_.reset();return;}
        if(!pinContainingModule()){
            // If the OS refuses the lifetime guard, never free live code.
            std::fprintf(stderr,"AWE64_RUNTIME_QUARANTINE_PIN_FAILED\n");
            (void)core_->stop(false);core_.reset();return;
        }
        auto* stranded=core_.release();
        quarantined_.fetch_add(1,std::memory_order_acq_rel);
        const auto thread=_beginthreadex(nullptr,0,&DosBoxRuntimeHost::reap,stranded,0,nullptr);
        if(thread)CloseHandle(reinterpret_cast<HANDLE>(thread));
        // Reaper creation failure retains the entire core until process exit.
        std::fprintf(stderr,"AWE64_RUNTIME_QUARANTINED reaper=%s\n",thread?"started":"unavailable");
    }
    bool waitForGuestMidiReady(const std::atomic<bool>* cancel=nullptr) noexcept {return core_&&core_->waitForGuestMidiReady(cancel);}
    void startReplacementCapture() noexcept {if(core_)core_->startReplacementCapture();}
    void startLegacyMidi() noexcept {if(core_)core_->startLegacyMidi();}
    using ResidentState=DosBoxRuntimeCore::ResidentState;
    bool residentBankChanging() const noexcept {return core_&&core_->residentBankChanging();}
    bool residentReloadAvailable() const noexcept {return core_&&core_->residentReloadAvailable();}
    std::uint8_t stageResidentBank(const std::vector<std::uint8_t>& bytes) {return core_?core_->stageResidentBank(bytes):0;}
    bool beginResidentBank(std::uint8_t slot) noexcept {return core_&&core_->beginResidentBank(slot);}
    ResidentState waitResidentBank(bool largeTransfer,const std::atomic<bool>* cancel=nullptr) noexcept {
        return core_?core_->waitResidentBank(largeTransfer,cancel):ResidentState::Failed;
    }
    bool finishResidentBank() noexcept {return core_&&core_->finishResidentBank();}
    void failResidentBank() noexcept {if(core_)core_->failResidentBank();}
    bool ready(std::size_t frames=0) noexcept {return core_&&core_->ready(frames);}
    bool running() const noexcept {return core_&&core_->running();}
    bool finished() const noexcept {return !core_||core_->finished();}
    unsigned bootTimeoutSeconds() const noexcept {return core_?core_->bootTimeoutSeconds():30u;}
    bool startupTimedOut() const noexcept {return core_&&core_->startupTimedOut();}
    int guestExitCode() const noexcept {return core_?core_->guestExitCode():-1;}
    bool setVoiceControls(std::uint64_t lo,unsigned hi) noexcept {return core_&&core_->setVoiceControls(lo,hi);}
    bool setAbsoluteSends(unsigned r,unsigned c) noexcept {return core_&&core_->setAbsoluteSends(r,c);}
    bool clocked() const noexcept {return core_&&core_->clocked();}
    unsigned demandLead() const noexcept {return core_?core_->demandLead():0;}
    std::uint64_t guestFrames() const noexcept {return core_?core_->guestFrames():0;}
    bool publishDemand(std::uint64_t horizon) noexcept {return core_&&core_->publishDemand(horizon);}
    bool submitTimed(rt::MidiBlockBatch& b,const rt::MidiTimeline& t) noexcept {return core_&&core_->submitTimed(b,t);}
    void requestMidiRecovery() noexcept {if(core_)core_->requestMidiRecovery();}
    std::size_t audioFramesAvailable() const noexcept {return core_?core_->audioFramesAvailable():0;}
    bool waitForAudio(std::size_t n,bool offline) noexcept {return core_&&core_->waitForAudio(n,offline);}
    std::uint64_t midiRecoveryEpoch() const noexcept {return core_?core_->midiRecoveryEpoch():0;}
    bool midiRecoveryPending() const noexcept {return core_&&core_->midiRecoveryPending();}
    bool sendMidi(const rt::MidiEvent& e) noexcept {return core_&&core_->sendMidi(e);}
    void measureQueueBeforeBlock() noexcept {if(core_)core_->measureQueueBeforeBlock();}
    void measureQueueAfterBlock() noexcept {if(core_)core_->measureQueueAfterBlock();}
    void discardLegacyAudio() noexcept {if(core_)core_->discardLegacyAudio();}
    std::size_t readAudio(float* l,float* r,std::size_t n) noexcept {return core_?core_->readAudio(l,r,n):0;}
    std::uint64_t underruns() const noexcept {return core_?core_->underruns():0;}
};
} // namespace awe64::integration
