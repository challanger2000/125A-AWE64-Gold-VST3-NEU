#include "HostRateConverter.hpp"
#include <cstdio>
#include <vector>
#include <random>
using namespace awe64::rt;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
int main(){HostRateConverter c;float l[512]{},r[512]{};
for(unsigned rate:{44100u,48000u,88200u,96000u,192000u}){CHECK(c.configure(rate));std::mt19937 rng(rate);
for(unsigned i=0;i<10000;++i){unsigned n=rng()%513;unsigned actual=0;const auto predicted=c.requiredInputFrames(n);
c.render(l,r,n,[&]() noexcept {++actual;return StereoFrame{.5f,-.5f};});CHECK(predicted==actual);}
std::printf("SRC exact input requirement at %u: 10000 blocks PASS\n",rate);}}
