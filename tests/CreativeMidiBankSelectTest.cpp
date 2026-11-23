#include "CreativeMidiBankSelect.hpp"
#include "CreativeMidiControllers.hpp"
#include <limits>
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
    for(unsigned ch=0;ch<16;++ch){
        for(unsigned i=0;i<midiControllerSpecs.size();++i){
            const auto number=midiControllerSpecs[i].number;
            const auto id=midiControllerBase+i*midiControllerStride+ch;
            CHECK(midiControllerIndex(number)==static_cast<int>(i));
            for(double v:{0.,0.5,1.}){
                MidiEvent e{};
                CHECK(makeMappedController(id,v,42,e));
                CHECK(e.absoluteHostSample==42);
                if(number==129){
                    const unsigned expected=v==0.?0:v==0.5?8192:16383;
                    CHECK(e.status==(0xe0u|ch)&&e.size==3);
                    CHECK((unsigned(e.data1)|(unsigned(e.data2)<<7))==expected);
                }else if(number==128){
                    CHECK(e.status==(0xd0u|ch)&&e.size==2&&e.data2==0);
                    CHECK(e.data1==std::lround(v*127.));
                }else{
                    CHECK(e.status==(0xb0u|ch)&&e.size==3&&e.data1==number);
                    CHECK(e.data2==std::lround(v*127.));
                }
            }
        }
    }
    MidiEvent invalid{};
    CHECK(!makeMappedController(midiControllerBase-1,0.5,0,invalid));
    CHECK(!makeMappedController(midiControllerBase+16*midiControllerSpecs.size(),0.5,0,invalid));
    CHECK(!makeMappedController(midiControllerBase,std::numeric_limits<double>::quiet_NaN(),0,invalid));
    CHECK(midiControllerIndex(99)==-1);
    puts("Creative mapped controller MIDI byte encoding: all 16 channels PASS");
    puts("Creative DOS CC0 Bank Select and Program Change: 4096 Release checks PASS");
}
