#pragma once
// Private SF2 staging only: avoid the measured Creative DOS loader's
// 20-generator preset-zone limit. Never edit PCM or the owner's file.
// Move only equivalent additive generators into exclusively owned
// instrument global zones; preserve effective local overrides and ranges.
// No speculative rewriting of shared instruments or modulated parameters.
#include "Sf2LegacyLoopCompatibility.hpp"
#include <algorithm>
#include <limits>
#include <utility>
namespace awe64::guest {
namespace sf2budget {
using Bytes=std::vector<std::uint8_t>;
struct Gen {unsigned op,raw; int value() const {return raw>=32768?int(raw)-65536:int(raw);}};
struct Zone {std::vector<Gen> gen;Bytes mod;};
using Group=std::vector<Zone>;
inline const Gen* find(const Zone& z,unsigned op) {
    for(const auto& g:z.gen)if(g.op==op)return &g;
    return nullptr;
}
inline unsigned count(const Zone& z,unsigned op) {
    return static_cast<unsigned>(std::count_if(z.gen.begin(),z.gen.end(),[&](Gen g){return g.op==op;}));
}
inline void erase(Zone& z,unsigned op) {
    z.gen.erase(std::remove_if(z.gen.begin(),z.gen.end(),[&](Gen g){return g.op==op;}),z.gen.end());
}
inline void append16(Bytes& b,unsigned v){b.push_back(v&255);b.push_back((v>>8)&255);}
inline void append32(Bytes& b,std::uint32_t v){for(int i=0;i<4;++i)b.push_back((v>>(8*i))&255);}
inline void put16(Bytes& b,std::size_t p,unsigned v){b[p]=v&255;b[p+1]=(v>>8)&255;}
inline void put32(Bytes& b,std::size_t p,std::uint32_t v){for(int i=0;i<4;++i)b[p+i]=(v>>(8*i))&255;}
inline void chunk(Bytes& b,const std::uint8_t* tag,const Bytes& data){
    b.insert(b.end(),tag,tag+4);append32(b,static_cast<std::uint32_t>(data.size()));
    b.insert(b.end(),data.begin(),data.end());if(data.size()&1)b.push_back(0);
}
struct Tables {sf2preflight::Chunk hdr,bag,mod,gen;std::size_t stride,offset;};
inline bool parse(const Tables& t,std::vector<Group>& out){
    const auto hc=t.hdr.size/t.stride,bc=t.bag.size/4,gc=t.gen.size/4,mc=t.mod.size/10;
    if(hc<2||hc>65536||bc<2||bc>65536||gc<2||gc>65536||mc<1||mc>65536||
       le16(t.hdr.p+t.offset)!=0||le16(t.hdr.p+(hc-1)*t.stride+t.offset)!=bc-1||
       le16(t.bag.p)!=0||le16(t.bag.p+2)!=0||
       le16(t.bag.p+(bc-1)*4)!=gc-1||le16(t.bag.p+(bc-1)*4+2)!=mc-1)return false;
    for(std::size_t i=0;i+1<hc;++i){
        auto first=le16(t.hdr.p+i*t.stride+t.offset),last=le16(t.hdr.p+(i+1)*t.stride+t.offset);
        if(first>last||last>=bc)return false;
        Group group;
        for(std::size_t j=first;j<last;++j){
            auto a=le16(t.bag.p+4*j),b=le16(t.bag.p+4*(j+1)),c=le16(t.bag.p+4*j+2),d=le16(t.bag.p+4*(j+1)+2);
            if(a>b||b>=gc||c>d||d>=mc)return false;
            Zone z;
            for(std::size_t k=a;k<b;++k)z.gen.push_back({le16(t.gen.p+4*k),le16(t.gen.p+4*k+2)});
            z.mod.assign(t.mod.p+10*c,t.mod.p+10*d);group.push_back(std::move(z));
        }
        out.push_back(std::move(group));
    }return true;
}
inline bool serialize(const Tables& t,const std::vector<Group>& groups,std::array<Bytes,4>& out){
    auto& [hdr,bag,mod,gen]=out;
    hdr.assign(t.hdr.p,t.hdr.p+t.hdr.size);
    for(std::size_t i=0;i<groups.size();++i){
        if(bag.size()/4>65535)return false;
        put16(hdr,i*t.stride+t.offset,static_cast<unsigned>(bag.size()/4));
        for(const auto& z:groups[i]){
            if(gen.size()/4>65535||mod.size()/10>65535)return false;
            append16(bag,static_cast<unsigned>(gen.size()/4));append16(bag,static_cast<unsigned>(mod.size()/10));
            for(auto g:z.gen){append16(gen,g.op);append16(gen,g.raw);}
            mod.insert(mod.end(),z.mod.begin(),z.mod.end());
        }
    }
    if(bag.size()/4>65535||gen.size()/4>65535||mod.size()/10>65535)return false;
    put16(hdr,groups.size()*t.stride+t.offset,static_cast<unsigned>(bag.size()/4));
    append16(bag,static_cast<unsigned>(gen.size()/4));append16(bag,static_cast<unsigned>(mod.size()/10));
    gen.insert(gen.end(),t.gen.p+t.gen.size-4,t.gen.p+t.gen.size);
    mod.insert(mod.end(),t.mod.p+t.mod.size-10,t.mod.p+t.mod.size);
    return true;
}
struct Range{unsigned op;int lo,hi;};
// Only SF2 additive operators whose implicit instrument default is zero.
// Explicit existing instrument values are allowed if the exact sum is legal.
inline constexpr Range candidates[]={
    {51,-120,120},{16,0,1000},{22,-16000,4500},{13,-960,960},
    {24,-16000,4500},{6,-12000,12000},{11,-12000,12000},
    {10,-12000,12000},{5,-12000,12000},{7,-12000,12000},
    {37,0,1440},{29,0,1000},{15,0,1000},{48,0,1440},{17,-500,500}
};
inline bool noMods(const Group& g){for(const auto& z:g)if(!z.mod.empty())return false;return true;}
inline bool uniqueOps(const Group& g) {
    for(const auto& z:g){std::bitset<65536> seen;for(auto a:z.gen){if(seen.test(a.op))return false;seen.set(a.op);}}
    return true;
}
} // namespace sf2budget
inline bool stageCreativePresetBudget(std::vector<std::uint8_t>& bank) {
    using namespace sf2budget;using namespace sf2preflight;
    if(bank.size()<12||!eq4(bank.data(),"RIFF")||!eq4(bank.data()+8,"sfbk")||std::uint64_t(le32(bank.data()+4))+8!=bank.size())return false;
    Chunk pdta{};
    if(!walk({bank.data()+12,bank.size()-12},[&](const auto* tag,Chunk c){
        if(!eq4(tag,"LIST"))return true;
        if(c.size<4)return false;
        return !eq4(c.p,"pdta")||assign(pdta,{c.p+4,c.size-4});
    })||!pdta.p)return false;
    constexpr const char* names[]={"phdr","pbag","pmod","pgen","inst","ibag","imod","igen","shdr"};
    constexpr std::size_t widths[]={38,4,10,4,22,4,10,4,46};
    std::array<Chunk,9> t{};
    if(!walk(pdta,[&](const auto* tag,Chunk c){
        for(unsigned j=0;j<9;++j)if(eq4(tag,names[j]))return c.size>=widths[j]&&c.size%widths[j]==0&&assign(t[j],c);
        return true;
    }))return false;
    for(auto c:t)if(!c.p)return false;
    const Tables pt{t[0],t[1],t[2],t[3],38,24},it{t[4],t[5],t[6],t[7],22,20};
    std::vector<Group> p,i;
    if(!parse(pt,p)||!parse(it,i))return false;
    std::vector<std::size_t> owner(i.size(),std::numeric_limits<std::size_t>::max());
    std::vector<bool> shared(i.size(),false);
    for(std::size_t n=0;n<p.size();++n)for(auto& z:p[n])for(auto g:z.gen)if(g.op==41){
        if(g.raw>=i.size())return false;
        auto& o=owner[g.raw];if(o==std::numeric_limits<std::size_t>::max())o=n;else if(o!=n)shared[g.raw]=true;
    }
    bool changed=false;
    for(std::size_t n=0;n<p.size();++n){
        auto& pg=p[n];
        if(pg.size()<2||find(pg.front(),41))continue;
        std::size_t maxCount=0;
        for(std::size_t z=1;z<pg.size();++z)maxCount=std::max(maxCount,pg[0].gen.size()+pg[z].gen.size());
        if(maxCount<=20||!noMods(pg)||!uniqueOps(pg))continue;
        std::vector<unsigned> targets;
        bool safe=true;
        for(std::size_t z=1;z<pg.size();++z){
            auto* link=find(pg[z],41);
            if(!link||shared[link->raw]||owner[link->raw]!=n){safe=false;break;}
            const auto& ig=i[link->raw];
            if(ig.size()<2||find(ig[0],53)||!noMods(ig)||!uniqueOps(ig)){safe=false;break;}
            if(std::find(targets.begin(),targets.end(),link->raw)==targets.end())targets.push_back(link->raw);
        }
        if(!safe||targets.empty())continue;
        std::vector<unsigned> moves;
        for(auto c:candidates){
            const auto* g=find(pg[0],c.op);if(!g)continue;
            const int v=g->value();
            bool can=true;
            for(std::size_t z=1;z<pg.size();++z)if(const auto* local=find(pg[z],c.op);local&&local->value()!=v)can=false;
            for(auto k:targets){
                const auto& ig=i[k];
                for(std::size_t z=1;z<ig.size();++z)if(find(ig[z],c.op))can=false;
                const auto* old=find(ig[0],c.op);
                const int sum=(old?old->value():0)+v;
                if(sum<c.lo||sum>c.hi)can=false;
            }
            if(can)moves.push_back(c.op);
            if(moves.size()>=std::max<std::size_t>(4,maxCount-20))break;
        }
        if(moves.size()<maxCount-20||moves.size()>=pg[0].gen.size())continue;
        for(auto op:moves){
            const int v=find(pg[0],op)->value();
            for(auto k:targets){
                auto& z=i[k][0];const auto* old=find(z,op);
                const int sum=(old?old->value():0)+v;
                erase(z,op);z.gen.push_back({op,static_cast<unsigned>(sum)&65535u});
            }
            for(auto& z:pg)erase(z,op);
        }
        // Zero local loop offsets with no inherited value are exactly the
        // implicit default. Avoid consuming scarce Creative instrument slots.
        for(auto k:targets)for(std::size_t z=1;z<i[k].size();++z){
            auto& g=i[k][z].gen;const auto& global=i[k][0];
            g.erase(std::remove_if(g.begin(),g.end(),[&](Gen a){
                return (a.op==2||a.op==3||a.op==45||a.op==50)&&a.raw==0&&!find(global,a.op);
            }),g.end());
        }
        changed=true;
    }
    if(!changed)return true;
    std::array<Bytes,4> np{},ni{};
    if(!serialize(pt,p,np)||!serialize(it,i,ni))return false;
    // Keep the original chunk ordering and every unmodified byte (including
    // INFO, sample RAM, sample headers and unknown chunks).
    Bytes list{'p','d','t','a'};
    for(std::size_t off=0;off<pdta.size;){
        const auto* tag=pdta.p+off;const auto sz=le32(tag+4);unsigned j=0;
        for(;j<8;++j)if(eq4(tag,names[j]))break;
        if(j<8)chunk(list,tag,j<4?np[j]:ni[j-4]);
        else list.insert(list.end(),tag,tag+8+sz+(sz&1u));
        off+=8+sz+(sz&1u);
    }
    const auto start=static_cast<std::size_t>(pdta.p-bank.data())-12;
    const auto oldEnd=start+12+pdta.size+(pdta.size&1u);
    if(start<12||oldEnd>bank.size())return false;
    Bytes result(bank.begin(),bank.begin()+start);
    chunk(result,reinterpret_cast<const std::uint8_t*>("LIST"),list);
    result.insert(result.end(),bank.begin()+oldEnd,bank.end());
    if(result.size()>32ull*1024*1024)return false;
    put32(result,4,static_cast<std::uint32_t>(result.size()-8));
    if(!validateSf2Structure(result)&&!stageLegacyLoopMetadata(result))return false;
    bank.swap(result);return true;
}
} // namespace awe64::guest
