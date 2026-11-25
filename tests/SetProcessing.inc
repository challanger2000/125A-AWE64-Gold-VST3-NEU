tresult PLUGIN_API DosBoxVst3Shell::setProcessing(TBool state) {
    // VST3 permits this on the processing thread. Never restart a guest,
    // wait, or allocate here. A processing suspension is a virtual pause:
    // the host does not publish a new demand horizon, so the guest reaches
    // the bounded existing horizon and holds there. Preserve the audio,
    // pending MIDI, resampler phase and host timeline for continuation.
    // Full deactivate/reactivate is a separate reset; this is not a seek.
    if(demandActive_.load(std::memory_order_acquire))
        timedProcessing_.store(state!=0,std::memory_order_release);
    return kResultOk;
}
