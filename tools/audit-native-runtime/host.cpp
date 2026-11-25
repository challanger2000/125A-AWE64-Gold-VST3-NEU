// Exact Windows production runtime host under deterministic test DLL faults.
// No actual VST3, no original DOSBox, no audio fidelity claim.
#include "DosBoxRuntimeHost.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace awe64;
using namespace std::chrono;
using Clock=steady_clock;
static int infra(const char* msg){std::cerr<<"AUDIT_INFRA_FAILURE "<<msg<<std::endl;return 2;}
template<class Predicate> static bool wait(Predicate p){
    const auto until=Clock::now()+seconds(5);
    while(Clock::now()<until){if(p())return true;std::this_thread::sleep_for(milliseconds(2));}
    return p();
}
int main(int argc,char** argv){
    std::cout<<std::unitbuf;std::cerr<<std::unitbuf;
    if(argc!=3)return infra("usage: host mode synthetic-fixture-dir");
    const std::string mode=argv[1];
    const auto dir=std::filesystem::absolute(argv[2]);
    if(mode!="normal"&&mode!="stall"&&mode!="recovery")return infra("mode");
    // These files contain only self-generated fake bytes; use a fresh directory.
    if(!std::filesystem::create_directory(dir))return infra("fresh fixture dir");
    {std::ofstream f(dir/"awe32.raw",std::ios::binary);std::vector<char> zero(1048576,0);f.write(zero.data(),zero.size());}
    std::ofstream(dir/"DEMO.EXE",std::ios::binary).put('X');
    std::ofstream(dir/"SYNTHGM.SF2",std::ios::binary).put('X');
    _putenv_s("AWE64_AUDIT_SHIM_MODE",mode.c_str());_putenv_s("AWE64_AUDIT_SHIM_DIR",dir.string().c_str());
    integration::DosBoxRuntimeHost h;
    // DLL is the explicit controlled fixture beside this executable.
    auto exe=std::filesystem::absolute(argv[0]);
    if(!h.begin(exe,dir))return infra("real production RuntimeHost::begin");
    if(!wait([&]{return h.ready();}))return infra("shim readiness not observed");
    unsigned failures=0;
    if(mode=="recovery"){
        if(!h.sendMidi({0,0xb0,7,0,3})||!wait([&]{return std::filesystem::exists(dir/"baseline-received");}))
            return infra("CC7 baseline not consumed");
        for(unsigned n=0;n<4096;++n)
            if(!h.sendMidi({n,0xb0,1,0,3}))return infra("fill expected queue");
        const bool accepted=h.sendMidi({4096,0xb0,7,127,3});
        if(accepted)return infra("overflow injection not reached");
        std::ofstream(dir/"resume").put('1');
        if(!wait([&]{return h.midiRecoveryEpoch()>0&&!h.midiRecoveryPending();}))return infra("recovery epoch incomplete");
        if(!h.sendMidi({4097,0x90,60,100,3}))return infra("post-recovery note rejected");
        // Wait until worker actually consumed the new note, not just queued it.
        auto delivered=[&](){
            std::ifstream f(dir/"midi.txt");unsigned a,b,c;
            while(f>>a>>b>>c)if(a==0x90&&b==60&&c==100)return true;
            return false;
        };
        if(!wait(delivered))return infra("post-recovery note not delivered");
        h.stop();
        std::ifstream f(dir/"midi.txt");unsigned a,b,c;bool zero=false,restored=false,note=false;
        while(f>>a>>b>>c){
            if(a==0xb0&&b==7){if(c==0)zero=true;if(c==127)restored=true;}
            if(a==0x90&&b==60&&c==100){note=true;break;}
        }
        if(!zero||!note)return infra("independent MIDI log control");
        std::cout<<"AUDIT_A02_NATIVE_CC7_REPLAY "<<(restored?"PASS":"FAIL")
                 <<" baseline_zero="<<zero<<" overflow_accepted="<<accepted<<" new_note_delivered="<<note<<std::endl;
        failures+=!restored;
    }else{
        const auto start=Clock::now();h.stop();
        const auto elapsed=duration_cast<milliseconds>(Clock::now()-start).count();
        std::cout<<"AUDIT_NATIVE_STOP mode="<<mode<<" elapsed_ms="<<elapsed<<std::endl;
        if(mode=="normal"&&elapsed>1000)return infra("cooperative stop control");
        if(mode=="stall"){
            // Audit bound: do not allow an injected non-cooperative guest
            // to hold the calling host thread beyond the existing 3-s
            // boot-wait bound. This is a bounded synthetic fault, not a
            // claim about natural DOSBox hang frequency.
            const bool pass=elapsed<=3000;
            std::cout<<"AUDIT_A05_NATIVE_STOP_BOUND "<<(pass?"PASS":"FAIL")<<std::endl;
            failures+=!pass;
        }
    }
    std::cout<<"AUDIT_NATIVE_HOST_SHIM_EXECUTED mode="<<mode<<" failures="<<failures<<std::endl;
    return failures?1:0;
}
