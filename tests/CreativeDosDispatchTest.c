/* Verifies AWE64_BRIDGE.H against exact Creative DOS MIDI function
 * signatures, but with mock implementations only. This is NOT a guest
 * DOS execution test and never claims original Creative PCM.
 */
#include <stdio.h>
#include <stdlib.h>
typedef unsigned short WORD;

static unsigned char ports[17];
static int lastPort=-1, lastAck=-1, call=-1, argcLast=0;
static WORD args[3];

static int mock_guest_inp(unsigned short p) {
    return p>=0xBEE0u && p<0xBEF1u ? ports[p-0xBEE0u] : 255;
}
static int mock_guest_outp(unsigned short p,int value) {
    lastPort=(int)p;lastAck=value;return value;
}
#define MOCK3(name,id) \
static WORD name(WORD a,WORD b,WORD c) { \
    call=id;argcLast=3;args[0]=a;args[1]=b;args[2]=c;return 0; \
}
#define MOCK2(name,id) \
static WORD name(WORD a,WORD b) { \
    call=id;argcLast=2;args[0]=a;args[1]=b;return 0; \
}
MOCK3(awe32NoteOff,1)
MOCK3(awe32NoteOn,2)
MOCK3(awe32PolyKeyPressure,3)
MOCK3(awe32Controller,4)
MOCK2(awe32ProgramChange,5)
MOCK2(awe32ChannelPressure,6)
MOCK3(awe32PitchBend,7)
#define _inp mock_guest_inp
#define _outp mock_guest_outp
#include "AWE64_BRIDGE.H"
#undef _inp
#undef _outp
#define CHECK(x) do { if(!(x)) { \
    fprintf(stderr,"Creative guest dispatch test failed at %d: %s\n",__LINE__,#x); \
    exit(1); \
} } while(0)
static void send(unsigned status,unsigned a,unsigned b,unsigned len) {
    ports[0]=3;ports[1]=(unsigned char)status;ports[2]=(unsigned char)a;
    ports[3]=(unsigned char)b;ports[4]=(unsigned char)len;
    call=-1;lastPort=-1;lastAck=-1;
    AWE64_GuestPoll();
    CHECK(lastPort==0xBEF0);
}
int main(void){
    ports[0]=1;
    AWE64_GuestPoll();
    CHECK(lastPort==-1);
    send(0x90,60,99,3);
    CHECK(call==2 && argcLast==3 && args[0]==0 && args[1]==60 &&
          args[2]==99 && lastAck==0xA5);
    send(0x90,60,0,3);
    CHECK(call==1 && args[0]==0 && args[1]==60 && lastAck==0xA5);
    send(0x87,48,67,3);
    CHECK(call==1 && args[0]==7 && args[1]==48 && args[2]==67);
    send(0xAE,43,81,3);
    CHECK(call==3 && args[0]==14 && args[1]==43 && args[2]==81);
    send(0xB3,64,127,3);
    CHECK(call==4 && args[0]==3 && args[1]==64 && args[2]==127);
    send(0xC4,29,0,2);
    CHECK(call==5 && args[0]==4 && args[1]==29);
    send(0xDE,64,0,2);
    CHECK(call==6 && args[0]==14 && args[1]==64);
    send(0xEB,0,64,3);
    CHECK(call==7 && args[0]==11 && args[1]==0 && args[2]==64);
    send(0xFF,1,2,3);
    CHECK(call==-1 && lastAck==0x5A);
    send(0x90,128,30,3);
    CHECK(call==-1 && lastAck==0x5A);
    send(0xC4,29,0,3);
    CHECK(call==-1 && lastAck==0x5A);
    puts("Original Creative DOS MIDI API ABI-adapter mocks PASS");
    return 0;
}
