"""Private coverage accounting for completed actual VST3 owner-sweep reports.
No sound approval, no private assets written to the repository. Reparse raw logs
and verify exact candidate/source identity; keep previous anomalies visible.
"""
import argparse
import hashlib
import json
from pathlib import Path
from collections import Counter, defaultdict
from OwnerPresetPlan import plan
from WindowsOwnerPresetSweep import parse_report


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def expected_identity(manifest):
    return {k: manifest[name] for k, name in (
        ('vst3', '125A AWE64 Gold.vst3'), ('engine', 'Engine.dll'),
        ('host', 'awe64_vst3_host_pcm_smoke.exe'))}


def collect(banks, identity, reports):
    """reports: iterable of (report JSON path, raw host log path)."""
    inventory = {b['file']: b for b in banks}
    planned = {(b['file'], p['program'], k, v)
               for b in banks for p in b['presets'] if p['status']=='PLANNED'
               for k, v in p['screen']}
    measured = defaultdict(list)
    rejected = []
    accepted = []
    for rp, lp in reports:
        try:
            r = json.loads(rp.read_text(encoding='utf-8'))
            if r.get('identity') != identity:
                raise ValueError('different executable identity')
            if r.get('source_sha') != inventory[r['bank']]['sha256']:
                raise ValueError('different source identity')
            if type(r.get('exit')) is not int or r['exit'] != 0 or r.get('boot_exists') is not True or r.get('boot_error') is not False:
                raise ValueError('missing successful process/boot evidence')
            if digest(lp) != r['log_sha']:
                raise ValueError('raw log digest mismatch')
            probes = [tuple(x) for x in r['probes']]
            if any(len(x)!=3 or (r['bank'], *x) not in planned for x in probes):
                raise ValueError('unplanned screening probe')
            raw = lp.read_text(encoding='utf-8', errors='replace')
            rows = parse_report(raw, probes)
            if rows != r['rows']:
                raise ValueError('stored rows do not match raw measurements')
        except (ValueError, KeyError, TypeError, OSError) as exc:
            rejected.append({'report':rp.name,'reason':str(exc)})
            continue
        accepted.append(rp.name)
        for row in rows:
            key = (r['bank'], row['program'], row['key'], row['velocity'])
            measured[key].append(row['status'])
    # Contaminated rows provide no held-note measurement. Retests cannot erase
    # an earlier silence/step review. A measured signal is not sound approval.
    covered = {k for k, statuses in measured.items() if any(s!='CONTAMINATED' for s in statuses)}
    signal = {k for k, statuses in measured.items() if any(s in ('SIGNAL_OBSERVED','LARGE_STEP_REVIEW') for s in statuses)}
    reviews = {k:sorted(set(s)-{'SIGNAL_OBSERVED'}) for k,s in measured.items() if set(s)-{'SIGNAL_OBSERVED'}}
    by_bank = []
    for name in sorted(inventory):
        required = {k for k in planned if k[0]==name}
        done = required & covered
        sounded = required & signal
        by_bank.append({'bank':name,'planned':len(required),'measured':len(done),'signal':len(sounded),
                        'presets_with_signal':len({k[1] for k in sounded}),
                        'complete_screen':bool(required) and done==required,
                        'remaining':[list(k[1:]) for k in sorted(required-done)],
                        'review':[{'probe':list(k[1:]),'statuses':s} for k,s in sorted(reviews.items()) if k[0]==name]})
    return {'status':'MEASUREMENT_COVERAGE_NOT_SOUND_APPROVAL','planned':len(planned),
            'measured':len(covered),'signal':len(signal),'complete_banks':sum(x['complete_screen'] for x in by_bank),
            'presets_with_signal':len({k[:2] for k in signal}),'accepted_reports':accepted,'rejected_reports':rejected,
            'banks':by_bank}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--banks',type=Path,required=True)
    p.add_argument('--manifest',type=Path,required=True)
    p.add_argument('--reports',type=Path,nargs='+',required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    result=collect(plan(a.banks),expected_identity(json.loads(a.manifest.read_text(encoding='utf-8-sig'))),
                   [(r,r.with_suffix('.log')) for r in a.reports])
    with a.output.open('x',encoding='utf-8') as fp:
        json.dump(result,fp,indent=2,ensure_ascii=False,allow_nan=False)
    print({k:v for k,v in result.items() if k not in ('banks','accepted_reports','rejected_reports')})
    return 1 if result['rejected_reports'] else 0


if __name__=='__main__':
    raise SystemExit(main())
