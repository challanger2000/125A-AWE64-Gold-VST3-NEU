#include "CreativeMidiBankSelect.hpp"
#include <cstdio>
#include <cstdlib>
using namespace awe64::rt;
#define CHECK(expr) do { if(!(expr)) { std::fprintf(stderr,"MIDI select check at %d: %s\\n",__LINE__,#expr); std::abort(); } } while(false)
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
    // Every assertion executes even when CMake builds this test in Release.
    for(unsigned channel=0;channel<16u;++channel) {
        for(unsigned bank=0;bank<128u;++bank) {
            const auto select=makeCreativeBankSelect(900,
                static_cast<std::uint8_t>(bank),static_cast<std::uint8_t>(channel));
            CHECK(select.absoluteHostSample==900 && select.status==(0xB0u|channel) &&
                  select.data1==0 && select.data2==bank && select.size==3);
        }
        for(unsigned program=0;program<128u;++program) {
            const auto change=makeCreativeProgramChange(901,
                static_cast<std::uint8_t>(program),static_cast<std::uint8_t>(channel));
            CHECK(change.absoluteHostSample==901 && change.status==(0xC0u|channel) &&
                  change.data1==program && change.data2==0 && change.size==2);
        }
    }
    puts("Creative DOS CC0 Bank Select and Program Change: 4096 Release checks PASS");
}
