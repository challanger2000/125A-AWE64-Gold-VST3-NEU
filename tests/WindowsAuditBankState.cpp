// Audit-only native host. No guest, ROM, Creative data or audio acceptance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <ole2.h>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "public.sdk/source/common/memorystream.h"
#include <vector>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
namespace S=Steinberg;
namespace V=Steinberg::Vst;
class MinimalHostApplication final : public V::IHostApplication {
    std::atomic<S::uint32> refs_{1};
public:
    S::tresult PLUGIN_API queryInterface(const S::TUID iid, void** obj) override {
        if(!obj)return S::kInvalidArgument;
        *obj=nullptr;
        if(S::FUnknownPrivate::iidEqual(iid,V::IHostApplication::iid)||
           S::FUnknownPrivate::iidEqual(iid,S::FUnknown::iid)) {
            addRef();
            *obj=static_cast<V::IHostApplication*>(this);
            return S::kResultOk;
        }
        return S::kNoInterface;
    }
    S::uint32 PLUGIN_API addRef() override {return ++refs_;}
    S::uint32 PLUGIN_API release() override {return --refs_;}
    S::tresult PLUGIN_API getName(V::String128 name) override {
        if(!name)return S::kInvalidArgument;
        constexpr char text[]="125A CI Synthetic VST3 Host";
        for(unsigned i=0;i<sizeof(text);++i)
            name[i]=static_cast<V::TChar>(text[i]);
        return S::kResultOk;
    }
    S::tresult PLUGIN_API createInstance(S::TUID,S::TUID,void** obj) override {
        if(obj)*obj=nullptr;
        return S::kNoInterface;
    }
};


