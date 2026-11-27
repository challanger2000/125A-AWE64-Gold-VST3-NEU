import struct as s, subprocess, tempfile, pathlib, sys
exe=pathlib.Path(sys.argv[1]).resolve()
U16=lambda x,o:s.unpack_from('<H',x,o)[0]
U32=lambda x,o:s.unpack_from('<I',x,o)[0]
def chunk(tag,d):return tag+s.pack('<I',len(d))+d+b'\0'*(len(d)%2)
def lst(name,*chunks):return chunk(b'LIST',name+b''.join(chunks))
def name(n):return n.encode().ljust(20,b'\0')
def gen(o,v):return s.pack('<HH',o,v&65535)
def sh(n,a,b,c,d):return s.pack('<20sIIIIIBbHH',name(n),a,b,c,d,44100,60,0,0,1)
def make(mode=1,start=0,end=1000,ls=100,le=900,offsets=(),global_zone=(),next_sample=False,nonzero_guard=False,preset_extra=(),mod_extra=(),wrong_link=False):
 samples=[100]*1000+[0]*46
 if nonzero_guard:samples[1000]=1
 hdr=sh('sample',start,end,ls,le)
 if next_sample:hdr+=sh('next',1001,1020,1005,1010)
 hdr+=sh('EOS',1046,1046,1046,1046)
 info=lst(b'INFO',chunk(b'ifil',s.pack('<HH',2,1)),chunk(b'isng',b'EMU8000\0'),chunk(b'INAM',b'Audit\0'))
 sd=lst(b'sdta',chunk(b'smpl',s.pack('<1046h',*samples)))
 pg=b''.join(gen(o,v) for o,v in preset_extra)+gen(41,0)+gen(0,0)
 ig0=b''.join(gen(o,v) for o,v in global_zone)
 ig1=b''.join(gen(o,v) for o,v in offsets)+(b'' if mode is None else gen(54,mode))+gen(53,1 if wrong_link else 0)
 ig=ig0+ig1+gen(0,0)
 ib=[0,len(ig0)//4,(len(ig)-4)//4] if global_zone else [0,(len(ig)-4)//4]
 pd=lst(b'pdta',chunk(b'phdr',s.pack('<20sHHHIII',name('tone'),0,0,0,0,0,0)+s.pack('<20sHHHIII',name('EOP'),0,0,1,0,0,0)),chunk(b'pbag',s.pack('<4H',0,0,len(pg)//4-1,0)),chunk(b'pmod',bytes(10)),chunk(b'pgen',pg),chunk(b'inst',s.pack('<20sH20sH',name('tone'),0,name('EOI'),len(ib)-1)),chunk(b'ibag',b''.join(s.pack('<HH',x,0 if i==0 else len(mod_extra)) for i,x in enumerate(ib))),chunk(b'imod',b''.join(s.pack('<HHhHH',0,x,1,0,0) for x in mod_extra)+bytes(10)),chunk(b'igen',ig),chunk(b'shdr',hdr))
 return chunk(b'RIFF',b'sfbk'+info+sd+pd)
def tables(b):
 result={}
 def walk(a,z):
  while a+8<=z:
   k,n=s.unpack_from('<4sI',b,a);p=a+8
   if k==b'LIST':walk(p+4,p+n)
   else:result[k]=b[p:p+n]
   a=p+n+(n&1)
 walk(12,len(b));return result
cases=[('valid',True,{}),('inactive_invalid_header',True,{'mode':0,'ls':100,'le':20000}),('inactive_invalid_offset',True,{'mode':0,'offsets':[(2,30000),(50,20)]}),('active_guard1',True,{'le':1001}),('active_guard2',True,{'le':1002}),('active_guard3',False,{'le':1003}),('active_guard_nonzero',False,{'le':1002,'nonzero_guard':True}),('active_guard_overlap',False,{'le':1002,'next_sample':True}),('active_loop_oob',False,{'le':0xffffffff}),('active_reversed_loop',False,{'ls':999,'le':900}),('active_bad_offset',False,{'offsets':[(45,10)]}),('invalid_end',False,{'mode':0,'end':2000,'le':20000}),('invalid_start',False,{'mode':0,'start':1001,'le':20000}),('global_active',False,{'mode':None,'global_zone':[(54,1)],'le':20000}),('global_offset_unoverridden',False,{'mode':0,'global_zone':[(2,30000)]}),('active_guard_address_mod',False,{'le':1002,'offsets':[(0,1)]}),('preset_mode_override',False,{'mode':0,'le':20000,'preset_extra':[(54,1)]}),('instrument_address_mod',False,{'mode':0,'le':20000,'mod_extra':[2]}),('bad_link',False,{'mode':0,'le':20000,'wrong_link':True})]
with tempfile.TemporaryDirectory() as d:
 d=pathlib.Path(d)
 for title,expected,kw in cases:
  src=d/(title+'.sf2');out=d/(title+'.out'); original=make(**kw);src.write_bytes(original)
  r=subprocess.run([str(exe),str(src),str(out)],capture_output=True,text=True)
  actual=r.returncode==0
  assert actual==expected,(title,expected,r.returncode,r.stdout,r.stderr)
  assert src.read_bytes()==original
  if actual:
   before=tables(original);after=tables(out.read_bytes());assert before[b'smpl']==after[b'smpl']
   if title.startswith('active_guard') or title=='valid':
    assert before[b'shdr']==after[b'shdr'] and before[b'igen']==after[b'igen']
   if title=='inactive_invalid_header':assert U32(after[b'shdr'],28)==0 and U32(after[b'shdr'],32)==1000
   if title=='inactive_invalid_offset':assert U16(after[b'igen'],2)==0 and U16(after[b'igen'],6)==0
  else:assert not out.exists() and r.returncode==1
  print(title,'PASS')
 print(f'{len(cases)}/{len(cases)} PASS')
