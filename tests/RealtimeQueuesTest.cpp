#include "RealtimeQueues.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <thread>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "CHECK failed %s:%d: %s\\n", __FILE__, __LINE__, #x); std::abort(); } } while(false)

using namespace awe64::rt;
int main() {
    SpscQueue<unsigned, 8> queue;
    unsigned value = 0;
    for (unsigned i = 0; i < 8; ++i) CHECK(queue.tryPush(i));
    CHECK(!queue.tryPush(9));
    for (unsigned i = 0; i < 4; ++i) { CHECK(queue.tryPop(value)); CHECK(value == i); }
    for (unsigned i = 8; i < 12; ++i) CHECK(queue.tryPush(i));
    for (unsigned i = 4; i < 12; ++i) { CHECK(queue.tryPop(value)); CHECK(value == i); }
    CHECK(!queue.tryPop(value));

    RealtimeQueues a;
    RealtimeQueues b;
    float left[4]{}, right[4]{};
    CHECK(a.readAudio(left, right, 4) == 0);
    for (int i = 0; i < 4; ++i) { CHECK(left[i] == 0.f); CHECK(right[i] == 0.f); }
    CHECK(a.audioUnderruns() == 4);
    const auto empty=a.snapshot();
    CHECK(empty.underruns==4 && empty.audioFrames==0 && empty.audioDrops==0 && empty.midiEvents==0);
    CHECK(a.pushAudio({0.125f, -0.5f}));
    CHECK(a.snapshot().audioFrames==1);
    CHECK(a.readAudio(left, right, 4) == 1);
    CHECK(left[0] == 0.125f && right[0] == -0.5f);
    CHECK(left[1] == 0.f && right[3] == 0.f);
    CHECK(b.audioUnderruns() == 0 && b.audioDrops() == 0);

    // A delayed callback from before suspension must never emit old PCM
    // after resume, even if it publishes after the consumer's O(1) discard.
    LegacyCaptureGate gate;
    const auto oldEpoch=gate.epoch();
    CHECK(a.pushAudio({.75f,-.75f},oldEpoch));
    gate.suspend();gate.suspend();CHECK((gate.epoch()&1u)==1u);
    a.discardAudio();
    gate.resume();gate.resume();CHECK(gate.epoch()==oldEpoch+2);
    CHECK(a.pushAudio({.9f,.9f},oldEpoch)); // deliberately late producer
    CHECK(a.pushAudio({.25f,-.25f},gate.epoch()));
    CHECK(a.readAudio(left,right,2,gate.epoch())==1);
    CHECK(left[0]==0.f&&right[0]==0.f&&left[1]==.25f&&right[1]==-.25f);

    const MidiEvent midi{729, 0x90, 60, 127, 3};
    CHECK(a.pushMidi(midi));
    CHECK(a.snapshot().midiEvents==1);
    MidiEvent received{};
    CHECK(a.popMidi(received));
    CHECK(received.absoluteHostSample == 729 && received.status == 0x90 &&
          received.data1 == 60 && received.data2 == 127 && received.size == 3);
    CHECK(!a.popMidi(received));

    SpscQueue<std::uint64_t, 1024> concurrent;
    constexpr std::uint64_t count = 200000;
    std::atomic<bool> ready{false};
    std::thread producer([&] {
        while (!ready.load(std::memory_order_acquire)) std::this_thread::yield();
        for (std::uint64_t i = 0; i < count; ++i)
            while (!concurrent.tryPush(i)) std::this_thread::yield();
    });
    std::thread consumer([&] {
        ready.store(true, std::memory_order_release);
        for (std::uint64_t i = 0; i < count; ++i) {
            std::uint64_t seen{};
            while (!concurrent.tryPop(seen)) std::this_thread::yield();
            CHECK(seen == i);
        }
    });
    producer.join();
    consumer.join();
    CHECK(concurrent.sizeApprox() == 0);
}
