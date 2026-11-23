# DOSBox-X-AWE32: original core in-process (development)

The pinned upstream DOSBox-X-AWE32 source remains unchanged in Git. The Python script applies reversible source changes only to a CI working copy:

- Export the emulator's original main() as AWE64_StartGuest() for invocation inside the DAW process, on a separate worker thread.
- Install actual emulated DOS hardware I/O handlers at BEE0h..BEEFh and BEF0h. The original Creative DOS APIs still perform the EMU8000 register writes.
- Tap the original DOSBox-X stereo mixer PCM output after its unchanged EMU8000 synthesis. Host callback only queues float samples.
- Compile the original Release x64 DOSBox-X source as an internal Windows DLL; never spawn a standalone EXE.

The host ABI bridge in DosBoxHostAdapter.hpp connects per-instance SPSC queues and the Creative DOS mailbox to these C exports without inserting any sound engine.

Current limitations: the DLL Windows CI build may still fail; VST3 loading, DOSBox guest boot, original Creative binary, private ROM/SBK/SF2, playback, headless SDL, clean shutdown, resampling, multi-instance isolation and DAW recall remain unproven. Original DOSBox-X uses global emulator state, so more than one active runtime is unsafe until resolved. No releases or downloaded runtime artifacts are permitted from this test.