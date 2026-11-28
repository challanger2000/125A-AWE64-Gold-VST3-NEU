#!/usr/bin/env python3
"""Private resident Creative guest generator: existing DOSBox IO ABI, no new engine."""
import argparse,hashlib
from pathlib import Path

def patch(data):
    blob=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
    if blob!='f27a54aecd1656a47bf3c50bf5b742bec1f0c43d': raise ValueError('Wrong original source')
    s=data.decode('latin1').replace('\r\n','\n')
    def once(a,b):
        nonlocal s
        if s.count(a)!=1: raise ValueError('Unexpected anchor: '+a)
        s=s.replace(a,b)
    code=r'''
/* Host command is consumed once by the existing per-DLL I/O callback.
 * The host must suppress new MIDI until final acknowledgement, and stage
 * the inactive slot before submitting. Files remain immutable during load.
 * No DOS file polling. No change to original MIDI handler or SF2LIB. */
#define RELOAD_PORT 0xBEE0
#define ACK_PORT 0xBEF0
#define BANK_A 0xA1
#define BANK_B 0xA2
#define BUSY 0x11
#define REJECT 0xE1
#define ROLLED_BACK 0xE2
#define FATAL 0xE3
static unsigned reloadPollCounter;
static int reloadCurrent=BANK_A;
static int residentFault;
static void ReloadAck(int status) { _outp(ACK_PORT,status); }
static const char* ReloadPath(int command) {
    if(command==BANK_A)return "D:\\BANKA.SF2";
    if(command==BANK_B)return "D:\\BANKB.SF2";
    return NULL;
}
static int RestoreClock(void) {
    /* Both restorations must execute even if one fails. */
    int a=system("config -set \"cpu turbo=false\"");
    int b=system("config -set \"cpu cycles=fixed 3000\"");
    return !a&&!b;
}
static int BoostClock(void) {
    if(system("config -set \"cpu cycles=fixed 24000\""))return 0;
    if(system("config -set \"cpu turbo=true\""))return 0;
    return 1;
}
static void ReleaseBank(void) {
    int i;
    awe32ReleaseAllBanks(&spSound);
    for(i=0;i<MAXBANKS;++i)if(pPresets[i])free(pPresets[i]);
    memset(pPresets,0,sizeof(pPresets));
    memset(lBankSizes,0,sizeof(lBankSizes));
    memset(&spSound,0,sizeof(spSound));
}
static int ReloadBank(int command) {
    int i;
    char argument[40];
    char *args[2];
    const char* path=ReloadPath(command);
    FILE *f;
    args[0]="RELOAD";args[1]=argument;
    if(!path)return 0;
    f=fopen(path,"rb");
    if(!f)return 0;
    fclose(f);
    for(i=0;i<16;++i){
        awe32Controller((WORD)i,64,0);
        awe32Controller((WORD)i,120,0);
        awe32Controller((WORD)i,123,0);
    }
    ReleaseBank();
    awe32TotalPatchRam(&spSound);
    strcpy(argument,"/SBK:");strcat(argument,path);
    if(LoadSBK(2,args))return -1;
    awe32InitMIDI();
    wMIDIState=UNDEFINED;wMIDIRecv=0;
    memset(bMIDIBuf,0,sizeof(bMIDIBuf));
    reloadCurrent=command;
    return 1;
}
static void ReloadPoll(void) {
    int command,result,prior,status;
    /* Called only on UART-idle path, between original MIDI handlers. */
    if(++reloadPollCounter!=64)return;
    reloadPollCounter=0;
    command=(unsigned char)_inp(RELOAD_PORT);
    if(command==0||command==255)return;
    if(!ReloadPath(command)){ReloadAck(REJECT);return;}
    if(command==reloadCurrent){ReloadAck(command);return;}
    prior=reloadCurrent;
    ReloadAck(BUSY);
    if(!BoostClock()){
        if(!RestoreClock()){residentFault=1;ReloadAck(FATAL);}
        else ReloadAck(REJECT);
        return;
    }
    result=ReloadBank(command);
    if(result==1)status=command;
    else if(result==0)status=REJECT;
    else status=ReloadBank(prior)==1?ROLLED_BACK:FATAL;
    if(!RestoreClock())status=FATAL;
    if(status==FATAL)residentFault=1;
    ReloadAck(status);
}
static int StartBank(int argc,char** argv) {
    int ok=0;
    /* Initial boot is also accelerated, and must restore normal playback
       before exposing readiness. Initial slot always A. */
    if(BoostClock())ok=!LoadSBK(argc,argv);
    if(!RestoreClock())ok=0;
    return ok;
}
'''
    once('int\nmain(int argc, char** argv)',code+'\nint\nmain(int argc, char** argv)')
    once('    if (LoadSBK(argc, argv)) goto cleanup;  /* load SBK file */','    if (!StartBank(argc, argv)) {ReloadAck(FATAL);goto cleanup;}')
    once('    ret = 0;\n    key = 1;','    ReloadAck(BANK_A);\n    ret = 0;\n    key = 1;')
    once('            ProcessMidi((BYTE) _inp(MPUPort(0)));\n        goto loop;', '            ProcessMidi((BYTE) _inp(MPUPort(0)));\n        else ReloadPoll();\n        if(residentFault){ret=1;goto cleanup;}\n        goto loop;')
    # Fix check of malloc/preset read without changing original sample transfers.
    once('    fread(pPresets[0], 1, (unsigned) spSound.preset_read_size, fp);', '''    if (!pPresets[0] || fread(pPresets[0], 1, (unsigned) spSound.preset_read_size, fp)!=(unsigned)spSound.preset_read_size) {
        fclose(fp);return TRUE;
    }''')
    return s.encode('latin1')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    if a.source.resolve()==a.output.resolve():raise ValueError('Refuse overwriting original')
    a.output.write_bytes(patch(a.source.read_bytes()))
