#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <istream>
#include <set>
#include <vector>
namespace awe64::qa {
using OwnerProbe=std::array<unsigned,3>; // program, key, nonzero velocity
inline bool readOwnerProbes(std::istream& in,std::vector<OwnerProbe>& result) {
    unsigned n=0;if(!(in>>n)||n<1||n>100000)return false;
    std::vector<OwnerProbe> candidate;std::set<OwnerProbe> unique;
    for(unsigned i=0;i<n;++i){
        OwnerProbe p{};
        if(!(in>>p[0]>>p[1]>>p[2])||p[0]>127||p[1]>127||p[2]<1||p[2]>127||!unique.insert(p).second)return false;
        candidate.push_back(p);
    }
    in>>std::ws;if(!in.eof())return false;
    result.swap(candidate);return true;
}
struct OwnerMetrics {
    std::uint64_t frames=0,jumps=0;
    double energy=0,peak=0,acMax=0,stepMax=0;
    double prevL=0,prevR=0;
    bool previous=false;
    bool add(const float* l,const float* r,unsigned n) noexcept {
        if(!n)return false;
        double sumL=0,sumR=0,blockEnergy=0;
        for(unsigned i=0;i<n;++i){
            const double a=l[i],b=r[i];
            if(!std::isfinite(a)||!std::isfinite(b))return false;
            blockEnergy+=(a*a+b*b)*.5;sumL+=a;sumR+=b;
            peak=std::max({peak,std::abs(a),std::abs(b)});
            if(previous){
                const auto step=std::max(std::abs(a-prevL),std::abs(b-prevR));
                stepMax=std::max(stepMax,step);jumps+=step>.15;
            }
            prevL=a;prevR=b;previous=true;
        }
        const double meanEnergy=(sumL*sumL+sumR*sumR)/(2.*n*n);
        acMax=std::max(acMax,std::sqrt(std::max(0.,blockEnergy/n-meanEnergy)));
        energy+=blockEnergy;frames+=n;return true;
    }
    double rms() const noexcept {return frames?std::sqrt(energy/frames):0.;}
};
}
