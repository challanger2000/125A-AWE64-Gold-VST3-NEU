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
EXPORT void AWE64_CALL AWE64_HostRegisterMidi(void* context,AWE64_PullMidiFn fn){midi=fn;midiContext=context;}
EXPORT void AWE64_CALL AWE64_HostRegisterPCM(void*,AWE64_PcmFn){}
EXPORT void AWE64_CALL AWE64_RequestStop(){stop.store(true,std::memory_order_release);}
EXPORT int AWE64_CALL AWE64_IsGuestMidiReady(){return ready.load(std::memory_order_acquire)?1:0;}
EXPORT void AWE64_CALL AWE64_SetAbsoluteSends(unsigned){}
EXPORT void AWE64_CALL AWE64_SetVoiceControls(uint64_t,unsigned){}
EXPORT void AWE64_CALL AWE64_SetGuestClockPCM(unsigned){}
EXPORT int AWE64_CALL AWE64_StartGuest(int,char**){
    using namespace std::chrono;
    const char* m=std::getenv("AWE64_AUDIT_SHIM_MODE");
    const char* p=std::getenv("AWE64_AUDIT_SHIM_DIR");
    if(!m||!p)return 91;
    const std::string mode=m;const std::filesystem::path dir=p;
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