static int fail(const std::string& m){std::cerr<<"AUDIT_INFRA_FAILURE "<<m<<std::endl;return 2;}
int main(int argc,char** argv){
    if(argc!=5)return fail("usage: host plugin first.sf2 second.sf2 malformed.sf2");
    // Match the SDK's actual Windows hosting module: OLE is initialized
    // on the host UI thread before loading/initializing the plug-in.
    struct OleScope {
        HRESULT status=OleInitialize(nullptr);
        ~OleScope(){if(SUCCEEDED(status))OleUninitialize();}
    } ole;
    if(FAILED(ole.status))return fail("Host OLE initialization failed");
    std::cout<<"HOST_OLE_READY\n";
    std::string filename=argv[1];
    HMODULE module=LoadLibraryExA(filename.c_str(),nullptr,
                                  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|
                                  LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!module)return fail("Windows LoadLibraryEx could not load actual VST3 binary "+
                           std::to_string(GetLastError()));
    using ModuleLifecycle=bool(*)();
    auto initDll=reinterpret_cast<ModuleLifecycle>(GetProcAddress(module,"InitDll"));
    auto exitDll=reinterpret_cast<ModuleLifecycle>(GetProcAddress(module,"ExitDll"));
    std::cout<<"HOST_MODULE_INIT_ENTER\n";
    if(!initDll||!exitDll||!initDll())return fail("Steinberg Windows module initialization failed");
    std::cout<<"HOST_MODULE_INIT_OK\n";
    using GetFactory=S::IPluginFactory* (PLUGIN_API*)();
    auto getFactory=reinterpret_cast<GetFactory>(GetProcAddress(module,"GetPluginFactory"));
    if(!getFactory)return fail("Real Steinberg GetPluginFactory export missing");
    S::IPluginFactory* factory=getFactory();
    if(!factory)return fail("Real VST3 factory returned null");

    S::PClassInfo info{};
    bool found=false;
    for(S::int32 i=0;i<factory->countClasses();++i){
        if(factory->getClassInfo(i,&info)!=S::kResultOk)continue;
        if(std::strcmp(info.category,kVstAudioEffectClass)==0) {
            found=true;break;
        }
    }
    if(!found)return fail("No real VST3 audio component registered");
    V::IComponent* component=nullptr;
    if(factory->createInstance(info.cid,V::IComponent::iid,
                               reinterpret_cast<void**>(&component))!=S::kResultOk||
       !component)return fail("VST3 IComponent creation failed");
    V::IAudioProcessor* processor=nullptr;
    if(component->queryInterface(V::IAudioProcessor::iid,
            reinterpret_cast<void**>(&processor))!=S::kResultOk||
       !processor)return fail("Real component has no VST3 audio processor");
    MinimalHostApplication host;
    if(component->initialize(&host)!=S::kResultOk)
        return fail("VST3 initialize() failed");
    
    V::IEditController* controller=nullptr;
    if(component->queryInterface(V::IEditController::iid,reinterpret_cast<void**>(&controller))!=S::kResultOk||!controller)
        return fail("controller unavailable");
    auto state=[](const std::string& path,unsigned program){
        auto n=static_cast<std::uint32_t>(path.size());
        std::vector<std::uint8_t> b{'D','B','X','D',2,0,static_cast<std::uint8_t>(program),0x10,0x27,0x5a,0xa5,
            std::uint8_t(n),std::uint8_t(n>>8),std::uint8_t(n>>16),std::uint8_t(n>>24)};
        b.insert(b.end(),path.begin(),path.end());return b;
    };
    auto recall=[&](const std::string& path,unsigned p){
        auto b=state(path,p);S::MemoryStream stream(b.data(),b.size());
        return component->setState(&stream);
    };
    auto exact=[&](const std::string& path,unsigned p){
        const auto b=state(path,p);S::MemoryStream stream;
        return component->getState(&stream)==S::kResultOk&&stream.getSize()==static_cast<S::TSize>(b.size())&&
            std::memcmp(stream.getData(),b.data(),b.size())==0&&std::abs(controller->getParamNormalized(1000u)-p/127.)<1e-6;
    };
    const std::string a=argv[2],b=argv[3],bad=argv[4];
    // Positive control: both structurally valid banks, differing path.
    if(recall(a,0)!=S::kResultOk||!exact(a,0)||recall(b,17)!=S::kResultOk||!exact(b,17))
        return fail("valid distinct bank paths did not roundtrip");
    // Negative control: invalid program and absent file must leave state intact.
    if(recall(b,0)==S::kResultOk||!exact(b,17)||recall(b+".missing",17)==S::kResultOk||!exact(b,17))
        return fail("negative control did not reject and preserve");
    std::cout<<"AUDIT_BANK_POSITIVE_NEGATIVE_CONTROLS_PASS"<<std::endl;
    unsigned failures=0;
    const auto invalid=recall(bad,0);
    const bool rejected=invalid!=S::kResultOk&&exact(b,17);
    std::cout<<"AUDIT_A03_LOOP_BOUNDS "<<(rejected?"PASS":"FAIL")<<" result="<<invalid<<std::endl;
    failures+=!rejected;
    if(recall(a,0)!=S::kResultOk||!exact(a,0))return fail("reset to first synthetic bank failed");
    // Both paths are unique generated test fixtures in the runner's temp dir.
    // Overwrite ONLY a with the already positively validated synthetic b.
    std::error_code ec;std::filesystem::copy_file(b,a,std::filesystem::copy_options::overwrite_existing,ec);
    if(ec)return fail("fixture replacement failed: "+ec.message());
    const auto changed=recall(a,17);
    const bool updated=changed==S::kResultOk&&exact(a,17);
    std::cout<<"AUDIT_A06_SAME_PATH_RECALL "<<(updated?"PASS":"FAIL")<<" result="<<changed
        <<" currentProgram="<<controller->getParamNormalized(1000u)*127.<<std::endl;
    failures+=!updated;
    controller->release();processor->release();
    if(component->terminate()!=S::kResultOk)return fail("terminate failed");
    component->release();factory->release();
    if(!exitDll())return fail("module shutdown failed");
    FreeLibrary(module);
    std::cout<<"AUDIT_NATIVE_BANK_STATE_EXECUTED failures="<<failures<<std::endl;
    return failures?1:0;
}
