"""Direction-specific PCM oracle for the known two-program overlay fixture.
Zero is preset bypass, not the low active endpoint. Compare active .5 / 1.
The positive filter route in this fixture starts bright and decays darker.
Fixed half-open 100-ms window ranges isolate each control's own phase.
"""
import math
import re
from statistics import mean

# id, first/last bin, sign of maximum-minus-midpoint, feature
SPECS = ((0,3,-1),(9,12,1),(11,16,1),(17,23,1),(3,7,1),
         (8,14,1),(11,16,1),(17,21,1),(11,16,1),(3,5,1))


def measure(profile, ident):
    rms, diff, harmonics = profile
    first, last, _ = SPECS[ident]
    if ident < 4:
        return mean(rms[first:last])
    ratios = []
    for n in range(first,last):
        h = harmonics[n*8:(n+1)*8]
        low = sum(v*v for v in h[:3])
        high = sum(v*v for v in h[3:])
        if low < 1e-10 or rms[n] < .0005:
            raise RuntimeError(f"Insufficient spectral energy for control {1100+ident}")
        ratios.append(math.sqrt(high/low))
    return mean(ratios)


def directions(profiles):
    """Return all measured signed effects, including failures; no retry-to-green."""
    results = []
    for i, (_,_,sign) in enumerate(SPECS):
        program = 17 if 5 <= i <= 7 else 0
        neutral = [measure(x,i) for x in profiles[(-1,0.,program)]]
        mid = measure(profiles[(i,.5,program)][0],i)
        maximum = measure(profiles[(i,1.,program)][0],i)
        noise = max(neutral)-min(neutral)
        delta = sign*(maximum-mid)
        # Reject changes indistinguishable from neutral repeatability, tiny
        # absolute effects, or effects smaller than 5% of the feature scale.
        threshold = max(3*noise,.05*max(mid,maximum),.0001 if i<4 else .005)
        results.append((1100+i,mid,maximum,noise,delta,threshold,delta>threshold))
    return results


def validate_direction_report(stdout, profiles):
    lines=stdout.splitlines()
    if lines.count('REAL_VST3_VOICE_PROFILE_CAPTURE_COMPLETE')!=1:
        raise RuntimeError('Native PCM capture or teardown not established')
    if sum(bool(re.fullmatch(r'REAL_VST3_VOICE_PCM_SUMMARY=(?:[0-9]|10)/10 NEUTRAL_RESTORE=PASS',line)) for line in lines)!=1:
        raise RuntimeError('Missing neutral restoration evidence')
    refs=[line for line in lines if line.startswith('FILTER_PCM_REFERENCE ')]
    if len(refs)!=1:
        raise RuntimeError('Missing filter reference')
    m=re.fullmatch(r'FILTER_PCM_REFERENCE PEAK=(\S+) LATE_HELD=(\S+) RELEASE_450MS=(\S+) VALID=PASS',refs[0])
    if not m:
        raise RuntimeError('Invalid filter reference')
    peak, held, release=map(float,m.groups())
    if not all(math.isfinite(x) for x in (peak,held,release)) or not (peak>=.002 and .2*peak<=held<=peak*1.00001 and .1*peak<=release<=peak*1.00001):
        raise RuntimeError('Filter reference values contradict PASS')
    silence=[line for line in lines if line.startswith('FILTER_PRE_NOTE_SILENCE ')]
    if len(silence)!=9:
        raise RuntimeError('Missing measured pre-note silence')
    for line in silence:
        m=re.fullmatch(r'FILTER_PRE_NOTE_SILENCE RMS=(\S+) WINDOWS=(\d+) PASS',line)
        if not m or not (0<=float(m[1])<=.00025 and 3<=int(m[2])<=100):
            raise RuntimeError('Invalid measured pre-note silence')
    results=directions(profiles)
    for i,mid,maximum,noise,delta,threshold,passed in results:
        print(f'VOICE_DIRECTION ID={i} MID={mid:.8g} MAX={maximum:.8g} NOISE={noise:.8g} SIGNED_DELTA={delta:.8g} THRESHOLD={threshold:.8g} {"PASS" if passed else "FAIL"}')
    if not all(x[-1] for x in results):
        raise RuntimeError('One or more acoustic control directions not established')
    return results
