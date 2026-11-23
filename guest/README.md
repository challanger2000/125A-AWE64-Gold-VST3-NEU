# Original Creative DOS guest – source-only bridge

The **goal** is a 16-bit DOS executable running **inside the pinned DOSBox-X-AWE32 emulator and the VST3 process**. This is an integration worksite, **not a compiled or tested guest program**.

The upstream Creative ADIP DOS SDK is documented at [qb40/sound-blaster-awe32](https://github.com/qb40/sound-blaster-awe32). The original `DOS/DEMO/DEMO.C` calls Creative's real `awe32Detect`, `awe32InitHardware`, `awe32SFontLoadRequest`, `awe32StreamSample`, `awe32SetPresets`, `awe32InitMIDI`, `awe32NoteOn/Off`, etc, linking to **original** Creative DOS SF2LIB objects.

**Strategy:** Keep the original Creative code and bank loading. Apply `tools/patch_creative_demo.py` **to a private copy**, introducing only `AWE64_BRIDGE.H` and its poll function. `AWE64_BRIDGE.H` calls the unchanged Creative MIDI functions. Build DOS guest privately with the proper original compiler/Creative libraries. No Creative binaries or source are committed here.

The guest polls the proposed IO mailbox at `BEE0h..BEF0h`. The PC-host side `src/CreativeDosMailbox.hpp` is per instance and backed by the actual lock-free host MIDI queue. Only a future DOSBox I/O adapter may bind those emulated ports; the **port registration, DOSBox in-process boot, original Creative guest build, audio output and all end-to-end tests are currently missing**.

This is deliberately *not* the native P009 player and does not synthesize or emulate original Creative's driver in Windows.
