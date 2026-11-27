#pragma once
// Non-RT, fallback-only legacy SoundFont loop compatibility. No PCM edits.
// First prove the exact existing validator on a verification copy. Only
// inactive invalid loop metadata may be normalized in the staged bank;
// active 1/2-word zero-guard extensions remain byte-identical in the bank.
#include "Sf2StructuralValidation.hpp"
#include <algorithm>

namespace awe64::guest {
inline bool stageLegacyLoopMetadata(std::vector<std::uint8_t>& bank) {
    using namespace sf2preflight;
    if(bank.size()<12||!eq4(bank.data(),"RIFF")||!eq4(bank.data()+8,"sfbk")||
       std::uint64_t(le32(bank.data()+4))+8!=bank.size())return false;
    Chunk sdta{},pdta{},smpl{};
    if(!walk({bank.data()+12,bank.size()-12},[&](const auto* t,Chunk c){
        if(!eq4(t,"LIST"))return true;
        if(c.size<4)return false;
        Chunk nested{c.p+4,c.size-4};
        if(eq4(c.p,"sdta"))return assign(sdta,nested);
        if(eq4(c.p,"pdta"))return assign(pdta,nested);
        return true;
    })||!sdta.p||!pdta.p||!walk(sdta,[&](const auto* t,Chunk c){
        return !eq4(t,"smpl")||assign(smpl,c);
    })||!smpl.p||smpl.size%2)return false;
    constexpr std::array<const char*,6> names{"pgen","pmod","inst","ibag","imod","igen"};
    constexpr std::array<std::size_t,6> widths{4,10,22,4,10,4};
    std::array<Chunk,6> tables{};
    Chunk shdr{};
    if(!walk(pdta,[&](const auto* t,Chunk c){
        if(eq4(t,"shdr"))return c.size>=92&&c.size%46==0&&assign(shdr,c);
        for(std::size_t i=0;i<names.size();++i)if(eq4(t,names[i]))
            return c.size>=widths[i]&&c.size%widths[i]==0&&assign(tables[i],c);
        return true;
    })||!shdr.p)return false;
    for(auto c:tables)if(!c.p||c.size==0)return false;
    const auto& [pgen,pmod,inst,ibag,imod,igen]=tables;
    const auto sc=shdr.size/46,ic=inst.size/22,bc=ibag.size/4,gc=igen.size/4,words=smpl.size/2;
    if(sc>65536||ic<2||ic>65536||bc<2||bc>65536||gc<2||gc>65536||
       !terminal(inst,22,"EOI")||!terminal(shdr,46,"EOS")||
       le16(inst.p+20)!=0||le16(inst.p+22*(ic-1)+20)!=bc-1||
       le16(ibag.p)!=0||le16(ibag.p+4*(bc-1))!=gc-1)return false;
    auto sampleOp=[](unsigned op){return op==0||op==1||op==2||op==3||op==4||op==12||op==45||op==50||op==53||op==54;};
    for(std::size_t i=0;i+4<pgen.size;i+=4)if(sampleOp(le16(pgen.p+i)))return false;
    for(auto mods:{pmod,imod})for(std::size_t i=0;i+10<mods.size;i+=10)
        if(sampleOp(le16(mods.p+i+2)))return false;
    for(std::size_t b=0;b+1<bc;++b){
        auto first=le16(ibag.p+4*b),last=le16(ibag.p+4*(b+1));
        if(first>last||last>=gc)return false;
    }
    std::vector<std::uint8_t> staged=bank;
    std::bitset<65536> seen,active,activeWithOffsets,guarded;
    auto put16=[&](const std::uint8_t* p,unsigned v){auto i=std::size_t(p-bank.data());staged[i]=std::uint8_t(v);staged[i+1]=std::uint8_t(v>>8);};
    auto put32=[](std::vector<std::uint8_t>& b,std::size_t i,std::uint32_t v){for(unsigned n=0;n<4;++n)b[i+n]=std::uint8_t(v>>(8*n));};
    auto rawLoopBad=[](const std::uint8_t* h){
        const auto a=le32(h+20),b=le32(h+24),c=le32(h+28),d=le32(h+32);
        return (c||d)&&(c<a||d>b||c>=d);
    };
    for(std::size_t i=0;i+1<ic;++i){
        const auto first=le16(inst.p+22*i+20),last=le16(inst.p+22*(i+1)+20);
        if(first>last||last>=bc)return false;
        SampleAddress global{};
        for(std::size_t b=first;b<last;++b){
            const auto g=le16(ibag.p+4*b),end=le16(ibag.p+4*(b+1));
            SampleAddress local{};
            const std::uint8_t* offsets[4]{};
            constexpr unsigned loopOps[]{2,3,45,50};
            unsigned links=0,id=0;
            for(std::size_t j=g;j<end;++j){
                const auto* p=igen.p+4*j;
                const auto op=le16(p),val=le16(p+2);
                local.generator(op,val);
                for(unsigned n=0;n<4;++n)if(op==loopOps[n]){
                    if(offsets[n])return false;
                    offsets[n]=p+2;
                }
                if(op==53){
                    if(val>=sc-1)return false;
                    if(links&&(val!=id||!(le16(shdr.p+46*val+44)&0x8000u)))return false;
                    ++links;id=val;
                }
            }
            if(!links){if(b!=first)return false;global=local;continue;}
            const auto* h=shdr.p+46*id;
            if(le16(h+44)&0x8000u)continue;
            auto v=[&](unsigned n){return local.present.test(n)?local.value[n]:global.value[n];};
            const auto a=std::int64_t(le32(h+20))+v(0)+32768*v(4);
            const auto z=std::int64_t(le32(h+24))+v(1)+32768*v(5);
            const auto c=std::int64_t(le32(h+28))+v(2)+32768*v(6);
            const auto d=std::int64_t(le32(h+32))+v(3)+32768*v(7);
            if(a<0||a>=z||z>std::int64_t(words))return false;
            seen.set(id);
            if(v(8)!=0){
                active.set(id);
                if(v(8)!=1&&v(8)!=3)activeWithOffsets.set(id);
                for(unsigned n=0;n<8;++n)if(v(n))activeWithOffsets.set(id);
                continue;
            }
            if((v(2)||v(3)||v(6)||v(7))&&(rawLoopBad(h)||c<a||c>=d||d>z)){
                constexpr unsigned indices[]{2,3,6,7};
                for(unsigned n=0;n<4;++n){
                    if(offsets[n])put16(offsets[n],0);
                    else if(global.value[indices[n]])return false;
                }
            }
        }
    }
    std::vector<std::size_t> nextStart(sc,words);
    std::size_t next=words;
    bool ordered=true;
    for(std::size_t i=sc-1;i-->0;){
        const auto* h=shdr.p+46*i;
        if(le16(h+44)&0x8000u)continue;
        const auto a=le32(h+20),b=le32(h+24);
        if(a>=b||b>words)return false;
        nextStart[i]=next;
        if(b>next)ordered=false;
        next=a;
    }
    for(std::size_t i=0;i+1<sc;++i){
        const auto* h=shdr.p+46*i;
        if((le16(h+44)&0x8000u)||!rawLoopBad(h))continue;
        if(!seen.test(i))return false;
        const auto a=le32(h+20),b=le32(h+24),c=le32(h+28),d=le32(h+32);
        const auto off=std::size_t(h-bank.data());
        if(!active.test(i)){
            put32(staged,off+28,a);put32(staged,off+32,b);
            continue;
        }
        // Preserve the actual active loop and declared end in the staged
        // bank. The proof copy includes the already-present zero guard.
        if(!ordered||activeWithOffsets.test(i)||c<a||c>=b||d<=b||
           std::uint64_t(d)-b>2||std::uint64_t(b)+8>nextStart[i]||std::uint64_t(b)+8>words)return false;
        for(std::size_t j=b;j<std::size_t(b)+8;++j)if(le16(smpl.p+2*j)!=0)return false;
        guarded.set(i);
    }
    auto proof=staged;
    for(std::size_t i=0;i+1<sc;++i)if(guarded.test(i)){
        const auto* h=shdr.p+46*i;
        put32(proof,std::size_t(h-bank.data())+24,le32(h+32));
    }
    // Every existing hierarchy, terminal, stereo link, sample bounds and
    // effective address check remains binding. Reject unrelated defects.
    if(!validateSf2Structure(proof))return false;
    bank.swap(staged);return true;
}
} // namespace awe64::guest
