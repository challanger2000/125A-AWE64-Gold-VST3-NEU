#pragma once
#include "HostEventPlan.hpp"
#include "GuestDemandClock.hpp"
#include <cstdint>
#include <limits>
namespace awe64::rt {
// Host-side clock epoch. Only the VST3 audio consumer calls begin/submit;
// configure/reset is non-concurrent with process (lifecycle serialization).
// Does not spin, wait, perform I/O, or call emulator code.
class HostDemandSession final {
    MidiTimeline timeline_{};
    HostEventPlan plan_{};
    MidiBlockBatch batch_{};
    unsigned rate_=0,maxBlock_=0,lead_=0,frames_=0;
    std::uint64_t position_=0,recoveryEpoch_=0;
    bool begun_=false,active_=false,fault_=false,validPlan_=false;
    static bool nativeCeil(std::uint64_t samples,unsigned rate,std::uint64_t& out) noexcept {
        if(!rate)return false;
        const auto whole=(samples/rate)*44100u;
        const auto rest=((samples%rate)*44100u+rate-1u)/rate;
        if(whole>std::numeric_limits<std::uint64_t>::max()-rest)return false;
        out=whole+rest;return true;
    }
public:
    enum class Result { Accepted, Recovery, Fault };
    bool configure(unsigned rate,unsigned maxBlock) noexcept {
        reset();rate_=maxBlock_=lead_=0;
        if(!GuestDemandClock::leadFor(rate,maxBlock,lead_) ||
           lead_>=GuestDemandClock::ringFrames/2 || !timeline_.configure(rate))return false;
        rate_=rate;maxBlock_=maxBlock;return true;
    }
    unsigned lead() const noexcept {return lead_;}
    unsigned latencySamples() const noexcept {
        return lead_?static_cast<unsigned>((std::uint64_t(lead_)*rate_+44100u-1u)/44100u):0u;
    }
    std::uint64_t position() const noexcept {return position_;}
    bool faulted() const noexcept {return fault_;}
    void fail() noexcept {fault_=true;}
    void reset() noexcept {
        timeline_.reset();plan_.reset();batch_.reset();
        position_=recoveryEpoch_=0;frames_=0;begun_=active_=fault_=validPlan_=false;
    }
    bool activate() noexcept {
        reset();if(!lead_||!timeline_.anchor(0,0,lead_))return false;
        active_=true;return true;
    }
    bool begin(unsigned frames) noexcept {
        if(fault_||!active_||begun_||frames>maxBlock_){fail();return false;}
        frames_=frames;begun_=true;plan_.begin(frames);return true;
    }
    bool parameter(int offset,unsigned id,double value) noexcept {return begun_&&plan_.parameter(offset,id,value);}
    bool note(int offset,int ch,int pitch,double v,bool on) noexcept {return begun_&&plan_.note(offset,ch,pitch,v,on);}
    void reject() noexcept {plan_.reject();}
    // Engine contract: submitTimed queues complete ordered packets BEFORE
    // publishDemand makes the next virtual tick legal. Even overflow recovery
    // advances the horizon, so the guest can finish its release sequence.
    template<class Engine,class Allowed>
    Result submit(Engine& engine,HostControlState& state,Allowed&& allowed) noexcept {
        if(!begun_||fault_){fail();return Result::Fault;}
        begun_=false;
        if(position_>std::numeric_limits<std::uint64_t>::max()-frames_){fail();return Result::Fault;}
        const auto end=position_+frames_;
        std::uint64_t native=0,horizon=0;
        if(!nativeCeil(end,rate_,native)||!GuestDemandClock::blockHorizon(native,lead_,horizon)){
            fail();return Result::Fault;
        }
        const auto epoch=engine.midiRecoveryEpoch();
        if(epoch!=recoveryEpoch_){plan_.invalidateCommit();recoveryEpoch_=epoch;}
        HostControlState next=state;
        const bool built=plan_.build(position_,next,batch_,allowed);
        validPlan_=built;
        bool submitted=false;
        if(built){
            state=next; // accepted host controls remain authoritative during recovery
            submitted=engine.submitTimed(batch_,timeline_);
        } else {
            batch_.reset();engine.requestMidiRecovery();
        }
        if(submitted)plan_.committed(state);else plan_.invalidateCommit();
        if(!engine.publishDemand(horizon)){fail();return Result::Fault;}
        position_=end;
        return submitted?Result::Accepted:Result::Recovery;
    }
    void applyMaster(float* left,float* right,float previous) const noexcept {
        if(validPlan_)plan_.applyMaster(left,right,previous);
        else if(left&&right)for(unsigned i=0;i<frames_;++i){left[i]*=previous;right[i]*=previous;}
    }
};
}
