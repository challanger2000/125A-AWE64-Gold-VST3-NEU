#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace awe64::rt {
// Fixed storage, no allocation. VST3 ordered automation points, linear
// interpolation between points, last point wins for duplicate offsets.
class MasterGainPlan {
    struct Point {int offset;float gain;};
    std::array<Point,4096> points_{};
    unsigned size_=0;
    int frames_=0;
    float previous_=1.f;
public:
    void begin(int frames,float previous) noexcept {frames_=frames;previous_=previous;size_=0;}
    bool add(int offset,double value) noexcept {
        if(offset<0||(frames_?offset>=frames_:offset!=0)||!std::isfinite(value))return false;
        if(size_&&offset<points_[size_-1].offset)return false;
        const float g=static_cast<float>(std::clamp(value,0.,1.));
        if(size_&&offset==points_[size_-1].offset){points_[size_-1].gain=g;return true;}
        if(size_>=points_.size())return false;
        points_[size_++]={offset,g};return true;
    }
    float last() const noexcept {return size_?points_[size_-1].gain:previous_;}
    void apply(float* l,float* r) const noexcept {
        float g=previous_;int prior=-1,pos=0;
        for(unsigned i=0;i<size_;++i){
            const auto p=points_[i];
            for(;pos<frames_&&pos<=p.offset;++pos){
                const float fraction=float(pos-prior)/float(p.offset-prior);
                const float gain=g+(p.gain-g)*fraction;
                l[pos]*=gain;r[pos]*=gain;
            }
            prior=p.offset;g=p.gain;
        }
        for(;pos<frames_;++pos){l[pos]*=g;r[pos]*=g;}
    }
};
}
