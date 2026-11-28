// Compile actual extracted product dispatcher and MPU pump against fake
// register storage/UART. This does not execute DOS, EMU8000 or VST3 audio.
#include "VoiceOverlay.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using Bitu=unsigned;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
static constexpr Bitu chipBase=0x620;
static uint64_t context=0;
static uint64_t AWE64_OverlayMidiContext(){return context;}
static std::atomic<uint64_t> awe64_voice_controls_lo{0};
static std::atomic<unsigned> awe64_voice_controls_hi{0},awe64_absolute_sends{0x10000u};
struct Voice {
    uint32_t ccca=0,ptrx=0x12344512,csl=0x23456789;
    unsigned ptrx_revb_send=0x45,csl_chor_send=0x23;
    uint16_t ifatn=0,pefe=0;
    std::array<std::array<uint16_t,8>,5> words{};
};
struct Emu {unsigned cur_voice=0,cur_reg=0;Voice voice[32]{};};
static unsigned row(Bitu p){
    switch(p-chipBase){case 0:return 0;case 2:return 1;case 0x400:return 2;case 0x402:return 3;case 0x800:return 4;default:std::abort();}
}
static void emu8k_outw(uint16_t port,uint16_t word,Emu* e){
    if(port==chipBase+0x802){e->cur_reg=(word>>5)&7;e->cur_voice=word&31;return;}
    auto& v=e->voice[e->cur_voice];const auto reg=e->cur_reg;
    v.words[row(port)][reg]=word;
    if(port==chipBase+0x800&&reg==1)v.ifatn=word;
    if(port==chipBase+0x800&&reg==2)v.pefe=word;
    if(port==chipBase&&reg==1){v.ptrx=(v.ptrx&0xffff0000u)|word;v.ptrx_revb_send=word>>8;}
    if(port==chipBase+2&&reg==7){v.csl=(v.csl&0xffffu)|(uint32_t(word)<<16);v.csl_chor_send=word>>8;}
}
struct Dispatcher {
    Emu storage{};Emu* emu=&storage;
    static constexpr Bitu base=chipBase;
    void Trace(Bitu,Bitu){}
#include "voice.inc"
    uint16_t write(unsigned voice,unsigned reg,unsigned port,uint16_t w){
        VoiceWrite(base+0x802,(reg<<5)|voice);VoiceWrite(base+port,w);
        return emu->voice[voice].words[row(base+port)][reg];
    }
};
static void setKnob(int index,unsigned amount){
    awe64_voice_controls_lo.store(index>=0&&index<8?uint64_t(amount)<<(index*8):0);
    awe64_voice_controls_hi.store(index>=8?amount<<((index-8)*8):0);
}
static void verifyVoice(){
    for(int k=-1;k<10;++k)for(unsigned a:{1u,128u,255u})for(unsigned order:{0u,1u}){
        Dispatcher d;setKnob(k,a);context=5; // first NoteOn
        const unsigned ar=a==1?127:a==128?66:6;
        const unsigned dr=a==1?127:a==128?69:12;
        const unsigned sus=a==1?0:a==128?64:127;
        const unsigned fc=a==1?0:a==128?128:255;
        const unsigned q=a==1?0:a==128?8:15;
        CHECK(d.write(0,4,0x400,0x7000)==(k==0?0x8000:0x7000));
        CHECK(d.write(0,6,0x400,0x7010)==(k==4?0x8000:0x7010));
        CHECK(d.write(0,4,0x402,0x4050)==(k==0?0x7f00u|ar:0x4050u));
        CHECK(d.write(0,6,0x402,0x3055)==(k==4?0x7f00u|ar:0x3055u));
        CHECK(d.write(0,5,0x400,0x5a55)==(k==1?0x5a00u|dr:k==2?(sus<<8)|0x55u:0x5a55u));
        CHECK(d.write(0,7,0x400,0x4b50)==(k==5?0x4b00u|dr:k==6?(sus<<8)|0x50u:0x4b50u));
        CHECK(d.write(0,0,0x402,0x302a)==(k==9?(q<<12)|0x2au:0x302au));
        CHECK(d.write(0,0,0x402,0x342a)==0x342a); // DMA flag in incoming CCCA
        CHECK(d.write(0,5,0x400,0x5ad5)==0x5ad5); // engine disabled
        if(order){d.write(0,2,0x800,0x8100);d.write(0,1,0x800,0xc05b);}
        else{d.write(0,1,0x800,0xc05b);d.write(0,2,0x800,0x8100);}
        CHECK(d.emu->voice[0].ifatn==(k==8?(fc<<8)|0x5bu:k>=4&&k<=7?0x405bu:0xc05bu));
        CHECK(d.emu->voice[0].pefe==(k>=4&&k<=7?0x8140:0x8100));
        context=10; // next NoteOff
        CHECK(d.write(0,5,0x400,0x8051)==(k==3?0x8000u|dr:0x8051u));
        CHECK(d.write(0,7,0x400,0x8052)==(k==7?0x8000u|dr:0x8052u));
        CHECK(d.write(0,4,0x402,0x4050)==0x4050);
        context=13;d.emu->voice[0].ccca=0x04000000;
        CHECK(d.write(0,4,0x402,0x4050)==0x4050);
        CHECK(d.write(0,0,0x402,0x302a)==0x302a);
        CHECK(d.write(30,4,0x402,0x4050)==0x4050);
        context=0;d.emu->voice[0].ccca=0;
        CHECK(d.write(0,4,0x402,0x4050)==0x4050);
    }
    Dispatcher d;context=0;setKnob(-1,0);
    d.emu->cur_reg=3;d.emu->cur_voice=12;
    awe64_absolute_sends.store(0x0000);d.ApplyAbsoluteSends();
    CHECK(d.emu->cur_reg==3&&d.emu->cur_voice==12);
    for(unsigned a:{0u,128u,255u})for(unsigned b:{0u,128u,255u}){
        awe64_absolute_sends.store(a|(b<<8));d.ApplyAbsoluteSends();
        for(unsigned v=0;v<30;++v){
            CHECK(d.emu->voice[v].ptrx_revb_send==a&&d.emu->voice[v].csl_chor_send==b);
            CHECK(d.emu->voice[v].ptrx== (0x12340012u|(a<<8)));
            CHECK(d.emu->voice[v].csl== (0x00456789u|(b<<24)));
        }
        CHECK(d.emu->voice[30].ptrx==0x12344512&&d.emu->voice[31].csl==0x23456789);
        CHECK(d.write(0,1,0,0x9912)==(a<<8|0x12u));
        CHECK(d.write(0,7,2,0x9945)==(b<<8|0x45u));
    }
    std::puts("REGISTER_DISPATCH_PASS: 10 isolated controls, 3 positions, 2 write orders, note context, 2 independent sends");
}
static constexpr unsigned M_UART=1,MPU401_QUEUE=32;
static struct {unsigned mode=M_UART,queue_used=0,queue_pos=0;unsigned char bytes[32]{};} mpu;
static std::atomic<bool> awe64_guest_midi_ready{false};
static bool clockSettled=true;
static bool AWE64_BootClockSettled(){return clockSettled;}
static unsigned char awe64_overlay_tags[32]{};
static void QueueByte(unsigned char b){unsigned i=(mpu.queue_pos+mpu.queue_used)%32;mpu.bytes[i]=b;awe64_overlay_tags[i]=0;++mpu.queue_used;}
static unsigned pulls=0,applied=0;
static unsigned pull(void*,unsigned char* buf,unsigned cap){
    ++pulls;CHECK(cap>=3);
    if(pulls==2){applied=1;return 0;} // ordered host voice-control command
    if(pulls==1||pulls==3){buf[0]=0x90;buf[1]=pulls==1?60:61;buf[2]=100;return 3;}
    return 0;
}
static std::atomic<decltype(&pull)> awe64_mpu_pull{&pull};
static std::atomic<void*> awe64_mpu_ctx{nullptr};
#include "pump.inc"
static unsigned pop(unsigned char expected){CHECK(mpu.queue_used);unsigned i=mpu.queue_pos;CHECK(mpu.bytes[i]==expected);mpu.queue_pos=(i+1)%32;--mpu.queue_used;return awe64_overlay_tags[i];}
static void verifyPump(){
    QueueByte(0xfe);AWE64_PumpGuestMidi();CHECK(!pulls);pop(0xfe);
    clockSettled=false;AWE64_PumpGuestMidi();CHECK(!pulls);clockSettled=true;
    AWE64_PumpGuestMidi();CHECK(pulls==1&&applied==0&&mpu.queue_used==3);
    CHECK(pop(0x90)==0);AWE64_PumpGuestMidi();CHECK(pulls==1&&applied==0);
    CHECK(pop(60)==0);AWE64_PumpGuestMidi();CHECK(pulls==1&&applied==0);
    CHECK(pop(100)==1);CHECK(applied==0); // first synchronous NoteOn handler sees old controls
    AWE64_PumpGuestMidi();CHECK(pulls==2&&applied==1&&mpu.queue_used==0);
    AWE64_PumpGuestMidi();CHECK(pulls==3&&applied==1&&mpu.queue_used==3);
    pop(0x90);pop(61);CHECK(pop(100)==1);
    std::puts("UART_CONTROL_ORDER_PASS: no read-ahead before complete original MIDI packet");
}
int main(){verifyVoice();verifyPump();}
