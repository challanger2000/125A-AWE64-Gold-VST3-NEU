# Independent Test Lab checkpoint — 26.11.2026

Product source under test: `0ff08116701efd831b9fca00970f8b059ae9b9af` (all six product workflows passed).
Original gate [45731174304](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174304): 47/47 validator, Cheezoids 128/128 comparison, p95 18.9569 / 18.6042 / 18.625 ms, zero measured transport drops/underruns. [Native startup](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174350): three independent cold starts PASS, all remaining modes PASS. [Native ABI](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174401): priming gate PASS.

The new isolated workflow pins the exact same product revision and independent Test Lab `7213fe1f81f4036f74431cb5b2d7dbd09b73ee5c`, builds native Windows hosts, stages exclusively synthetic fixtures, and invokes unmodified P74 ABI/GUI inventory plus technical baseline (playback/save, sample-rate/buffer matrix, native audio/state, fresh-process recall where supported). It publishes reports only, not the binary or fixtures. No original-clang suite is retriggered by this workflow-only commit. No production code changed. Generic host timing or offline behavior must be interpreted against the existing asynchronous guest contract, not silently marked PASS. A P74 failure requires diagnosis; a PASS does not establish musical correctness, feature completeness, physical ASIO or commercial DAW approval. Independent Plugin Tester (private repository) is still NOT RUN; prior passing Lab reports belong to different source. General in-process native fatal exception isolation is not guaranteed. FULL AUDIT and final Studio Pro release remain open.

Result: NOT RUN until actual workflow finishes.

## 26.11.2026 — Current candidate recheck

Previous pinned baseline Lab workflow [45734851189](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45734851189) SUCCESS (older source only). New product pin `aabcc0939b72aaeea7b90535493b1fb1196f627b` after successful full original regression 45900070562 and targeted performance 45900070547; no product modification. Same immutable Lab revision `7213fe1f81f4036f74431cb5b2d7dbd09b73ee5c`. Current result NOT RUN until actual evidence. Synthetic-only evidence; no original PCM, owner DAW or release claim.
