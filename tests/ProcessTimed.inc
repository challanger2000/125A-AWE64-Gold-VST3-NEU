tresult DosBoxVst3Shell::processTimed(ProcessData& data,integration::DosBoxRuntimeHost& engine) {
    // The lifecycle mutex is already owned by process(). Neither this
    // function nor HostDemandSession calls DOSBox or waits on the RT path.
    auto silence=[&]() noexcept {
        if(!data.outputs||data.numOutputs<1||!data.outputs[0].channelBuffers32)return;
        auto& out=data.outputs[0];
        for(int32 ch=0;ch<out.numChannels;++ch)
            if(out.channelBuffers32[ch]&&data.numSamples>0)
                std::fill_n(out.channelBuffers32[ch],data.numSamples,0.f);
        out.silenceFlags=out.numChannels>=2?3:1;
    };
    auto fault=[&]() noexcept -> tresult {
        hostDemand_.fail();engine.requestMidiRecovery();
        guestStatus_.store(-2,std::memory_order_release);silence();return kResultFalse;
    };
    if(hostDemand_.faulted()||engine.finished()||data.numSamples<0)return fault();
    if(!timedProcessing_.load(std::memory_order_acquire)) {
        // A parameter-only flush may precede setProcessing(true).
        if(data.numSamples>0){silence();return kResultFalse;}
    }
    if(!engine.ready(static_cast<std::size_t>(data.numSamples))) {
        // Native boot proof is still a gate. Do not silently discard an
        // early MIDI event or claim a parameter change was accepted.
        silence();
        if((data.inputEvents&&data.inputEvents->getEventCount()>0)||
           (data.inputParameterChanges&&data.inputParameterChanges->getParameterCount()>0))return fault();
        return kResultOk;
    }
    guestStatus_.store(2,std::memory_order_release);
    if(data.numSamples>0&&(!data.outputs||data.numOutputs!=1||data.outputs[0].numChannels!=2||
       !data.outputs[0].channelBuffers32||!data.outputs[0].channelBuffers32[0]||!data.outputs[0].channelBuffers32[1]))
        return fault();
    if(!hostDemand_.begin(static_cast<unsigned>(data.numSamples)))return fault();
    // A snapshot is the baseline for this block. Only changed members are
    // published below. Transactional save/recall still requires separate QA.
    rt::HostControlState prior{};
    prior.program=program_.load(std::memory_order_acquire);
    prior.master=master_.load(std::memory_order_acquire);
    prior.reverbType=reverbType_.load(std::memory_order_acquire);
    prior.chorusType=chorusType_.load(std::memory_order_acquire);
    prior.reverbSend=reverbSend_.load(std::memory_order_acquire);
    prior.chorusSend=chorusSend_.load(std::memory_order_acquire);
    for(unsigned i=0;i<10;++i)prior.voice[i]=voiceControls_[i].load(std::memory_order_acquire);
    auto next=prior;
    // Limit work even for a hostile host supplying enormous point counts.
    // The portable planner also enforces aggregate capacity.
    unsigned inputs=0;
    constexpr unsigned maxInputs=4096;
    bool valid=true;
    if(data.inputParameterChanges) {
        const auto count=data.inputParameterChanges->getParameterCount();
        if(count<0||count>static_cast<int32>(maxInputs))valid=false;
        for(int32 i=0;valid&&i<count;++i) {
            auto* q=data.inputParameterChanges->getParameterData(i);
            if(!q){valid=false;break;}
            const auto points=q->getPointCount();
            if(points<0||static_cast<unsigned>(points)>maxInputs-inputs){valid=false;break;}
            inputs+=static_cast<unsigned>(points);
            for(int32 j=0;j<points&&valid;++j) {
                int32 offset=0;ParamValue value=0.;
                valid=q->getPoint(j,offset,value)==kResultOk&&hostDemand_.parameter(offset,q->getParameterId(),value);
            }
        }
    }
    if(valid&&data.inputEvents) {
        const auto count=data.inputEvents->getEventCount();
        if(count<0||static_cast<unsigned>(count)>maxInputs-inputs)valid=false;
        for(int32 i=0;valid&&i<count;++i) {
            Event event{};
            if(data.inputEvents->getEvent(i,event)!=kResultOk){valid=false;break;}
            if(event.type==Event::kNoteOnEvent) {
                const auto& n=event.noteOn;
                valid=event.busIndex==0&&hostDemand_.note(event.sampleOffset,n.channel,n.pitch,n.velocity,true);
            } else if(event.type==Event::kNoteOffEvent) {
                const auto& n=event.noteOff;
                valid=event.busIndex==0&&hostDemand_.note(event.sampleOffset,n.channel,n.pitch,n.velocity,false);
            }
        }
    }
    if(!valid)hostDemand_.reject();
    const auto result=hostDemand_.submit(engine,next,[this](unsigned p) noexcept {
        return startedCatalog_.Find(0,static_cast<std::uint8_t>(p))!=nullptr;
    });
    if(result==rt::HostDemandSession::Result::Fault)return fault();
    // An overflowing batch requests recovery. Authoritative changed controls
    // are reasserted after the recovery epoch; lost sustain is not re-engaged.
    if(next.program!=prior.program)program_.store(next.program,std::memory_order_release);
    if(next.master!=prior.master)master_.store(next.master,std::memory_order_release);
    if(next.reverbType!=prior.reverbType)reverbType_.store(next.reverbType,std::memory_order_release);
    if(next.chorusType!=prior.chorusType)chorusType_.store(next.chorusType,std::memory_order_release);
    if(next.reverbSend!=prior.reverbSend)reverbSend_.store(next.reverbSend,std::memory_order_release);
    if(next.chorusSend!=prior.chorusSend)chorusSend_.store(next.chorusSend,std::memory_order_release);
    for(unsigned i=0;i<10;++i)if(next.voice[i]!=prior.voice[i])voiceControls_[i].store(next.voice[i],std::memory_order_release);
    if(!data.numSamples)return kResultOk;
    const auto frames=static_cast<std::size_t>(data.numSamples);
    const auto need=rateConverter_.requiredInputFrames(frames);
    // In realtime, fail closed rather than advancing an audio clock on
    // artificial samples. In offline mode a bounded non-RT wait is allowed.
    if(!engine.waitForAudio(need,data.processMode==kOffline))return fault();
    auto& out=data.outputs[0];auto* l=out.channelBuffers32[0];auto* r=out.channelBuffers32[1];
    engine.measureQueueBeforeBlock();
    bool full=true;
    if(rateConverter_.bypass())full=engine.readAudio(l,r,frames)==frames;
    else rateConverter_.render(l,r,frames,[&]() noexcept {
        rt::StereoFrame f{};if(engine.readAudio(&f.left,&f.right,1)!=1)full=false;return f;
    });
    engine.measureQueueAfterBlock();
    if(!full)return fault();
    hostDemand_.applyMaster(l,r,prior.master);
    // Truthful silence flags for both channels.
    out.silenceFlags=0;
    if(std::all_of(l,l+frames,[](float x){return x==0.f;}))out.silenceFlags|=1;
    if(std::all_of(r,r+frames,[](float x){return x==0.f;}))out.silenceFlags|=2;
    return kResultOk;
}
