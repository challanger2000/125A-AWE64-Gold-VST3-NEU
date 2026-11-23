#pragma once
// Same-process DOSBox-X C ABI -> original Creative DOS mailbox / real mixer PCM.
#include "CreativeDosMailbox.hpp"
#include <cstdint>
#if defined(_WIN32)
#define AWE64_ABI_CALL __cdecl
#else
#define AWE64_ABI_CALL
#endif
namespace awe64::integration {
class DosBoxHostAdapter final {
    rt::RealtimeQueues& queues_;
    guest::CreativeDosMailbox mailbox_;
public:
    explicit DosBoxHostAdapter(rt::RealtimeQueues& q) noexcept:
        queues_(q),mailbox_(q){}
    DosBoxHostAdapter(const DosBoxHostAdapter&)=delete;
    DosBoxHostAdapter& operator=(const DosBoxHostAdapter&)=delete;
    guest::CreativeDosMailbox& mailbox() noexcept{return mailbox_;}
    static unsigned char AWE64_ABI_CALL readPort(void* ctx,unsigned short p) noexcept {
        return ctx?static_cast<DosBoxHostAdapter*>(ctx)->mailbox_.ioRead(p):0xffu;
    }
    static void AWE64_ABI_CALL writePort(void* ctx,unsigned short p,
                                         unsigned char v) noexcept {
        if(ctx)static_cast<DosBoxHostAdapter*>(ctx)->mailbox_.ioWrite(p,v);
    }
    static void AWE64_ABI_CALL receivePCM(void* ctx,const std::int16_t* s,
                                           unsigned int frames) noexcept {
        if(!ctx||!s)return;
        auto& ring=static_cast<DosBoxHostAdapter*>(ctx)->queues_;
        constexpr float scale=1.f/32768.f;
        for(unsigned int i=0;i<frames;i++)
            (void)ring.pushAudio({float(s[2u*i])*scale,float(s[2u*i+1u])*scale});
    }
};
}
