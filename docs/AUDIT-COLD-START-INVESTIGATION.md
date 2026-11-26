# Historical cold-start investigation — 26 November 2026

**Root cause of observed 12:15 UTC failure: NOT VERIFIED.** Do not conflate mitigation with historical attribution.

## Evidence

- Exact historical product ebbc14895fbe41c57eb08ea14af67b81586334bf, Windows adversarial run [45817583829](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45817583829), job 139901196715. Last emitted upstream boot log at 12:15:56.6561182 UTC: Final arrangement: Below 4GB = 16384KB, Above 4GB = 0KB. At 12:15:59.5922188 both early and late peaks zero; process errors zero. Native host immediately returned via fail() and did not deactivate the component or emit teardown counters. No UART readiness, queue depth, PCM reserve, guest exit status or later progress recorded.
- Same exact product, runtime run [45817583796](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45817583796), job 139901196202: three independent early PASS, peak 0.304871, 0.304871, 0.304901; same mkdir_recursive Permission denied messages and same early init messages. These messages are not a discriminator. Earlier failure was direct PowerShell execution; successful comparison used Python subprocess captured streams on a different runner. This is correlation, not causation.
- Synthetic MZ fixture requires precise MIDI bytes, then emits a two-second PC speaker tone and exits. Late peak is not an independent proof of a second note (may include first-note tail). Both zero proves no audible response during original window, not specific loss of first event.
- Subsequent watchdog/SDL/process-policy/config changes reduce known risks; [45879596716](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45879596716) 20/20 targeted native tests incl. config isolation. These prove specific injected failures, not historical causality. Exact full current original regression NOT RUN.

## Falsifiable experiment

Build both historical ebbc14895fbe41c57eb08ea14af67b81586334bf and current f513c0dbca5f951f895f29be5230fdc241aa1fc7 at exact commits and pinned upstream, same diagnostic host from investigation checkout. Product implementation unmodified. For each, interleave 6 inherited-stream and 6 captured-stream fresh child processes. Original 220-block early and 70-block late deadlines remain unchanged; on silence the common diagnostic host continues only for evidence, then deactivates and emits counters. Failure remains failure even if delayed audio succeeds. Capture full streams, exit status, duration, current UART/PCM/prime diagnostic and available historical queue/pull counters. Results are not interchangeable with authentic Creative or owner ASIO tests. Separate runners for versions limit cross-version causality; launcher methods are compared within each runner. No retries-to-green. A zero-failure experiment cannot retrospectively identify a non-reproduced event; further instrumentation or controlled reproduction would still be needed.

## Executed result

[Run 45881429683](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45881429683), historical job 140121462697 and current job 140121462412, both SUCCESS. Exact historical: 6/6 captured + 6/6 inherited. Exact current: 6/6 captured + 6/6 inherited. All 24 measured first-note peaks 0.304871–0.304901, PROCESS_ERRORS=0. All 12 current shutdown diagnostics UART=1, max_pcm=1024, primed=1, timeout=0, pulled_notes=1. Historical failure did not recur; no evidence supports launcher attribution. Historical root cause remains NOT VERIFIED; no further speculative product changes. Retain the proven mitigations and diagnostic visibility, resume full regression.

This was a synthetic fixture experiment, not a 128-preset Original Creative or owner DAW acceptance.
