#!/usr/bin/env python3
"""Generate only synthetic audit fixtures into a fresh build directory."""
import math
import struct
import sys
from pathlib import Path

def chunk(tag,data):
    return tag+struct.pack('<I',len(data))+data+b'\0'*(len(data)%2)
def list_chunk(kind,*parts):return chunk(b'LIST',kind+b''.join(parts))
def name(s):return s.encode('ascii').ljust(20,b'\0')
def phdr(label,program,bag):return struct.pack('<20sHHHIII',name(label),program,0,bag,0,0,0)
def gen(op,val):return struct.pack('<HH',op,val&65535)
def shdr(label,start,end,ls,le,rate,pitch,typ):return struct.pack('<20sIIIIIBbHH',name(label),start,end,ls,le,rate,pitch,0,0,typ)
out=Path(sys.argv[1])
out.mkdir(parents=True,exist_ok=True)
info=list_chunk(b'INFO',chunk(b'ifil',struct.pack('<HH',2,0)),chunk(b'isng',b'EMU8000\0'),chunk(b'INAM',b'Audit\0'))
samples=[round(10000*math.sin(2*math.pi*n/100)) for n in range(4000)]+[0]*46
sdta=list_chunk(b'sdta',chunk(b'smpl',struct.pack('<'+'h'*len(samples),*samples)))
ph=chunk(b'phdr',phdr('AuditTone',0,0)+phdr('EOP',0,1))
pdta=list_chunk(b'pdta',ph,chunk(b'pbag',struct.pack('<4H',0,0,1,0)),chunk(b'pmod',bytes(10)),chunk(b'pgen',gen(41,0)+gen(0,0)),chunk(b'inst',struct.pack('<20sH20sH',name('AuditTone'),0,name('EOI'),1)),chunk(b'ibag',struct.pack('<4H',0,0,2,0)),chunk(b'imod',bytes(10)),chunk(b'igen',gen(54,1)+gen(53,0)+gen(0,0)),chunk(b'shdr',shdr('AuditTone',0,4000,100,3900,44100,60,1)+shdr('EOS',4046,4046,4046,4046,0,0,0)))
valid=chunk(b'RIFF',b'sfbk'+info+sdta+pdta)
def write_bytes(n,data):
    path=out/(n+'.sf2')
    if path.exists():
        if path.read_bytes()!=data:raise ValueError('Refuse to overwrite differing file: '+str(path))
        return
    with path.open('xb') as f:f.write(data)
write_bytes('valid',valid)
write_bytes('missing_sample_and_hydra',chunk(b'RIFF',b'sfbk'+info+list_chunk(b'pdta',ph)))
write_bytes('invalid_signature',b'XXXX'+valid[4:])
def split(b):
    p=0;result=[]
    while p<len(b):
        n=struct.unpack_from('<I',b,p+4)[0];result.append((b[p:p+4],b[p+8:p+8+n]));p+=8+n+(n%2)
    if p!=len(b):raise ValueError('fixture RIFF bounds')
    return result
parts=[(b[:4],split(b[4:])) for a,b in split(valid[12:]) if a==b'LIST']
def build(change):
    lists={k:list(v) for k,v in parts};change(lists)
    return chunk(b'RIFF',b'sfbk'+b''.join(list_chunk(k,*(chunk(t,d) for t,d in v)) for k,v in lists.items()))
def mutate(l,tag,offset,fmt,value):
    for i,(t,d) in enumerate(l):
        if t==tag:
            data=bytearray(d);struct.pack_into(fmt,data,offset,value);l[i]=(t,bytes(data));return
    raise ValueError(tag)
def without(l,tag):l[:]=[(t,d) for t,d in l if t!=tag]
def write(n,fn):write_bytes(n,build(fn))
for tag,_ in dict(parts)[b'pdta']:
    write('reject_missing_'+tag.decode(),lambda l,t=tag:without(l[b'pdta'],t))
write('reject_missing_sdta',lambda l:l.pop(b'sdta'))
write('reject_duplicate_phdr',lambda l:l[b'pdta'].append(l[b'pdta'][0]))
write('reject_duplicate_smpl',lambda l:l[b'sdta'].append(l[b'sdta'][0]))
write('reject_phdr_bag_oob',lambda l:mutate(l[b'pdta'],b'phdr',24,'<H',65535))
write('reject_pbag_gen_oob',lambda l:mutate(l[b'pdta'],b'pbag',4,'<H',65535))
write('reject_pbag_mod_oob',lambda l:mutate(l[b'pdta'],b'pbag',6,'<H',65535))
write('reject_terminal_instrument_reference',lambda l:mutate(l[b'pdta'],b'pgen',2,'<H',1))
write('reject_terminal_sample_reference',lambda l:mutate(l[b'pdta'],b'igen',6,'<H',1))
write('reject_sample_end_oob',lambda l:mutate(l[b'pdta'],b'shdr',24,'<I',0xffffffff))
write('reject_sample_start_after_end',lambda l:mutate(l[b'pdta'],b'shdr',20,'<I',4001))
write('reject_sample_zero_rate',lambda l:mutate(l[b'pdta'],b'shdr',36,'<I',0))
write('reject_missing_sample_link',lambda l:mutate(l[b'pdta'],b'igen',4,'<H',0))
write('reject_wrong_terminal',lambda l:mutate(l[b'pdta'],b'shdr',46,'<I',0))
write('accept_sf201',lambda l:mutate(l[b'INFO'],b'ifil',2,'<H',1))
def rom(l):
    mutate(l[b'pdta'],b'shdr',44,'<H',0x8001);l.pop(b'sdta')
write('accept_rom_only',rom)
