#pragma once
// Single-producer/single-consumer bridge for an in-process DOSBox worker.
// No synthesizer, original driver or VST3 runtime is implemented by this file.
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace awe64::rt {

template <typename T, std::size_t Capacity>
class SpscQueue final {
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two >= 2");
    static_assert(std::is_trivially_copyable_v<T>, "Payload must be trivial");
    std::array<T, Capacity> data_{};
    alignas(64) std::atomic<std::uint64_t> read_{0};
    alignas(64) std::atomic<std::uint64_t> written_{0};
public:
    SpscQueue() noexcept = default;
    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;
    static constexpr std::size_t capacity() noexcept { return Capacity; }

    [[nodiscard]] bool tryPush(const T& item) noexcept {
        const auto w = written_.load(std::memory_order_relaxed);
        const auto r = read_.load(std::memory_order_acquire);
        if (w - r >= Capacity) return false;
        data_[static_cast<std::size_t>(w) & (Capacity - 1)] = item;
        written_.store(w + 1, std::memory_order_release);
        return true;
    }
    // Consumer only: inspect complete packet without consuming it when
    // the UART cannot currently accept its entire length.
    [[nodiscard]] bool tryPeek(T& item) const noexcept {
        const auto r=read_.load(std::memory_order_relaxed);
        const auto w=written_.load(std::memory_order_acquire);
        if(w==r)return false;
        item=data_[static_cast<std::size_t>(r)&(Capacity-1)];
        return true;
    }
    [[nodiscard]] bool tryPop(T& item) noexcept {
        const auto r = read_.load(std::memory_order_relaxed);
        const auto w = written_.load(std::memory_order_acquire);
        if (w == r) return false;
        item = data_[static_cast<std::size_t>(r) & (Capacity - 1)];
        read_.store(r + 1, std::memory_order_release);
        return true;
    }
    // Consumer only: retire the currently published prefix in O(1).
    // A concurrently publishing producer may still add a later entry.
    void discardPublished() noexcept {
        read_.store(written_.load(std::memory_order_acquire),std::memory_order_release);
    }
    [[nodiscard]] std::size_t sizeApprox() const noexcept {
        const auto r = read_.load(std::memory_order_acquire);
        const auto w = written_.load(std::memory_order_acquire);
        return static_cast<std::size_t>(w - r);
    }
};

struct StereoFrame final { float left; float right; };
// Legacy asynchronous guest capture: even epochs admit PCM, odd epochs
// suspend capture. Distinguish old in-flight callbacks on resume.
class LegacyCaptureGate final {
    std::atomic<std::uint32_t> epoch_{0};
public:
    std::uint32_t epoch() const noexcept {return epoch_.load(std::memory_order_acquire);}
    void suspend() noexcept {epoch_.fetch_or(1u,std::memory_order_acq_rel);}
    void resume() noexcept {
        auto e=epoch();
        if(e&1u)(void)epoch_.compare_exchange_strong(e,e+1u,std::memory_order_acq_rel);
    }
};
enum class GuestCommand : std::uint8_t { Midi, VoiceControls, AbsoluteSends };
struct MidiEvent final {
    std::uint64_t absoluteHostSample;
    std::uint8_t status;
    std::uint8_t data1;
    std::uint8_t data2;
    std::uint8_t size;
    std::array<std::uint8_t,11> sysex{};
    std::uint64_t guestFrame=0; // valid only in explicitly clocked mode
    GuestCommand command=GuestCommand::Midi;
    std::uint64_t controlLo=0;
    unsigned controlHi=0;
};

// Audio producer: DOSBox worker; consumer: VST3 audio thread.
// MIDI producer: VST3 audio thread; consumer: DOSBox worker.
// All queues are per instance and never share mutable state.
// Host-rate resampling and guest scheduling are NOT yet implemented.
class RealtimeQueues final {
    struct TaggedFrame {StereoFrame sample;std::uint32_t epoch;};
    SpscQueue<TaggedFrame, 32768> audio_{};
    SpscQueue<MidiEvent, 4096> midi_{};
    std::atomic<std::uint64_t> audioDrops_{0};
    std::atomic<std::uint64_t> midiDrops_{0};
    std::atomic<bool> midiRecovery_{false};
    std::atomic<std::uint64_t> midiRecoveryEpoch_{0};
    std::atomic<std::uint64_t> audioUnderruns_{0};
public:
    bool pushAudio(const StereoFrame& frame,std::uint32_t epoch=0) noexcept {
        if (audio_.tryPush({frame,epoch})) return true;
        audioDrops_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    bool pushMidi(const MidiEvent& event) noexcept {
        // Never admit new events behind a lost release while the consumer
        // still owes its fail-safe release sequence. The single producer
        // requests recovery; the single consumer acknowledges it only
        // after all already accepted MIDI and recovery packets are emitted.
        if (midiRecovery_.load(std::memory_order_acquire)) {
            midiDrops_.fetch_add(1,std::memory_order_relaxed);
            return false;
        }
        if (midi_.tryPush(event)) return true;
        midiDrops_.fetch_add(1, std::memory_order_relaxed);
        midiRecovery_.store(true,std::memory_order_release);
        return false;
    }
    // Producer-only rejection while a recovery is already in progress.
    // Do not tryPush after observing pending: the consumer may ACK between
    // that observation and push, bypassing the caller's pending CC replay.
    void countRejectedMidi() noexcept {midiDrops_.fetch_add(1,std::memory_order_relaxed);}
    // Producer only: a rejected host batch must also initiate recovery.
    void requestMidiRecovery() noexcept {
        midiRecovery_.store(true,std::memory_order_release);
    }
    bool midiRecoveryPending() const noexcept {
        return midiRecovery_.load(std::memory_order_acquire);
    }
    // Consumer only: call after its final recovery packet is delivered.
    void completeMidiRecovery() noexcept {
        midiRecoveryEpoch_.fetch_add(1,std::memory_order_relaxed);
        midiRecovery_.store(false,std::memory_order_release);
    }
    std::uint64_t midiRecoveryEpoch() const noexcept {
        return midiRecoveryEpoch_.load(std::memory_order_acquire);
    }
    bool peekMidi(MidiEvent& event) const noexcept {return midi_.tryPeek(event);}
    bool popMidi(MidiEvent& event) noexcept { return midi_.tryPop(event); }
    void discardAudio() noexcept {audio_.discardPublished();}
    std::size_t readAudio(float* left,float* right,std::size_t frames,std::uint32_t epoch=0) noexcept {
        if (!left || !right) return 0;
        std::size_t available = 0;
        TaggedFrame frame{};
        for (std::size_t i = 0; i < frames; ++i) {
            if (audio_.tryPop(frame)&&frame.epoch==epoch) {
                left[i] = frame.sample.left;
                right[i] = frame.sample.right;
                ++available;
            } else {
                left[i] = 0.f;
                right[i] = 0.f;
            }
        }
        if (available != frames)
            audioUnderruns_.fetch_add(frames - available, std::memory_order_relaxed);
        return available;
    }
    // Consumer-side diagnostic; no writes to the queue or additional
    // work on the sample producer path.
    struct Snapshot {
        std::uint64_t audioDrops, midiDrops, underruns;
        std::size_t audioFrames, midiEvents;
    };
    Snapshot snapshot() const noexcept {
        return {audioDrops(),midiDrops(),audioUnderruns(),
                audio_.sizeApprox(),midi_.sizeApprox()};
    }
    std::uint64_t audioDrops() const noexcept {
        return audioDrops_.load(std::memory_order_relaxed);
    }
    std::uint64_t midiDrops() const noexcept {
        return midiDrops_.load(std::memory_order_relaxed);
    }
    std::uint64_t audioUnderruns() const noexcept {
        return audioUnderruns_.load(std::memory_order_relaxed);
    }
};
} // namespace awe64::rt
