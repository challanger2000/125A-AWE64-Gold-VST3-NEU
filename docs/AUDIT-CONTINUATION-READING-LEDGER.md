# Audit continuation and revision-pinned reading ledger — 2026-11-26

## Verified source and continuation

Product main before documentation checkpoint: `6133a1a8d59e803f0f37514f456330feca1f534d`; version 0.0.0-dev. Product audit correction 8368fd95; original Windows workflow correction 49de4439; latest 6133a1a8 changes test fixture and audit documentation only. No new production edit or build in this checkpoint. Keep single main branch, Creative original driver, pinned EMU8000 and P165 design.

## Completed CI evidence, exact scope

- [Native adversarial Windows job 139638474533](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738813550/job/139638474533), product 8368fd95: A07-O non-silent offline 289771 nonzero frames, peak 0.304871, elapsed 13110 ms, 0 process errors; A06-I accepted bank retained program 0/state_same=1; A01-O byte order vNNV as expected. Native job SUCCESS. This does not prove sample-exact timing, PDC, original SF2 offline render, or cross-process recall.
- Same run's portable job failed on malformed regex (not a production failure); [corrected portable 45738889998](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738889998) SUCCESS, seven adversarial negative controls. [Latest Linux/Windows unit suite 45738970027](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738970027) and version 45738970411 on 6133a1a8 SUCCESS.
- [Original Windows run 45738890137](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738890137), head 49de4439, job 139638836044 completed SUCCESS. Actual logs inspected: validator 47/47; verified original ROM, Creative DOS EMU8000 PCM and NoteOn/Off, 12903 port writes; Cheezoids 128/128 comparison; 10/10 voice-control PCM and neutral restore; RAM stereo tone and settled release; original dual instance PCM lifecycle; p95 19.7959 / 18.9167 / 17.6458 ms (three rate gate), zero recorded audio/MIDI drops and underrun frames in these measurements. No owner DAW/physical ASIO or hardware identity claim. A13 native PCM and register checks positive in measured scope, not proof of all concurrency interleavings. A25 original dual PASS within the test gate, not universal isolation.
- [Runtime host faults 45738813482](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738813482), [SF2 state 45738813457](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738813457), [ABI faults 45738813528](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738813528) on 8368fd95 completed SUCCESS. Logs inspected for pause, automation, bank rollback, bounded stop 2508 ms plus reaping, CC7 replay, ROM snapshot and startup gate.
- A22 general in-process fatal exit, abort, SEH / host process protection remains OPEN. A12 GUI host rejection, A19 private build provenance, complete timing/host/owner matrix and independent full audit remain OPEN. Do not mark FULL AUDIT or FINAL; no speculative build.

## Engineering mandatory reading checkpoint (same assignment, current immutable revision)

Source repository challanger2000/125A-Engineering commit 53b10bd4a81fe6c5d62e110b932aef7155172ad6.
- START-HERE.md blob 91a0e14dd24ff72874bbedf58962e0e8c14c444e: full content inspected in preceding turn of this continuation.
- README.md blob bd94e5a896bcc905af65ca5659e533eee854b46f: full content inspected in preceding turn.
- ENGINEERING-GUARDRAILS.md blob 8a076e21d8226a5b1ded7061362590415fe4839f: 3186 non-trailing lines (3187 with terminal split); consecutive inspected ranges 1–100, 101–200, 201–260, 261–330. Earlier 201–330 response truncated; both smaller ranges subsequently received untruncated. Next unread line 331 through EOF. Previous audit document reports verified prior receipts but does not expose their exact ranges/blob list; do not assume that claim alone establishes completion. If recoverable, verify original receipt against current exact blobs instead of repeating completed material.
- Actionable anchors: lines 1–40 challenge structural complexity, preserve proven original GUI; 40–100 rollback coherence, real PCM and host test adequacy, sample boundaries and independent counterexamples; 101–200 complete reading, coherent asynchronous bank/recall transactions, preserve owner contracts; 201–330 actual host lifecycle and test adequacy, exact SDK interfaces, literal regex/YAML edits, adversarial PCM, exact valid I/O contract.
- Relevant standards and QA, including QA/AUDIT-COMPLETENESS-CONTRACT.md (blob 1bb7508e1fd482503d0c1198f2a8e38f40e0a88f), relevant research/roadmap, release register and applicable project mandatory documents still need complete verified reading or revision-pinned prior receipt. NOT READ / incomplete in current continuation; no new implementation authorized by this checkpoint.
- Resume at 331, not line 1. Do not treat retrieved/count-only text as inspected. Advance mandatory reading before implementing A22. Evaluate bounded recovery for known fatal paths, but no claim of complete process isolation from C++ exceptions or a worker thread in the same address space. Do not change approved architecture without owner authorization; any modification must be justified and tested. No source/build trigger from checkpoint.


