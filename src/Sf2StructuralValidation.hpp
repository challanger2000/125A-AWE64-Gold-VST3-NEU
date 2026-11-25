#pragma once
// Non-RT structural preflight for SF2 2.00/2.01. Based on the SF2 Hydra
// table relationships, not a replacement for the original Creative loader.
#include "Sf2CreativeDosCompatibility.hpp"
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace awe64::guest {
namespace sf2preflight {
struct Chunk {const std::uint8_t* p=nullptr; std::size_t size=0;};
// Read all nested chunk boundaries, including odd-byte padding.
template<class Visitor> inline bool walk(Chunk c, Visitor visit) noexcept {
    for(std::size_t i=0;i<c.size;) {
        if(c.size-i<8)return false;
        const auto* tag=c.p+i;
        const std::size_t n=le32(tag+4), start=i+8;
        if(n>c.size-start || (n&1u)>c.size-start-n)return false;
        if(!visit(tag,Chunk{c.p+start,n}))return false;
        i=start+n+(n&1u);
    }
    return true;
}
inline bool assign(Chunk& dest,Chunk c) noexcept {
    if(dest.p)return false;
    dest=c;return true;
}
inline bool terminal(Chunk c,std::size_t stride,const char* name) noexcept {
    return c.size>=stride && c.p[c.size-stride]==name[0] &&
        c.p[c.size-stride+1]==name[1] && c.p[c.size-stride+2]==name[2] &&
        c.p[c.size-stride+3]==0;
}
// SF2 instrument-zone address generators: a local generator supersedes
// the same operator in the instrument global zone; fine and coarse offsets
// combine. Signed arithmetic prevents underflow/wrap at zero.
struct SampleAddress {
    std::array<std::int64_t,9> value{};
    std::bitset<9> present{};
    void generator(unsigned op,unsigned raw) noexcept {
        constexpr unsigned ops[]{0,1,2,3,4,12,45,50,54};
        for(unsigned i=0;i<9;++i)if(ops[i]==op){
            value[i]=raw>=32768?std::int64_t(raw)-65536:raw;
            present.set(i);return;
        }
    }
    bool validRam(const std::uint8_t* s,std::size_t words,const SampleAddress& global) const noexcept {
        auto v=[&](unsigned i){return present.test(i)?value[i]:global.value[i];};
        const std::int64_t a=std::int64_t(le32(s+20))+v(0)+32768*v(4);
        const std::int64_t b=std::int64_t(le32(s+24))+v(1)+32768*v(5);
        const std::int64_t c=std::int64_t(le32(s+28))+v(2)+32768*v(6);
        const std::int64_t d=std::int64_t(le32(s+32))+v(3)+32768*v(7);
        if(a<0||a>=b||b>std::int64_t(words))return false;
        const auto mode=static_cast<unsigned>(v(8))&3u;
        if(mode==1||mode==3||v(2)||v(3)||v(6)||v(7))
            return a<=c&&c<d&&d<=b;
        return true;
    }
};
// The final header references the terminal bag, the final bag references
// terminal generator/modulator records. No index may reference a terminal
// instrument/sample as a real sound. Permit legacy generator ordering and
// global zones; do not reinterpret any generator values here.
inline bool hierarchy(Chunk headers,std::size_t stride,std::size_t offset,
                      Chunk bags,Chunk gens,Chunk mods,unsigned linkOp,
                      std::size_t targetCount, const std::bitset<65536>* required,
                      std::bitset<65536>* referenced, Chunk sampleHeaders={},std::size_t sampleWords=0) noexcept {
    const auto hc=headers.size/stride,bc=bags.size/4,
        gc=gens.size/4,mc=mods.size/10;
    if(hc<2||bc<2||gc<2||mc<1||targetCount<2||
       hc>65536||bc>65536||gc>65536||mc>65536||targetCount>65536)return false;
    if(le16(headers.p+offset)!=0 ||
       le16(headers.p+(hc-1)*stride+offset)!=bc-1 ||
       le16(bags.p)!=0 || le16(bags.p+2)!=0 ||
       le16(bags.p+(bc-1)*4)!=gc-1 ||
       le16(bags.p+(bc-1)*4+2)!=mc-1)return false;
    for(std::size_t b=0;b+1<bc;++b) {
        const auto g=le16(bags.p+b*4),gEnd=le16(bags.p+(b+1)*4);
        const auto m=le16(bags.p+b*4+2),mEnd=le16(bags.p+(b+1)*4+2);
        if(g>gEnd||gEnd>=gc||m>mEnd||mEnd>=mc)return false;
    }
    for(std::size_t h=0;h+1<hc;++h) {
        const auto first=le16(headers.p+h*stride+offset),
            last=le16(headers.p+(h+1)*stride+offset);
        if(first>last||last>=bc)return false;
        bool linked=false;
        SampleAddress globalAddress{};
        for(std::size_t b=first;b<last;++b) {
            const auto g=le16(bags.p+b*4),gEnd=le16(bags.p+(b+1)*4);
            unsigned links=0;
            unsigned firstTarget=0;
            SampleAddress address{};
            for(std::size_t i=g;i<gEnd;++i) {
                const auto op=le16(gens.p+4*i);
                if(sampleHeaders.p)address.generator(op,le16(gens.p+4*i+2));
                if(op!=linkOp)continue;
                const auto target=le16(gens.p+4*i+2);
                if(target>=targetCount-1)return false;
                // Creative SYNTHGM.SF2 has two sampleID generators in one
                // instrument bag, both referencing the same ROM sample.
                // Preserve the original bytes and loader semantics. Do not
                // permit ambiguous links or repeat links to RAM samples.
                if(links && (!sampleHeaders.p || target!=firstTarget ||
                    !(le16(sampleHeaders.p+46u*target+44u)&0x8000u)))return false;
                firstTarget=target;
                ++links;
                if(referenced)referenced->set(target);
            }
            if(links){
                linked=true;
                if(sampleHeaders.p){
                    const auto* sh=sampleHeaders.p+46u*firstTarget;
                    if(!(le16(sh+44)&0x8000u)&&!address.validRam(sh,sampleWords,globalAddress))return false;
                }
            }
            // Only the first zone can be global (no instrument/sample ID).
            else if(b!=first)return false;
            else globalAddress=address;
        }
        // Some legacy banks retain unused empty instruments. They are
        // safe to keep only if no preset refers to them.
        if(!linked&&(!required||required->test(h)))return false;
    }
    return true;
}
} // namespace sf2preflight

inline bool validateSf2Structure(const std::vector<std::uint8_t>& bytes) noexcept {
    using namespace sf2preflight;
    if(bytes.size()<12 || !eq4(bytes.data(),"RIFF") || !eq4(bytes.data()+8,"sfbk") ||
       static_cast<std::uint64_t>(le32(bytes.data()+4))+8!=bytes.size())return false;
    Chunk info{},sdta{},pdta{},smpl{};
    if(!walk({bytes.data()+12,bytes.size()-12},[&](const auto* t,Chunk c){
        if(!eq4(t,"LIST"))return true;
        if(c.size<4)return false;
        const Chunk nested{c.p+4,c.size-4};
        if(eq4(c.p,"INFO"))return assign(info,nested);
        if(eq4(c.p,"sdta"))return assign(sdta,nested);
        if(eq4(c.p,"pdta"))return assign(pdta,nested);
        return true;
    }) || !info.p || !pdta.p)return false;
    if(sdta.p && !walk(sdta,[&](const auto* t,Chunk c){
        return !eq4(t,"smpl") || assign(smpl,c);
    }))return false;
    if(smpl.size%2)return false;
    constexpr std::array<const char*,9> names{"phdr","pbag","pmod","pgen","inst","ibag","imod","igen","shdr"};
    constexpr std::array<std::size_t,9> widths{38,4,10,4,22,4,10,4,46};
    std::array<Chunk,9> tables{};
    if(!walk(pdta,[&](const auto* t,Chunk c){
        for(std::size_t i=0;i<names.size();++i)if(eq4(t,names[i]))
            return c.size>=widths[i] && c.size%widths[i]==0 && assign(tables[i],c);
        return true;
    }))return false;
    for(const auto& t:tables)if(!t.p)return false;
    const auto& [phdr,pbag,pmod,pgen,inst,ibag,imod,igen,shdr]=tables;
    if(!terminal(phdr,38,"EOP")||!terminal(inst,22,"EOI")||!terminal(shdr,46,"EOS"))return false;
    std::bitset<65536> referencedInstruments;
    if(!hierarchy(phdr,38,24,pbag,pgen,pmod,41,inst.size/22,nullptr,&referencedInstruments)||
       !hierarchy(inst,22,20,ibag,igen,imod,53,shdr.size/46,&referencedInstruments,nullptr,shdr,smpl.size/2))return false;
    const auto count=shdr.size/46;
    for(std::size_t i=0;i+1<count;++i) {
        const auto* s=shdr.p+46*i;
        const auto start=le32(s+20),end=le32(s+24),rate=le32(s+36);
        const auto type=le16(s+44); const auto kind=type&0x7fffu;
        if(kind!=1&&kind!=2&&kind!=4&&kind!=8)return false;
        if(start>=end||rate==0)return false;
        if(!(type&0x8000u)) {
            if(!smpl.p||end>smpl.size/2)return false;
            const auto loopStart=le32(s+28),loopEnd=le32(s+32);
            // Legacy unlooped samples may have both loop points zero.
            // Otherwise bounds are absolute sample indices, end exclusive.
            if((loopStart||loopEnd)&&(loopStart<start||loopEnd>end||loopStart>=loopEnd))return false;
        }
        if(kind!=1 && le16(s+42)>=count-1)return false;
    }
    return true;
}
} // namespace awe64::guest
