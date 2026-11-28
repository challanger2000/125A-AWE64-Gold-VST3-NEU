#include "VoiceOverlay.hpp"
#include <cstdio>
#include <cstdlib>
using namespace awe64::overlay;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
// Exact divisor from the pinned EMU8000 initialization code (rate 0 stops).
constexpr unsigned divisor(unsigned rate) noexcept {
    const unsigned idx=rate-1u,group=(idx>>4)&7u,m=idx&15u;
    return group==0?m+1u:(m+17u)<<(group-1u);
}
int main(){
    const Controls zero{};
    for(unsigned w=0;w<65536;++w){
        CHECK(delay(w,0)==w && attack(w,0)==w && resonance(w,0)==w);
        CHECK(decaySustainRelease(w,0,0,0)==w);
        CHECK(filter(w,w,zero).ifatn==w && filter(w,w,zero).pefe==w);
        for(unsigned a: {1u,2u,64u,128u,192u,254u,255u}){
            auto at=attack(w,a),dr=decaySustainRelease(w,a,a,a),q=resonance(w,a);
            CHECK((at&0x8080)==(w&0x8080));
            CHECK((at&0x7f00)==0x7f00);
            CHECK((dr&0x8080)==(w&0x8080));
            CHECK((q&0xfff)==(w&0xfff));
            CHECK((at&127)>=6&&(at&127)<=127);
            CHECK((dr&127)>=12&&(dr&127)<=127);
            CHECK(delay(w,a)==((w&0x8000u)?w:0x8000u));
            if(w&0x8000){
                CHECK((dr&0xff80)==(w&0xff80));
                CHECK(decaySustainRelease(w,a,a,0)==w);
            }else{
                CHECK(((dr>>8)&127)==activeValue(a,127u));
                CHECK(decaySustainRelease(w,0,0,a)==w);
                CHECK((decaySustainRelease(w,a,0,0)&0xff80)==(w&0xff80));
                CHECK((decaySustainRelease(w,0,a,0)&0x80ff)==(w&0x80ff));
            }
            Controls c{};c[8]=a;
            const auto f=filter(w,w,c);
            CHECK((f.ifatn&255)==(w&255));CHECK(f.pefe==w);
            CHECK((f.ifatn>>8)==activeValue(a,255u));
            if(a>1){
                CHECK((at&127)<=(attack(w,a-1)&127));
                CHECK((dr&127)<=(decaySustainRelease(w,a-1,a-1,a-1)&127));
                CHECK((q>>12)>=(resonance(w,a-1)>>12));
            }
        }
        CHECK((attack(w,1)&127)==127&&(attack(w,255)&127)==6);
        CHECK((decaySustainRelease(w,1,1,1)&127)==127&&(decaySustainRelease(w,255,255,255)&127)==12);
        CHECK((resonance(w,1)>>12)==0&&(resonance(w,255)>>12)==15);
        if(!(w&0x8000u)){
            CHECK((decaySustainRelease(w,0,1,0)&0x7f00)==0);
            CHECK((decaySustainRelease(w,0,255,0)&0x7f00)==0x7f00);
        }
    }
    // All active positions and all possible original rates. No stopped
    // envelopes, including formerly stopped or already-fast presets.
    for(unsigned base=0;base<=127;++base)for(unsigned a=1;a<=255;++a){
        const auto at=attack(base,a)&127u;
        const auto dr=decaySustainRelease(base,a,0,a)&127u;
        CHECK(at&&dr);
        if(a>1){
            CHECK(divisor(at)<=divisor(attack(base,a-1)&127u));
            CHECK(divisor(dr)<=divisor(decaySustainRelease(base,a-1,0,a-1)&127u));
            CHECK(activeValue(a,127)>=activeValue(a-1,127));
        }
    }
    // All original cutoff and signed filter-route values, across active
    // positions; no route/pitch destruction and no reversed cutoff travel.
    for(unsigned cutoff=0;cutoff<256;++cutoff)for(unsigned route=0;route<256;++route){
        const auto ifatn=std::uint16_t((cutoff<<8)|37u),pefe=std::uint16_t(0x8100u|route);
        Controls c{};c[4]=1;
        auto first=filter(ifatn,pefe,c);
        if(!route)CHECK(unsigned(first.ifatn>>8)+2u*(first.pefe&255u)==cutoff);
        unsigned previous=0;
        for(unsigned a=1;a<=255;++a){
            c[8]=static_cast<std::uint8_t>(a);const auto f=filter(ifatn,pefe,c);
            CHECK((f.ifatn&255u)==37u&&(f.pefe&0xff00u)==0x8100u);
            CHECK(unsigned(f.ifatn>>8)>=previous);previous=f.ifatn>>8;
            if(route){CHECK(f.pefe==pefe);CHECK(unsigned(f.ifatn>>8)==activeValue(a,255u));}
            else {CHECK((f.pefe&255u)<=64u);const auto target=activeValue(a,255u),peak=unsigned(f.ifatn>>8)+2u*(f.pefe&255u);CHECK(peak<=target&&target-peak<=1u);}
            c[4]=255;const auto second=filter(ifatn,pefe,c);c[4]=1;
            CHECK(f.ifatn==second.ifatn&&f.pefe==second.pefe);
        }
    }
    CHECK(divisor(127)==1984&&divisor(6)==6&&divisor(12)==12);
    std::puts("P165_RAW_REGISTER_MATH_PASS: 65536 neutral/flag words; active ADSR time and sustain direction; 16711680 cutoff/route combinations; independent controls");
}
