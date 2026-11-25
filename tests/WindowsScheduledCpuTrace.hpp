#pragma once
// Test-host only. Never part of the plugin, guest, audio callback or installer.
// Microsoft SystemTraceProvider + Thread/CSwitch, QPC clock. No cycle-to-time
// conversion. The temporary system-wide ETL is never uploaded or printed.
// https://learn.microsoft.com/windows/win32/etw/configuring-and-starting-a-systemtraceprovider-session
// https://learn.microsoft.com/windows/win32/etw/cswitch
// https://learn.microsoft.com/windows/win32/etw/thread-v2-typegroup1
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <objbase.h>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <cstdio>
#include "ScheduledCpuTime.hpp"
namespace awe64::qa {
class WindowsScheduledCpuTrace final {
    struct Properties {
        EVENT_TRACE_PROPERTIES p{};
        wchar_t name[128]{};
        wchar_t file[MAX_PATH]{};
    } props_{};
    TRACEHANDLE session_=0;
    std::uint64_t freq_=0;
    DWORD status_=ERROR_SUCCESS;
    bool failed_=false;
    std::vector<SchedEvent> events_;
    // Classic kernel Thread provider. Payload PID/TID are authoritative.
    static constexpr GUID threadGuid{0x3d6fa8d1,0xfe05,0x11d0,{0x9d,0xda,0x00,0xc0,0x4f,0xd7,0xba,0x7c}};
    void error(DWORD code) noexcept {failed_=true;status_=code;}
    static void WINAPI record(EVENT_RECORD* event) noexcept {
        auto* self=static_cast<WindowsScheduledCpuTrace*>(event->UserContext);
        if(!self||self->failed_||!IsEqualGUID(event->EventHeader.ProviderId,threadGuid))return;
        const auto opcode=event->EventHeader.EventDescriptor.Opcode;
        if(opcode!=1&&opcode!=2&&opcode!=3&&opcode!=36)return;
        // For Thread start/end/rundown the initial uint32 pair is PID,TID;
        // CSwitch starts with NewThreadId,OldThreadId. Both are pointer-size independent.
        if(event->UserDataLength<8||!event->UserData||event->EventHeader.TimeStamp.QuadPart<=0||
           self->events_.size()>=4000000u){self->error(ERROR_INVALID_DATA);return;}
        std::uint32_t first=0,second=0;
        std::memcpy(&first,event->UserData,4);
        std::memcpy(&second,static_cast<const unsigned char*>(event->UserData)+4,4);
        try {
            self->events_.push_back({static_cast<std::uint64_t>(event->EventHeader.TimeStamp.QuadPart),
                opcode==36?SchedKind::Switch:(opcode==2?SchedKind::End:SchedKind::Start),second,first});
        } catch(...) {self->error(ERROR_NOT_ENOUGH_MEMORY);}
    }
    bool stop() noexcept {
        if(!session_)return false;
        const auto result=ControlTraceW(session_,nullptr,&props_.p,EVENT_TRACE_CONTROL_STOP);
        if(result!=ERROR_SUCCESS){error(result);return false;}
        session_=0;
        if(props_.p.EventsLost||props_.p.LogBuffersLost||props_.p.RealTimeBuffersLost)error(ERROR_INVALID_DATA);
        return !failed_;
    }
public:
    WindowsScheduledCpuTrace()=default;
    WindowsScheduledCpuTrace(const WindowsScheduledCpuTrace&)=delete;
    WindowsScheduledCpuTrace& operator=(const WindowsScheduledCpuTrace&)=delete;
    ~WindowsScheduledCpuTrace() {
        if(session_)stop();
        if(props_.file[0])DeleteFileW(props_.file);
    }
    DWORD status() const noexcept{return status_;}
    std::uint64_t now() const noexcept {
        LARGE_INTEGER q{};return QueryPerformanceCounter(&q)&&q.QuadPart>0?static_cast<std::uint64_t>(q.QuadPart):0;
    }
    bool start() noexcept {
        LARGE_INTEGER f{};
        if(!QueryPerformanceFrequency(&f)||f.QuadPart<=0){error(ERROR_INVALID_DATA);return false;}
        freq_=static_cast<std::uint64_t>(f.QuadPart);
        wchar_t temp[MAX_PATH]{};
        auto n=GetTempPathW(MAX_PATH,temp);
        if(!n||n>=MAX_PATH||!GetTempFileNameW(temp,L"AWE",0,props_.file)){error(GetLastError());return false;}
        if(!DeleteFileW(props_.file)){error(GetLastError());return false;}
        const int named=swprintf_s(props_.name,128,L"125A-Cpu-%lu-%llu",
                                   static_cast<unsigned long>(GetCurrentProcessId()),static_cast<unsigned long long>(now()));
        if(named<=0||CoCreateGuid(&props_.p.Wnode.Guid)!=S_OK){error(ERROR_INVALID_DATA);return false;}
        props_.p.Wnode.BufferSize=sizeof(props_);
        props_.p.Wnode.Flags=WNODE_FLAG_TRACED_GUID;
        props_.p.Wnode.ClientContext=1; // QPC, matched against native window marks
        props_.p.LoggerNameOffset=offsetof(Properties,name);
        props_.p.LogFileNameOffset=offsetof(Properties,file);
        props_.p.LogFileMode=EVENT_TRACE_SYSTEM_LOGGER_MODE|EVENT_TRACE_FILE_MODE_SEQUENTIAL;
        props_.p.BufferSize=64;
        props_.p.MinimumBuffers=16;
        props_.p.MaximumBuffers=128;
        props_.p.MaximumFileSize=256;
        props_.p.EnableFlags=EVENT_TRACE_FLAG_THREAD|EVENT_TRACE_FLAG_CSWITCH|EVENT_TRACE_FLAG_NO_SYSCONFIG;
        TRACEHANDLE ownedSession=0;
        const auto result=StartTraceW(&ownedSession,props_.name,&props_.p);
        if(result!=ERROR_SUCCESS){error(result);return false;}
        session_=ownedSession;
        return true;
    }
    template<std::size_t N>
    bool finish(const std::array<SchedWindow,N>& windows,std::array<double,N>& ratios) {
        if(!stop())return false;
        EVENT_TRACE_LOGFILEW log{};
        log.LogFileName=props_.file;
        log.ProcessTraceMode=PROCESS_TRACE_MODE_EVENT_RECORD|PROCESS_TRACE_MODE_RAW_TIMESTAMP;
        log.EventRecordCallback=&record;
        log.Context=this;
        auto handle=OpenTraceW(&log);
        if(handle==INVALID_PROCESSTRACE_HANDLE){error(GetLastError());return false;}
        if(log.LogfileHeader.PerfFreq.QuadPart<=0||
           static_cast<std::uint64_t>(log.LogfileHeader.PerfFreq.QuadPart)!=freq_){
            CloseTrace(handle);error(ERROR_INVALID_DATA);return false;
        }
        const auto result=ProcessTrace(&handle,1,nullptr,nullptr);
        CloseTrace(handle);
        if(result!=ERROR_SUCCESS){error(result);return false;}
        if(log.EventsLost||log.LogfileHeader.BuffersLost||failed_){error(ERROR_INVALID_DATA);return false;}
        std::array<std::uint64_t,N> totals{};
        if(!scheduledTime(events_,GetCurrentProcessId(),windows,totals)){error(ERROR_INVALID_DATA);return false;}
        for(std::size_t i=0;i<N;++i)ratios[i]=double(totals[i])/double(windows[i].end-windows[i].begin);
        return true;
    }
};
}
