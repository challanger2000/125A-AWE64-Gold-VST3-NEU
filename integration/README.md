# DOSBox-X-AWE32: original core in-process (development)

The pinned upstream DOSBox-X-AWE32 source remains unchanged in Git. The Python script applies reversible source changes only to a CI working copy:

- Export the emulator's original main() as AWE64_StartGuest() for invocation inside the DAW process, on a separate worker thread.
- Install actual emulated DOS hardware I/O handlers at BEE0h..BEEFh and BEF0h. The original Creative DOS APIs still perform the EMU8000 register writes.
- Tap the original DOSBox-X stereo mixer PCM output after its unchanged EMU8000 synthesis. Host callback only queues float samples.
- Compile the original Release x64 DOSBox-X source as an internal Windows DLL; never spawn a standalone EXE.

The **preferred and simpler original-driver route** uses the original DOSBox-X MPU-401 UART at ports 330h/331h. The unmodified original Creative DOS DEMO.EXE polls these ports and dispatches incoming MIDI to Creative's own SF2LIB functions. No recompiled 16-bit Creative executable or bespoke patched guest program is needed for this route.

The host registers a C callback AWE64_HostRegisterMidi. The emulation thread pumps complete 2/3-byte MIDI messages into the *original* MPU401 queue when the guest polls its UART status. A preexisting MIDI device on the physical Windows host is not required. No Sysex or sample-exact scheduling is yet implemented.

The old BEE0/BEF0 guest mailbox remains in source solely as a non-default experimental fallback; never register both consumers against one SPSC MIDI queue.

DosBoxHostAdapter.hpp now exports pullMidi, host PCM and the non-default experimental port mailbox callbacks. Only pullMidi is intended for the production path.

Current limitations: the DLL Windows CI build may still fail; VST3 loading, DOSBox guest boot, original Creative binary, private ROM/SBK/SF2, playback, headless SDL, clean shutdown, resampling, multi-instance isolation and DAW recall remain unproven. Original DOSBox-X uses global emulator state, so more than one active runtime is unsafe until resolved. No releases or downloaded runtime artifacts are permitted from this test.

The original DOSBox-X event loop now supports AWE64_RequestStop. It sets an atomic flag, and the DOS emulator thread throws the original DOSBox shutdown signal when GFX_Events next polls. The caller must wait for the worker to exit before unloading the DLL. This is **not** yet an end-to-end lifecycle proof, and a blocked guest may not be interruptible.


**Host audio isolation (new):** DOSBox-X SDL2 is linked statically into the internal DLL. On the VST3-only path, its audio driver is changed to SDL's built-in **dummy** backend before `SDL_OpenAudioDevice`; the original DOSBox-X mixer still runs at its original callback cadence and sends original PCM to VST3's SPSC queue, but does not open a separate hardware output or modify global SDL environment variables. `AWE64_IsInternalAudioOnly()` checks a live internal SDL dummy device. The host must fail safely if this device cannot be initialized; actual Creative sound is not yet proven.
