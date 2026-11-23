#include "CreativeMidiBankSelect.hpp"
#include <cassert>
using namespace awe64::rt;
constexpr auto b0=makeCreativeBankSelect(500,0);
constexpr auto b127=makeCreativeBankSelect(501,127,15);
constexpr auto p=makeCreativeProgramChange(502,76,15);
static_assert(b0.absoluteHostSample==500 && b0.status==0xB0 &&
              b0.data1==0 && b0.data2==0 && b0.size==3);
static_assert(b127.absoluteHostSample==501 && b127.status==0xBF &&
              b127.data1==0 && b127.data2==127 && b127.size==3);
static_assert(p.absoluteHostSample==502 && p.status==0xCF &&
              p.data1==76 && p.data2==0 && p.size==2);
int main() {
    for(unsigned bank=0;bank<128u;++bank) {
        const auto select=makeCreativeBankSelect(900,static_cast<std::uint8_t>(bank));
        assert(select.status==0xB0 && select.data1==0 &&
               select.data2==bank && select.size==3);
    }
}
