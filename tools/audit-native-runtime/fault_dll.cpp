// Deliberately synthetic test DLL; no DOSBox, no audio/Creative emulation.
// Exact public ABI, deterministic fault injection at Windows loader boundary.
#include "AWE64_DOSBOX_ABI.h"
#include <windows.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <string>
#define EXPORT extern "C" __declspec(dllexport)
static std::atomic<bool> stop{false},ready{false};
static AWE64_PullMidiFn midi=nullptr;
static void* midiContext=nullptr;
static AWE64_PcmFn pcm=nullptr;
static void* pcmContext=nullptr;
EXPORT void AWE64_CALL AWE64_HostRegisterMidi(void* context,AWE64_PullMidiFn fn){midi=fn;midiContext=context;}
EXPORT void AWE64_CALL AWE64_HostRegisterPCM(void* context,AWE64_PcmFn fn){pcm=fn;pcmContext=context;}
EXPORT void AWE64_CALL AWE64_RequestStop(){stop.store(true,std::memory_order_release);}
EXPORT int AWE64_CALL AWE64_IsGuestMidiReady(){return ready.load(std::memory_order_acquire)?1:0;}
EXPORT void AWE64_CALL AWE64_SetAbsoluteSends(unsigned){}
EXPORT void AWE64_CALL AWE64_SetVoiceControls(uint64_t,unsigned){}
EXPORT void AWE64_CALL AWE64_SetGuestClockPCM(unsigned){}
EXPORT int AWE64_CALL AWE64_StartGuest(int argc,char** argv){
    using namespace std::chrono;
    const char* m=std::getenv("AWE64_AUDIT_SHIM_MODE");
    const char* p=std::getenv("AWE64_AUDIT_SHIM_DIR");
    if(!m||!p)return 91;
    const std::string mode=m;const std::filesystem::path dir=p;
    if(mode=="bootstall"){
        if(!pcm||!midi)return 98;
        ready.store(true,std::memory_order_release);
        while(!stop.load(std::memory_order_acquire)&&!std::filesystem::exists(dir/"resume"))
            std::this_thread::sleep_for(milliseconds(1));
        if(!stop.load(std::memory_order_acquire)){
            int16_t samples[2048];for(auto& x:samples)x=8192;
            pcm(pcmContext,samples,1024);
            unsigned char packet[11]{};
            if(midi(midiContext,packet,11))std::ofstream(dir/"late-midi").put('1');
            std::ofstream(dir/"late-pcm").put('1');
        }
        while(!stop.load(std::memory_order_acquire))std::this_thread::sleep_for(milliseconds(1));
        return 0;
    }
    if(mode=="defaultbank"){
        std::string staged;
        bool bankArgument=false;
        for(int i=0;i<argc;++i){
            const std::string arg=argv[i];
            const std::string prefix="mount d \"";
            if(arg.rfind(prefix,0)==0&&arg.back()=='"')
                staged=arg.substr(prefix.size(),arg.size()-prefix.size()-1);
            if(arg.find("/SBK:D:\\SYNTHGM.SF2")!=std::string::npos)bankArgument=true;
        }
        if(staged.empty()||!bankArgument)return 93;
        const auto path=std::filesystem::path(staged)/"SYNTHGM.SF2";
        std::ofstream(dir/"bank-path.txt")<<path.string();
        ready.store(true,std::memory_order_release);
        while(!stop.load(std::memory_order_acquire)&&!std::filesystem::exists(dir/"resume"))
            std::this_thread::sleep_for(milliseconds(1));
        std::ifstream f(path,std::ios::binary);
        char ch=0;
        const bool exact=bool(f.get(ch))&&ch=='X'&&f.peek()==std::char_traits<char>::eof();
        if(exact)std::ofstream(dir/"bank-read-ok").put('1');
        while(!stop.load(std::memory_order_acquire))std::this_thread::sleep_for(milliseconds(1));
        return exact?0:94;
    }
    if(mode=="exitready"){
        if(!pcm||!midi)return 98;
        ready.store(true,std::memory_order_release);
        int16_t samples[2048];
        for(auto& x:samples)x=8192;
        pcm(pcmContext,samples,1024);
        while(!stop.load(std::memory_order_acquire)&&!std::filesystem::exists(dir/"resume"))
            std::this_thread::sleep_for(milliseconds(1));
        // Preserve the deliberately stale readiness and nonzero PCM.
        return 97;
    }
    if(mode=="startup"){
        if(!pcm||!midi)return 98;
        ready.store(true,std::memory_order_release);
        unsigned char bytes[11]{};
        // Poll repeatedly before PCM is produced; the host's early packets
        // must remain untouched until the actual PCM reserve is published.
        for(unsigned n=0;n<100;++n){
            if(midi(midiContext,bytes,11))std::ofstream(dir/"startup-premature").put('1');
            std::this_thread::sleep_for(milliseconds(2));
        }
        std::ofstream(dir/"startup-polled").put('1');
        while(!stop.load(std::memory_order_acquire)&&!std::filesystem::exists(dir/"resume"))
            std::this_thread::sleep_for(milliseconds(1));
        const int16_t frames[2048]{};
        if(!stop.load(std::memory_order_acquire))pcm(pcmContext,frames,1024);
        unsigned stage=0;
        while(!stop.load(std::memory_order_acquire)){
            const auto n=midi(midiContext,bytes,11);
            if(n){
                if(stage==0&&n==3&&bytes[0]==0xb0&&bytes[1]==0&&bytes[2]==0)++stage;
                else if(stage==1&&n==2&&bytes[0]==0xc0&&bytes[1]==0)++stage;
                else if(stage==2&&n==3&&bytes[0]==0x90&&bytes[1]==60&&bytes[2]==100){
                    ++stage;std::ofstream(dir/"startup-delivered").put('1');
                }else return 99;
            }else std::this_thread::sleep_for(milliseconds(1));
        }
        return stage==3?0:99;
    }
    if(mode=="candidate"){
        if(!pcm)return 97;
        ready.store(true,std::memory_order_release);
        const int16_t old[8]={24576,24576,24576,24576,24576,24576,24576,24576};
        for(unsigned n=0;n<10000;++n)pcm(pcmContext,old,4);
        std::ofstream(dir/"candidate-burst-done").put('1');
        while(!stop.load(std::memory_order_acquire)&&!std::filesystem::exists(dir/"resume"))
            std::this_thread::sleep_for(milliseconds(1));
        const int16_t fresh[8]={8192,-8192,8192,-8192,8192,-8192,8192,-8192};
        if(!stop.load(std::memory_order_acquire))pcm(pcmContext,fresh,4);
        while(!stop.load(std::memory_order_acquire))std::this_thread::sleep_for(milliseconds(1));
        return 0;
    }
    if(mode=="bootfail"){
        // begin() has already loaded this ABI and launched the worker.
        // Return an actual error without ever opening the UART.
        std::ofstream(dir/"boot-failed").put('1');
        std::this_thread::sleep_for(milliseconds(300));
        return 96;
    }
    if(mode=="rom"){
        const std::string prefix="sblaster awe32rom=";
        std::string selected;
        for(int i=0;i<argc;++i){
            const std::string arg=argv[i];
            if(arg.rfind(prefix,0)==0)selected=arg.substr(prefix.size());
        }
        if(selected.empty())return 93;
        std::ofstream(dir/"rom-path.txt")<<selected;
        ready.store(true,std::memory_order_release);
        while(!stop.load(std::memory_order_acquire)&&!std::filesystem::exists(dir/"resume"))
            std::this_thread::sleep_for(milliseconds(1));
        std::ifstream rom(selected,std::ios::binary);
        unsigned count=0;char byte=0;bool zero=true;
        while(rom.get(byte)){++count;zero=zero&&byte==0;}
        if(count==1048576&&zero)std::ofstream(dir/"rom-read-ok").put('1');
        while(!stop.load(std::memory_order_acquire))std::this_thread::sleep_for(milliseconds(1));
        return count==1048576&&zero?0:94;
    }
    ready.store(true,std::memory_order_release);
    if(mode=="stall"){
        // Simulate an unresponsive worker that ignores the stop flag for
        // 6 seconds but then returns; no genuinely infinite hang in CI.
        std::this_thread::sleep_for(seconds(6));return 0;
    }
    if(mode=="recovery"){
        std::ofstream log(dir/"midi.txt",std::ios::out|std::ios::trunc);
        if(!log)return 92;
        bool first=false;
        while(!stop.load(std::memory_order_acquire)){
            if(first&&!std::filesystem::exists(dir/"resume")){
                std::this_thread::sleep_for(milliseconds(1));continue;
            }
            unsigned char bytes[11]{};
            const auto n=midi?midi(midiContext,bytes,11):0u;
            if(n){
                log<<unsigned(bytes[0])<<' '<<unsigned(bytes[1])<<' '<<unsigned(bytes[2])<<'\n'<<std::flush;
                if(!first&&n==3&&bytes[0]==0xb0&&bytes[1]==7&&bytes[2]==0){
                    first=true;std::ofstream(dir/"baseline-received").put('1');
                }
            }else std::this_thread::sleep_for(milliseconds(1));
        }
        return 0;
    }
    while(!stop.load(std::memory_order_acquire))std::this_thread::sleep_for(milliseconds(1));
    return 0;
}
