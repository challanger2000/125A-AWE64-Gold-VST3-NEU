#pragma once
#include <stdint.h>
// Stable C ABI. Resolve the exports through LoadLibrary/GetProcAddress from
// the in-bundle Windows DLL; no C++/SDL/Steinberg objects cross this boundary.
#if defined(_WIN32)
#define AWE64_CALL __cdecl
#else
#define AWE64_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned char (AWE64_CALL *AWE64_ReadPortFn)(void*,unsigned short);
typedef void (AWE64_CALL *AWE64_WritePortFn)(void*,unsigned short,unsigned char);
typedef void (AWE64_CALL *AWE64_PcmFn)(void*,const int16_t*,unsigned int);
typedef void (AWE64_CALL *AWE64_RegisterIOFn)(void*,AWE64_ReadPortFn,AWE64_WritePortFn);
typedef void (AWE64_CALL *AWE64_RegisterPCMFn)(void*,AWE64_PcmFn);
typedef unsigned int (AWE64_CALL *AWE64_PullMidiFn)(void*,unsigned char*,unsigned int);
typedef void (AWE64_CALL *AWE64_RegisterMidiFn)(void*,AWE64_PullMidiFn);
typedef int (AWE64_CALL *AWE64_StartGuestFn)(int,char**);
typedef void (AWE64_CALL *AWE64_RequestStopFn)(void);
typedef int (AWE64_CALL *AWE64_IsInternalAudioOnlyFn)(void);
#ifdef __cplusplus
}
#endif
