"""Public 16-bit DOS MIDI->hardware-audio MZ test fixture; NO Creative bytes.

The original DOSBox-X MPU-401 at 330h/331h receives a MIDI NoteOn
(channel 0, middle C, velocity 100) THROUGH THE ACTUAL VST3 PROCESSOR.
Only after all three bytes match does this original DOSBox-X guest activate
the real emulated PIT/PC-speaker I/O (43h, 42h, 61h) for ~2 seconds.

The Python fixture never generates audio and does not implement EMU8000,
SF2, or the Creative DOS driver. Original AWE sound is NOT proven by it.
"""
import struct

from MakeMpuCom import Asm


def build_midi_triggered_exe():
    asm = Asm()

    def call(label):
        # 16-bit near CALL; use the same relative relocation rules as Asm.jmp.
        asm.put(0xE8)
        asm.reloc.append((len(asm.b), label, "rel16"))
        asm.put(0, 0)

    asm.put(0x0E, 0x1F)              # push cs; pop ds
    asm.put(0xBA, 0x31, 0x03)        # mov dx,331h
    asm.put(0xB0, 0x3F, 0xEE)        # original MPU401 switch to UART
    call("getbyte")
    asm.jr(0x72, "exit")            # no ACK -> no PC speaker
    asm.put(0x3C, 0xFE)              # expect original MPU_ACK_OK
    asm.jr(0x75, "exit")
    asm.mark("search")
    call("getbyte")
    asm.jr(0x72, "exit")
    asm.put(0x3C, 0x90)              # expect NoteOn on channel 0
    asm.jr(0x75, "search")          # ignore bank/program and other messages
    call("getbyte")
    asm.jr(0x72, "exit")
    asm.put(0x3C, 0x3C)              # expect middle C, MIDI note 60
    asm.jr(0x75, "search")
    call("getbyte")
    asm.jr(0x72, "exit")
    asm.put(0x3C, 0x64)              # expect velocity 100
    asm.jr(0x75, "search")

    asm.mark("play")
    asm.put(0xB0, 0xB6, 0xE6, 0x43)  # program original PIT channel 2
    asm.put(0xBA, 0x42, 0x00, 0xB8, 0x98, 0x0A)
    asm.put(0xEE, 0x88, 0xE0, 0xEE)  # divisor 0A98h -> ~440 Hz
    asm.put(0xE4, 0x61, 0x0C, 0x03, 0xE6, 0x61)  # speaker ON
    asm.put(0xB4, 0x00, 0xCD, 0x1A, 0x89, 0xD3)  # BIOS timer start
    asm.mark("hold")
    asm.put(0x53, 0xB4, 0x00, 0xCD, 0x1A, 0x5B)  # timer, save BX
    asm.put(0x29, 0xDA, 0x83, 0xFA, 0x24)  # ~36/18.2 = 2 seconds
    asm.jr(0x72, "hold")
    asm.put(0xE4, 0x61, 0x24, 0xFC, 0xE6, 0x61)  # speaker OFF

    asm.mark("exit")
    asm.put(0xB8, 0x00, 0x4C, 0xCD, 0x21)  # DOS exit

    # Reading the original emulated MPU401 input byte with a ~10s watchdog.
    # This is not a made-up MIDI queue: 331h is real UART status and 330h data.
    asm.mark("getbyte")
    asm.put(0x53, 0xB4, 0x00, 0xCD, 0x1A, 0x89, 0xD3)  # save BX, start ticks
    asm.mark("outer")
    asm.put(0xBA, 0x31, 0x03, 0xB9, 0xFF, 0xFF)  # status/65535 polls
    asm.mark("poll")
    asm.put(0xEC, 0xA8, 0x80)  # IN AL,331h; TEST 80h
    asm.jr(0x74, "ready")     # data ready, clear status bit
    asm.jr(0xE2, "poll")      # LOOP with bounded counter
    asm.put(0x53, 0xB4, 0x00, 0xCD, 0x1A, 0x5B)  # elapsed BIOS ticks
    asm.put(0x29, 0xDA, 0x81, 0xFA, 0xB6, 0x00)  # 182 ticks ~10s
    asm.jr(0x72, "outer")
    asm.put(0xF9, 0x5B, 0xC3)  # STC, POP BX, RET on timeout
    asm.mark("ready")
    asm.put(0xBA, 0x30, 0x03, 0xEC, 0xF8, 0x5B, 0xC3)  # IN 330h; CLC POP BX RET

    # Asm uses COM origin 0x100 for absolute data, but this EXE uses only
    # PC-relative branches/calls; all near displacements remain identical
    # when loaded by DOS as a zero-based MZ .EXE.
    code = asm.build()
    if len(code) > 512:
        raise ValueError("Unexpectedly long private-data-free MIDI DOS fixture")
    header = bytearray(64)
    length = 64 + len(code)
    fields = [0x5A4D, length % 512, (length + 511) // 512,
              0, 4, 0x1000, 0xFFFF, 0, 0xFFFE,
              0, 0, 0, 0x1C, 0]
    struct.pack_into("<14H", header, 0, *fields)
    return bytes(header + code)
