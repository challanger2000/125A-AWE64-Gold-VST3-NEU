#pragma once
#include "MidiTimeline.hpp"
#include "CreativeMidiBankSelect.hpp"
#include "CreativeMidiControllers.hpp"
#include "CreativeEffectMidi.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
namespace awe64::rt {
// Snapshot of audible state, not a VST3 controller or a guest implementation.
// Discrete MIDI/quantized overlay points are held until their next explicit
// point. Master gain is applied to output PCM separately with interpolation.
struct HostControlState {
    std::uint8_t program=0,reverbType=4,chorusType=2,reverbSend=0,chorusSend=0;
    std::array<std::uint8_t,10> voice{};
    float master=1.f;
};
class HostEventPlan final {
    static constexpr unsigned capacity=4096;
    enum class Kind : std::uint8_t {Control, Mapped, Note};
    struct Item {unsigned offset,ordinal,id; double value; MidiEvent midi; Kind kind;};
    std::array<Item,capacity> items_{};
    unsigned count_=0, frames_=0;
    bool invalid_=false;
    HostControlState last_{};
    bool haveLast_=false;
    static bool ordinary(unsigned id) noexcept {
        return id==1000||id==1001||id==1002||(id>=1100&&id<=1111)||id==1200||id==1201;
    }
    bool validOffset(int offset) const noexcept {
        return offset>=0 && (frames_?static_cast<unsigned>(offset)<frames_:offset==0);
    }
    bool append(Item i) noexcept {
        if(invalid_||count_>=capacity){invalid_=true;return false;}
        i.ordinal=count_;items_[count_++]=i;return true;
    }
    static unsigned quantize(double v,unsigned max) noexcept {
        return static_cast<unsigned>(std::lround(std::clamp(v,0.,1.)*max));
    }
    template<class ProgramAllowed>
    static void update(HostControlState& state,unsigned id,double v,ProgramAllowed&& allowed) noexcept {
        if(id==1000){auto p=quantize(v,127);if(allowed(p))state.program=static_cast<std::uint8_t>(p);}
        else if(id==1001)state.master=static_cast<float>(std::clamp(v,0.,1.));
        else if(id>=1100&&id<1110)state.voice[id-1100]=static_cast<std::uint8_t>(quantize(v,255));
        else if(id==1110)state.reverbSend=static_cast<std::uint8_t>(quantize(v,255));
        else if(id==1111)state.chorusSend=static_cast<std::uint8_t>(quantize(v,255));
        else if(id==1200)state.reverbType=static_cast<std::uint8_t>(quantize(v,7));
        else if(id==1201)state.chorusType=static_cast<std::uint8_t>(quantize(v,7));
    }
    static bool emit(MidiBlockBatch& batch,std::uint64_t at,const HostControlState& state,
                     const HostControlState& old,bool force) noexcept {
        bool ok=true;
        auto add=[&](const MidiEvent& e){ok=batch.add(e,MidiPhase::Setup)&&ok;};
        if(force)add(makeCreativeBankSelect(at,0));
        if(force||state.program!=old.program)add(makeCreativeProgramChange(at,state.program));
        if(force||state.voice!=old.voice){
            std::uint64_t lo=0;for(unsigned n=0;n<8;++n)lo|=std::uint64_t(state.voice[n])<<(8*n);
            unsigned hi=unsigned(state.voice[8])|(unsigned(state.voice[9])<<8);
            add(makeVoiceControlCommand(at,lo,hi));
        }
        if(force||state.reverbSend!=old.reverbSend||state.chorusSend!=old.chorusSend)
            add(makeAbsoluteSendCommand(at,unsigned(state.reverbSend)|(unsigned(state.chorusSend)<<8)));
        if(force||state.reverbType!=old.reverbType)add(makeCreativeEffectType(at,false,state.reverbType));
        if(force||state.chorusType!=old.chorusType)add(makeCreativeEffectType(at,true,state.chorusType));
        return ok;
    }
public:
    void begin(unsigned frames) noexcept {frames_=frames;count_=0;invalid_=false;}
    void reset() noexcept {haveLast_=false;begin(0);}
    // Invalid/nonfinite known events make this batch fail closed. Unknown
    // parameters are ignored, and unsupported bank edits remain ignored.
    bool parameter(int offset,unsigned id,double normalized) noexcept {
        MidiEvent e{};
        const bool mapped=id>=midiControllerBase && id<midiControllerBase+midiControllerStride*midiControllerSpecs.size();
        if(!ordinary(id)&&!mapped)return true;
        if(!validOffset(offset)||!std::isfinite(normalized)){invalid_=true;return false;}
        if(mapped&&!makeMappedController(id,normalized,0,e)){invalid_=true;return false;}
        return append({static_cast<unsigned>(offset),0,id,normalized,e,mapped?Kind::Mapped:Kind::Control});
    }
    bool note(int offset,int channel,int pitch,double velocity,bool on) noexcept {
        if(!validOffset(offset)||channel<0||channel>15||pitch<0||pitch>127||!std::isfinite(velocity)){
            invalid_=true;return false;
        }
        MidiEvent e{0,static_cast<std::uint8_t>((on?0x90:0x80)|channel),static_cast<std::uint8_t>(pitch),
                    static_cast<std::uint8_t>(quantize(velocity,127)),3};
        return append({static_cast<unsigned>(offset),0,0,0.,e,Kind::Note});
    }
    void reject() noexcept {invalid_=true;}
    // Preallocated, chronological processing. Caller publishes state only
    // after build; transport commit ACK is separate. No guest I/O here.
    template<class ProgramAllowed>
    bool build(std::uint64_t start,HostControlState& state,MidiBlockBatch& batch,
               ProgramAllowed&& allowed) noexcept {
        batch.reset();
        if(invalid_||start>std::numeric_limits<std::uint64_t>::max()-frames_)return false;
        auto less=[](const Item& a,const Item& b) noexcept {
            if(a.offset!=b.offset)return a.offset<b.offset;
            if(a.kind!=b.kind)return a.kind<b.kind;
            return a.ordinal<b.ordinal;
        };
        std::make_heap(items_.begin(),items_.begin()+count_,less);
        std::sort_heap(items_.begin(),items_.begin()+count_,less);
        unsigned i=0;
        bool force=!haveLast_,ok=true;
        HostControlState prior=last_;
        // Merge all offset-zero control points BEFORE initial setup. This
        // prevents a redundant program selection or stale overlay command.
        if(!count_||items_[0].offset!=0){ok=emit(batch,start,state,prior,force);prior=state;force=false;}
        while(i<count_){
            const unsigned offset=items_[i].offset;unsigned end=i;
            while(end<count_&&items_[end].offset==offset)++end;
            const auto at=start+offset;
            unsigned next=i;
            for(;next<end&&items_[next].kind==Kind::Control;++next)
                update(state,items_[next].id,items_[next].value,allowed);
            ok=emit(batch,at,state,prior,force)&&ok;
            prior=state;force=false;
            for(;next<end;++next){
                auto e=items_[next].midi;e.absoluteHostSample=at;
                ok=batch.add(e,items_[next].kind==Kind::Mapped?MidiPhase::Controller:MidiPhase::Note)&&ok;
            }
            i=end;
        }
        return ok;
    }
    void committed(const HostControlState& state) noexcept {last_=state;haveLast_=true;}
    void invalidateCommit() noexcept {haveLast_=false;}
    // Master gain is not a MIDI event. Interpolate in host output sample
    // domain between explicit VST3 points; previous block's last value is
    // the virtual point at -1. Multiple points at one offset: last wins.
    // Must follow build(), which sorts inputs chronologically.
    void applyMaster(float* l,float* r,float previous) const noexcept {
        if(!l||!r)return;
        float gain=previous;int prior=-1;unsigned pos=0;
        for(unsigned i=0;i<count_;++i){
            if(items_[i].kind!=Kind::Control||items_[i].id!=1001)continue;
            unsigned j=i;
            for(unsigned k=i+1;k<count_&&items_[k].offset==items_[i].offset;++k)
                if(items_[k].kind==Kind::Control&&items_[k].id==1001)j=k;
            const int end=static_cast<int>(items_[i].offset);
            const float target=static_cast<float>(std::clamp(items_[j].value,0.,1.));
            for(;pos<frames_&&static_cast<int>(pos)<=end;++pos){
                const float frac=float(static_cast<int>(pos)-prior)/float(end-prior);
                const float v=gain+(target-gain)*frac;l[pos]*=v;r[pos]*=v;
            }
            gain=target;prior=end;i=j;
        }
        for(;pos<frames_;++pos){l[pos]*=gain;r[pos]*=gain;}
    }
};
}