## Reading continuation — completed Engineering coverage for A22

At unchanged Engineering commit 53b10bd4a81fe6c5d62e110b932aef7155172ad6, START-HERE and README previously read in full; guardrails blob 8a076e21d8226a5b1ded7061362590415fe4839f now inspected through EOF. Additional consecutive ranges: 331–430, 431–530, 531–630, 631–730, 731–850, 851–1000, 1001–1160, 1161–1360, 1361–1560, 1561–1760, 1761–1960, 1961–2160, 2161–2360, 2361–2500, 2501–2700, 2701–2820, 2821–2920, 2921–3030, 3031–3110, 3111–3187. Truncated 3031–3187 retrieval replaced by the last two smaller ranges. No unread guardrails ranges remain.

Read in full at that same commit:
- STANDARDS/AI-ASSISTED-ENGINEERING.md 12ce40b84b2acd79544dbd56ddb8c88cf342f4fd
- STANDARDS/DSP-GUIDELINES.md 0307c6c8d9b7158f9c1f9ffb467d83ee8d23a4ae
- STANDARDS/PROFESSIONAL-PLUGIN-BEHAVIOR.md d9c696ee522faf84010f783f358b11c8c11dbdb5
- STANDARDS/REPOSITORY-WORKFLOW.md e3553244e78a82f2ba1d4cc4936daca0e57d7bac
- STANDARDS/SOURCE-BASELINE-GATE.md 47a7ba32d333158b2bd84d7464e4c084a2bb40c4
- STANDARDS/GUI-GUIDELINES.md 556fdbf3568a77b8b2aebc2c7c14190e2ced31f1
- QA/AUDIT-COMPLETENESS-CONTRACT.md 1bb7508e1fd482503d0c1198f2a8e38f40e0a88f
- QA/RELEASE-QA.md fb11d2a3057817b14234d13563edc200b5e7b147
- QA/REALTIME-QA.md 9a157249bd1e6a561bf8a527554ea2fdbe6f8a8a
- QA/AUDIO-FIXTURES.md 367c478d8c96970fc23d5d58ab712830ed537687
- RESEARCH/SOURCES.md 7dc00c32374c3976aa0780825eccc29e5306ee1f
- RESEARCH/OPEN-QUESTIONS.md c7bb5ed4a798b7d9b19bec61af849d4af940abc1
- RESEARCH/REJECTED-APPROACHES.md e21ea8ccc179aff8a789d942cbd05141997dc7cd
- RESEARCH/RESAMPLING-OVERSAMPLING.md 2aa90ce2c60bffa8f1c489b9706de98e23dc90b8
- RESEARCH/EXTERNAL-PROJECTS/pluginval.md f665f5ca356e6b4ff5cb8b61c8855f3cddad730d
- ROADMAP/V3-CANDIDATES.md b41bbfb179b2e64debe2783603082cb7985daa2f

Task scope: unreleased 0.0.0-dev; no artwork, commercial licensing, new project, or V3 upgrade. Preserve explicit owner single-main architecture. Read product START-HERE f3b413e8d7300ec4d568c32b37d33ad65efca515, docs/PROVENANCE.md 4e1a064e9034ea968c12acc270b0f20bdef8c8b5, docs/TEST-ASSET-LOCATION.md 9a0a6365a30465fd3b1b5e73bfd0bb70986453a3 in full. Current sections AKTUELLE-VERSION.md c462be9828a9ba005e25adb85f9ae4f00e96f5a7 lines 1–55 and CHANGELOG.md ee890f5d4bb84c68ea03eba51742c74aeb39aa74 lines 1–75 inspected; historical remainder not claimed fully inspected. AGENTS previously inspected. Relevant decisions: do not mistake controlled E_Exit recovery for universal native crash containment; keep original core and driver behavior, isolate faults within explicit architecture limits, test true negative cases and valid recovery; no RT allocation or waits; exact SDK signatures and local preflight before CI; maintain atomic documentation+code commits. Source audit and implementation still outstanding. This checkpoint supersedes the earlier unread Engineering ranges; do not repeat them unless blob changes.
