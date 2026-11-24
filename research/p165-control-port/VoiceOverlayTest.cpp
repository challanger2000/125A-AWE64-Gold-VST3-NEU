#include "VoiceOverlay.hpp"
#include <cassert>
#include <iostream>
using namespace awe64::overlay;
int main(){
    Controls zero{};
    for(unsigned w=0;w<65536;++w){
        assert(delay(w,0)==w && attack(w,0)==w && resonance(w,0)==w);
        assert(decaySustainRelease(w,0,0,0)==w);
        assert(filter(w,w,zero).ifatn==w && filter(w,w,zero).pefe==w);
        for(unsigned a: {1u,64u,128u,192u,255u}){
            auto at=attack(w,a),dr=decaySustainRelease(w,a,a,a),q=resonance(w,a);
            assert((at&0x8080)==(w&0x8080));
            assert((dr&0x8080)==(w&0x8080));
            assert((q&0xfff)==(w&0xfff));
            assert((at&127)>=(w&127));
            assert((dr&127)>=(w&127));
            if(w&0x8000){assert(delay(w,a)==w);assert((dr&0xff80)==(w&0xff80));}
            else {assert(delay(w,a)>=w);assert(((dr>>8)&127)<=((w>>8)&127));}
            Controls c{};c[4]=a;c[8]=a;
            auto f=filter(w,w,c);
            assert((f.ifatn&255)==(w&255));
            assert((f.pefe&0xff00)==(w&0xff00));
            assert((f.ifatn>>8)<=(w>>8));
            if(w&255)assert(f.pefe==w);
        }
        assert((attack(w,255)&0x7f7f)==0x7f7f);
        assert((resonance(w,255)>>12)==15);
        if(!(w&0x8000))assert(delay(w,255)==0x8000);
    }
    Controls c{};c[4]=255;
    for(unsigned cutoff=0;cutoff<256;++cutoff){
        auto f=filter((cutoff<<8)|37,0x8100,c);
        assert(unsigned(f.ifatn>>8)+2u*(f.pefe&255u)==cutoff);
        assert((f.pefe&255)<=96);
    }
    for(unsigned base=0;base<=127;++base)for(unsigned a=1;a<=255;++a){
        assert(faster(base,a)>=faster(base,a-1));
        assert(scale(base,a)>=scale(base,a-1));
    }
    std::cout<<"P165_RAW_REGISTER_MATH_PASS: exhaustive 65536 words; neutral, flags, endpoints, monotonicity, dark filter route\n";
}
