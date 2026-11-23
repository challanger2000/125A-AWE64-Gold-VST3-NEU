"""Original DOSBox-X MPU-401 UART integration probe. No Creative copyrighted code.

Create a 16-bit .COM file with a tiny DOS program that:
  - uses real 0x331/0x330 UART hardware I/O
  - requests UART mode and consumes the original MPU-401 ACK
  - receives exactly three MIDI bytes from original DOSBox-X hardware
  - saves those bytes to MIDI.BIN using DOS INT 21h file functions.

The guest probes actual emulated I/O, not a synthetic or rebuilt sound engine.
"""
from dataclasses import dataclass,field
from pathlib import Path

@dataclass
class Asm:
    b:bytearray=field(default_factory=bytearray)
    labels:dict=field(default_factory=dict)
    reloc:list=field(default_factory=list)
    def put(self,*values):
        self.b.extend(values)
    def mark(self,name):
        if name in self.labels: raise ValueError("duplicate label: "+name)
        self.labels[name]=len(self.b)+0x100
    def word(self,name):
        self.reloc.append((len(self.b),name,"abs16"))
        self.put(0,0)
    def jr(self,opcode,name):
        self.put(opcode)
        self.reloc.append((len(self.b),name,"rel8"))
        self.put(0)
    def jmp(self,name):
        self.put(0xE9)
        self.reloc.append((len(self.b),name,"rel16"))
        self.put(0,0)
    def build(self):
        for pos,name,typ in self.reloc:
            if name not in self.labels:raise ValueError("undefined label: "+name)
            target=self.labels[name]
            if typ=="abs16":
                if not (0<=target<65536):raise ValueError("absolute out of range")
                self.b[pos:pos+2]=target.to_bytes(2,"little")
            elif typ=="rel8":
                delta=target-(0x100+pos+1)
                if not(-128<=delta<=127):raise ValueError("short branch out of range")
                self.b[pos]=delta&0xff
            else:
                delta=target-(0x100+pos+2)
                if not(-32768<=delta<=32767):raise ValueError("near branch out of range")
                self.b[pos:pos+2]=(delta&65535).to_bytes(2,"little")
        return bytes(self.b)

def build_probe():
    a=Asm()
    a.put(0x0e,0x1f)       # PUSH CS / POP DS
    a.put(0xba,0x31,0x03)  # MOV DX, 0331h
    a.put(0xb0,0x3f,0xee)  # MOV AL, UART MODE / OUT DX,AL
    a.put(0xb9,0xff,0xff)  # MOV CX,ffff
    a.mark("ack_wait")
    a.put(0xec,0xa8,0x80)  # IN AL,DX / TEST AL,80h
    a.jr(0x74,"read_ack")  # JZ ready
    a.jr(0xe2,"ack_wait")  # LOOP wait
    a.jmp("write_file")    # write zeros on failed ACK
    a.mark("read_ack")
    a.put(0xba,0x30,0x03,0xec) # MOV DX,330 / IN AL,DX
    a.put(0x3c,0xfe)       # CMP AL, MPU_ACK_OK
    a.jr(0x75,"write_file")
    a.put(0xbe);a.word("buffer") # MOV SI,buffer
    a.put(0xbd,0x03,0x00)  # MOV BP,3
    a.mark("next_byte")
    a.put(0xba,0x31,0x03)  # MOV DX,331
    a.put(0xb9,0xff,0xff)  # MOV CX,ffff
    a.mark("wait_byte")
    a.put(0xec,0xa8,0x80)  # IN AL,DX / TEST AL,80h
    a.jr(0x74,"read_byte")
    a.jr(0xe2,"wait_byte")
    a.jmp("write_file")
    a.mark("read_byte")
    a.put(0xba,0x30,0x03,0xec) # MOV DX,330 / IN AL,DX
    a.put(0x88,0x04,0x46,0x4d) # MOV [SI],AL / INC SI / DEC BP
    a.jr(0x75,"next_byte")
    a.mark("write_file")
    a.put(0xb4,0x3c,0x31,0xc9) # MOV AH,3Ch; XOR CX,CX
    a.put(0xba);a.word("filename") # DX points to ASCIIZ name
    a.put(0xcd,0x21)            # INT 21h: create
    a.jr(0x72,"exit")           # JC
    a.put(0x89,0xc3)            # MOV BX,AX (file handle)
    a.put(0xb4,0x40,0xb9,0x03,0x00) # AH=write CX=3
    a.put(0xba);a.word("buffer")
    a.put(0xcd,0x21)            # INT 21h: write
    a.put(0xb4,0x3e,0xcd,0x21) # close
    a.mark("exit")
    a.put(0xb8,0x00,0x4c,0xcd,0x21)
    a.mark("filename")
    a.put(*b"MIDI.BIN\0")
    a.mark("buffer")
    a.put(0,0,0)
    data=a.build()
    if len(data)>=512:raise AssertionError("MPU test too big")
    return data

if __name__=="__main__":
    import sys
    if len(sys.argv)!=2:raise SystemExit("Usage: MakeMpuCom.py OUTPUT.COM")
    Path(sys.argv[1]).write_bytes(build_probe())
    print("Independent 16-bit DOS MPU-401 probe built")
