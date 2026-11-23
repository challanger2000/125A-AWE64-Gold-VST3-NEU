/* Copyright 2026 125A. Minimal adapter to ORIGINAL Creative DOS SF2LIB.
 * Include inside the original private Creative DOS/DEMO/DEMO.C build.
 * This header invokes actual Creative awe32* API functions; it does NOT
 * replace, emulate or reconstruct Creative's driver or synthesis.
 *
 * Host must install guest I/O handlers at BEE0h..BEF0h and supply events.
 * A guest without those handlers sees FFh and never sends fake MIDI.
 *
 * Note: This does NOT handle timing, arbitrary SysEx, or SF2 loading;
 * those remain the original Creative demo's responsibility.
 */
#ifndef AWE64_VST3_ORIGINAL_CREATIVE_BRIDGE_H
#define AWE64_VST3_ORIGINAL_CREATIVE_BRIDGE_H
#ifndef AWE64_GUEST_BASE
#define AWE64_GUEST_BASE 0xBEE0
#endif
#ifndef AWE64_GUEST_ACK
#define AWE64_GUEST_ACK 0xBEF0
#endif
/* Expects original ctaweapi.h to be included by DEMO.C already. */
static void AWE64_GuestPoll(void)
{
    unsigned int available, msg, d1, d2, len, ch;
    WORD result;
    available=(unsigned int)_inp(AWE64_GUEST_BASE);
    if ((available & 3U)!=3U) return;
    msg=(unsigned int)_inp(AWE64_GUEST_BASE+1);
    d1=(unsigned int)_inp(AWE64_GUEST_BASE+2);
    d2=(unsigned int)_inp(AWE64_GUEST_BASE+3);
    len=(unsigned int)_inp(AWE64_GUEST_BASE+4);
    ch=msg&15U;
    result=0;
    /* Protect against malformed packets. Both 2- and 3-byte channel
       messages are dispatched to the ORIGINAL Creative functions. */
    if(msg<0x80U || msg>=0xF0U || d1>127U || d2>127U) result=1;
    else if (((msg&0xF0U)==0xC0U || (msg&0xF0U)==0xD0U)
             ? len!=2U : len!=3U) result=1;
    else switch(msg & 0xF0U) {
    case 0x80:result=awe32NoteOff((WORD)ch,(WORD)d1,(WORD)d2);break;
    case 0x90:
        result=d2 ? awe32NoteOn((WORD)ch,(WORD)d1,(WORD)d2)
                  : awe32NoteOff((WORD)ch,(WORD)d1,0);break;
    case 0xA0:result=awe32PolyKeyPressure((WORD)ch,(WORD)d1,(WORD)d2);break;
    case 0xB0:result=awe32Controller((WORD)ch,(WORD)d1,(WORD)d2);break;
    case 0xC0:result=awe32ProgramChange((WORD)ch,(WORD)d1);break;
    case 0xD0:result=awe32ChannelPressure((WORD)ch,(WORD)d1);break;
    case 0xE0:result=awe32PitchBend((WORD)ch,(WORD)d1,(WORD)d2);break;
    default:result=1;break;
    }
    _outp(AWE64_GUEST_ACK,result ? 0x5A : 0xA5);
}
#endif
