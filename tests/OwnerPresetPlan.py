"""Read-only private SF2 note/velocity coverage inventory, not a synth.
One probe for each distinct reachable set of sample zones, plus representative
low/middle/high keys and velocities. Bank 0 is the only proven product route.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def require(ok, message):
    if not ok:
        raise ValueError(message)


def chunks(data):
    pos = 0
    while pos < len(data):
        require(pos + 8 <= len(data), 'truncated RIFF chunk')
        tag, size = struct.unpack_from('<4sI', data, pos)
        end = pos + 8 + size
        require(end + (size & 1) <= len(data), 'RIFF chunk bounds')
        yield tag, data[pos + 8:end]
        pos = end + (size & 1)


def unique(items):
    result = {}
    for tag, value in items:
        require(tag not in result, 'duplicate chunk ' + repr(tag))
        result[tag] = value
    return result


def range_of(zone, op, default):
    value = zone.get(op)
    if value is None:
        return default
    lo, hi = value & 255, value >> 8
    require(0 <= lo <= hi <= 127, 'invalid key/velocity range')
    return lo, hi


def groups(headers, bags, generators, terminal_op):
    result = []
    require(len(headers) >= 2 and bags and generators, 'empty hydra table')
    require(headers[0] == 0 and headers[-1] == len(bags)-1, 'header index bounds')
    require(bags[0][0] == 0 and bags[-1][0] == len(generators)-1, 'generator bounds')
    for first, last in zip(headers, headers[1:]):
        require(first <= last < len(bags), 'invalid bag bounds')
        global_zone = {}
        local = []
        for k in range(first, last):
            a, b = bags[k][0], bags[k+1][0]
            require(a <= b < len(generators), 'invalid generator range')
            # Some legacy Creative banks repeat a terminal sampleID within
            # a single bag. Treat each terminated subzone separately.
            entries = generators[a:b]
            part = {}
            for op, value in entries:
                require(op not in part, 'duplicate generator before terminal')
                part[op] = value
                if op == terminal_op:
                    local.append(global_zone | part)
                    part = {}
            if part:
                require(k == first and not local and not global_zone, 'unlinked local zone')
                global_zone = part
        # Empty instruments are retained, never silently counted as audible.
        result.append(local)
    return result


def table(pdta, name, fmt):
    return list(struct.iter_unpack(fmt, pdta[name]))


def inventory(raw):
    require(len(raw) >= 12 and raw[:4] == b'RIFF' and raw[8:12] == b'sfbk', 'not SF2')
    require(struct.unpack_from('<I', raw, 4)[0] + 8 == len(raw), 'RIFF size mismatch')
    lists = unique((data[:4], unique(chunks(data[4:]))) for tag, data in chunks(raw[12:]) if tag == b'LIST')
    pd = lists[b'pdta']
    ph = table(pd, b'phdr', '<20sHHHIII')
    ih = table(pd, b'inst', '<20sH')
    sh = table(pd, b'shdr', '<20sIIIIIBbHH')
    require(ph[-1][0].startswith(b'EOP') and ih[-1][0].startswith(b'EOI') and sh[-1][0].startswith(b'EOS'), 'missing terminal')
    pg = groups([h[3] for h in ph], table(pd,b'pbag','<HH'),table(pd,b'pgen','<HH'),41)
    ig = groups([h[1] for h in ih], table(pd,b'ibag','<HH'),table(pd,b'igen','<HH'),53)
    result = []
    seen = set()
    for header, zones in zip(ph, pg):
        name, program, bank = header[:3]
        require(program <= 127 and bank <= 16383 and (bank,program) not in seen, 'invalid/duplicate preset address')
        seen.add((bank,program))
        regions = []
        for p in zones:
            require(p[41] < len(ig), 'invalid instrument link')
            for z in ig[p[41]]:
                require(z[53] < len(sh)-1, 'invalid sample link')
                pkey, ikey = range_of(p,43,(0,127)), range_of(z,43,(0,127))
                pvel, ivel = range_of(p,44,(1,127)), range_of(z,44,(1,127))
                kl,kh = max(pkey[0],ikey[0]), min(pkey[1],ikey[1])
                vl,vh = max(1,pvel[0],ivel[0]),min(pvel[1],ivel[1])
                if kl <= kh and vl <= vh:
                    regions.append((kl,kh,vl,vh))
        probes = select_probes(regions)
        result.append({'bank':bank,'program':program,'name':name.split(b'\0')[0].decode('latin1').strip(),
                       'regions':len(regions),'screen':screen_probes(regions),'probes':probes,'legacy_probe_reachable':any(a<=60<=b and c<=100<=d for a,b,c,d in regions),
                       'status':'PLANNED' if bank==0 and probes else 'NOT_ADDRESSABLE' if bank!=0 else 'NO_REACHABLE_ZONE'})
    return result


def screen_probes(regions):
    # First pass uses three representative key/velocity pairs rather than
    # extreme MIDI values which can be intentionally nearly silent.
    result = set()
    for key, vel in ((48, 65), (60, 100), (72, 115)):
        if not regions:
            break
        candidates = [(max(a,min(b,key)),max(c,min(d,vel))) for a,b,c,d in regions]
        result.add(min(candidates,key=lambda p:(abs(p[0]-key)+abs(p[1]-vel),p)))
    return sorted(result)


def select_probes(regions):
    if not regions:
        return []
    # Partition by rectangle endpoints. The signature is constant inside
    # each cell. Cover every distinct reachable signature, not just every
    # zone: overlapping layers can otherwise conceal a broken layer.
    keys=sorted({x for a,b,c,d in regions for x in (a,b+1)})
    velocities=sorted({x for a,b,c,d in regions for x in (c,d+1)})
    points={}
    for lo,hi in zip(keys,keys[1:]):
        key=(lo+hi-1)//2
        for low,high in zip(velocities,velocities[1:]):
            vel=(low+high-1)//2
            signature=tuple(i for i,(a,b,c,d) in enumerate(regions) if a<=key<=b and c<=vel<=d)
            if signature:
                # Prefer points near note 60 / velocity 100 when equivalent.
                score=(abs(key-60)+abs(vel-100),key,vel)
                if signature not in points or score<points[signature]:
                    points[signature]=score
    probes={(key,vel) for _,key,vel in points.values()}
    # Add low/mid/high pitch and velocity coverage within reachable zones.
    lo=min(a for a,_,_,_ in regions); hi=max(b for _,b,_,_ in regions)
    for target in (lo,(lo+hi)//2,hi):
        a,b,c,d=min(regions,key=lambda r:(max(r[0]-target,target-r[1],0),abs((r[0]+r[1])/2-target)))
        probes.add((max(a,min(b,target)),max(c,min(d,100))))
    for target in (min(c for _,_,c,_ in regions),64,max(d for _,_,_,d in regions)):
        a,b,c,d=min(regions,key=lambda r:(max(r[2]-target,target-r[3],0),abs((r[0]+r[1])/2-60)))
        probes.add((max(a,min(b,60)),max(c,min(d,target))))
    return sorted(probes)


def plan(directory):
    results=[]
    for path in sorted(directory.iterdir(),key=lambda p:p.name.casefold()):
        if not path.is_file() or path.suffix.lower()!='.sf2':continue
        raw=path.read_bytes()
        results.append({'file':path.name,'sha256':hashlib.sha256(raw).hexdigest(),'size':len(raw),'presets':inventory(raw)})
    require(results,'no SF2 files')
    return results


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    result = plan(args.directory)
    with args.output.open('x',encoding='utf-8') as fp:
        json.dump(result,fp,indent=2,ensure_ascii=False)
