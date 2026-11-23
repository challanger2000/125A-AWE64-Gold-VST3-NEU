#include "HostRateConverter.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#define CHECK(x) do { if(!(x)) {std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();} }while(false)
int main(){
    constexpr double pi=3.14159265358979323846;
    using namespace awe64::rt;
    HostRateConverter c;
    CHECK(!c.configure(32000));CHECK(!c.configure(200000));CHECK(c.configure(44100));
    unsigned n=0;float l[4]{},r[4]{};
    c.render(l,r,4,[&]() noexcept {return StereoFrame{float(++n),-float(n)};});
    CHECK(n==4&&l[0]==1&&l[3]==4&&r[3]==-4&&c.latencySamples()==0);
    for(unsigned rate:{48000u,88200u,96000u,192000u}){
        CHECK(c.configure(rate));CHECK(!c.bypass()&&c.latencySamples()>0);
        std::vector<float> a(rate),b(rate),split(rate),splitR(rate);
        n=0;
        auto pull=[&]() noexcept {return StereoFrame{float(std::sin(2*pi*3000*n++/44100.)),0.f};};
        c.render(a.data(),b.data(),a.size(),pull);
        CHECK(n==44101);
        double e=0.,ref=0.,maxError=0.;
        for(unsigned i=200;i<rate;++i){
            const double expected=std::sin(2*pi*3000*(double(i)/rate-16./44100.));
            const double error=a[i]-expected;
            e+=error*error;ref+=expected*expected;maxError=std::max(maxError,std::abs(error));
            CHECK(b[i]==0.f);
        }
        CHECK(std::sqrt(e/ref)<0.002&&maxError<0.005);
        c.reset();n=0;
        for(unsigned i=0;i<rate;){
            const auto count=std::min(137u,rate-i);
            c.render(split.data()+i,splitR.data()+i,count,pull);i+=count;
        }
        CHECK(n==44101&&a==split&&b==splitR);
    }
    std::puts("44.1 kHz transparent bypass and higher-rate streaming sinc conversion PASS");
}
