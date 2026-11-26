# Original regression and deadline investigation — 26 November 2026

Exact regression `3a53c1db2523fd492fa55084d2b7f444cf0393bc`, [run 45883848769](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45883848769), Windows job 140129709875: FAIL, independent gate performance=1 timed=0. All preceding native DLL/ABI/GUI/validator gates PASS, 20/20 fault tests, 47/47 validator, original dual, all 128 Cheezoids envelope comparisons (not hardware identity). RAM spectral, voice overlays, send and effect gates completed, final wrapper reached only performance failure. Original source code unchanged from f513c0db.

| rate | block | p95 MIDI/PCM ms | max callback ms | overrun | queue blocks | drops/underrun |
|---|---:|---:|---:|---:|---:|---:|
| 44100 | 128 | 19.3651 | 0.5191 | 0 | 9254 | 0 |
| 48000 | 128 | 19.8958 | 2.2295 | 0 | 28522 | 0 |
| 96000 | 128 | 18.625 | 2.2166 | 1 | 14269 | 0 |

96-kHz budget 1.33333 ms. The host measures wall time around process(); no evidence whether the one callback was running CPU or descheduled. ETW overall CPU valid, 32-note whole-machine 4.12043% on 4 logical CPUs, not owner's PC. Do not waive strict zero-overrun gate. No production modification based on this single measurement.

Targeted experiment: 3 fixed independent 96-kHz/128-sample original-Creative runs, exact existing RAM fixture and unchanged Python acceptance gates, additional QPC bracket and ETW scheduled time of the host calling thread for the single worst callback. Instrumentation is runner-local in host only; the two additional QPC calls bracket rather than occur inside existing wall-clock measurement. Ratio is diagnostic only, QPC interval slightly wider than wall-clock; ETW scheduling cannot distinguish preemption from blocking or hypervisor steals, nor prove plugin algorithm cost. No trace upload. Missing interval NOT VERIFIED. No retries-to-green; previous failure preserved even if no recurrence. Original complete regression cannot be labeled PASS until unresolved performance gate is assessed. No release/DAW go yet. Execution NOT RUN.
