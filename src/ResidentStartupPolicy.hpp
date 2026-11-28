#pragma once
#include <cstring>
namespace awe64::guest {
enum class ResidentStartup { Legacy, Resident, Invalid };
// The private resident guest is a paired installation capability. Legacy
// remains available without those assets and for the clocked/offline path.
// A partial installation must not masquerade as the fast path.
inline ResidentStartup selectResidentStartup(const char* overrideMode,
        bool clocked,bool guestPresent,bool extenderPresent) noexcept {
    if(overrideMode){
        if(std::strcmp(overrideMode,"0")==0)return ResidentStartup::Legacy;
        if(std::strcmp(overrideMode,"1")!=0)return ResidentStartup::Invalid;
        return !clocked&&guestPresent&&extenderPresent?ResidentStartup::Resident:ResidentStartup::Invalid;
    }
    if(clocked)return ResidentStartup::Legacy;
    if(guestPresent!=extenderPresent)return ResidentStartup::Invalid;
    return guestPresent?ResidentStartup::Resident:ResidentStartup::Legacy;
}
}
