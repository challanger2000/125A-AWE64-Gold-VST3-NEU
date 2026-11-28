# Preset playback repair candidate — 2026-11-27
Owner baseline 65c419e, display correction e41199c. Local private Wine x64 direct guest reproduction (not native DAW): actual Bass Synth program 0 has ~9700 int16 jumps every 5116 frames at MIDI 60, also on pre-optimization 609ab17. Filter-LFO-only zero counterfactual removes large jumps. State dump: IFATN FF15, Q0, FMMOD 007F, signed filter modulation oscillates across zero while bypass freezes filter state. Candidate retains the static unmodulated bypass and continuous filtering during potentially downward modulation. Actual patched PCM still needs verification.
Synth 512 actual programs 8,12,21 send no note register writes. Neutral extra preset generator records on program 0 establish 20 accepted / 21 silent. Local private compatibility experiment moves only equivalent additive values from preset global to exclusively owned instrument globals, removes zero local loop offsets without inherited offsets, retains samples and local overrides. All 15 overloaded programs (8,12,21,32,34,35,45,47,50,52,55,57,70,74,77) produce nonzero PCM at MIDI 60 in prototype; three-pitch tests performed; this is not full all-program native acceptance. Source files are never modified. The C++ integration and new engine require build and comparison against exact private banks; shared/modulated/unsupported cases are deliberately not rewritten. No owner media, audio or state dump in repository.

## 28.11.2026 — Private Owner-SF2-Inventur und Massentest-Kandidat (PARTIAL)

Scope: exakter main-Produktstand 2c233f3c52e88c44c01156db84ef57c57ed5ba92, Linux-Lokalumgebung, originales privates SF2.rar (56.128.364 Bytes). Alle 22 SF2 aus dem Archiv lokal vollständig extrahiert, Originaldateien unverändert. Separate Owner-Uploads Synth 512 Bank(5).sf2 und Bass Synth.sf2 stimmen mit Archivdateien überein (SHA256 d5b99ec38fddf59f53f66b90921e170d207bf3bbd73960f6e4175c39ef68349b und 984883a7366f62fe0f1e3831c9bd4dae465ad87a5446c15748d379a0417e4f59). Kein SoundFont, kein Audio und kein privater Plan im Repository.

| Inventur (tatsächlich ausgeführt) | Ergebnis |
| --- | ---: |
| SF2-Dateien | 22 |
| Reale phdr-Presets | 955 |
| Adressen in Bank 0 | 926 |
| Nicht adressierbare Presets außerhalb Bank 0 | 29 |
| Bank-0-Presets, bei denen bisher Note 60/Velocity 100 keine Zone erreicht | 14 |
| Ersttest (3 Note-/Velocity-Paare pro Bank-0-Preset) | 2.778 |
| Erweiterte Zonen-/Randwertproben in Bank 0 | 11.722 |

Die 29 nicht adressierbaren Presets befinden sich in Vintage Dreams Bank (8), Vintage Dreams Drums (8), Vintage Drums 2001 (13). Produktcode akzeptiert nur Bank 0. Keine stillschweigende Gleichsetzung mit getesteten Presets und keine automatische Bankumsetzung. Bekannte Synth-512- und Bass-Synth-Korrekturkandidaten bleiben unverändert.

- OwnerPresetPlan.py (Blob 96306601468ab6b209ccf7db36f9747cf8c0d735) liest tatsächlich die Preset-/Instrument-/Sample-Zonen inklusive Key/Velocity-Schnittmengen; berücksichtigt lokale Overrides und Legacy-Subzonen; leere Instrumente werden nicht als klingend behandelt. Zwei Stufen: repräsentatives Screening und erweiterte Auswahl je unterschiedlicher erreichbarer Layer-Kombination. Lokal erzeugter privater Plan SHA256 875ca9bf56a5110f0669c3dee753c7c6ea5a4468205fb036d486d59dafed5516. Unabhängige vollständige Rasterprüfung der Auswahl gegen 128 × 127 NoteOn-Kombinationen bei allen 955 Presets: 7.117 unterschiedliche erreichbare Layer-Signaturen insgesamt, sämtliche im erweiterten Plan vertreten. Das ist eine Zoneninventur, keine Audio- oder SF2-Kompatibilitätsabnahme. Ständig gemeinsam ausgelöste Layer sind in aggregiertem PCM nicht individuell isoliert.
- Native Host-Sweep-Kandidat erhält explizite Program/Key/Velocity-Tripel via stdin, initialisiert auch sparse Banken ohne Program 0, bleibt ein Hostprozess pro Bank. CC120 + gemessene Ruhe vor jeder Probe, drei Sekunden Hold, ca. eine Sekunde Release. Ausgabe: RMS, maximaler blockzentrierter AC-RMS, Peak, größter Sample-Sprung und Anzahl Sprünge >0,15 einschließlich Blockgrenzen, Release-RMS, Frames. Ein kontaminierter Ausgang wird als solcher gemeldet, nicht als Ton. Kurze Impulse werden erfasst; niedrige AC-Werte, große Sprünge und Abweichungen sind Verdachtsfälle, kein automatisches musikalisches FAIL/PASS. Sehr langsame/leise Presets brauchen gegebenenfalls längere Nachprüfung; hohe Frequenzen/Drums können echte große Sprünge haben. Keine polyphone, Tonhöhen- oder klangidentische Freigabe durch diese Messung.
- OwnerPresetMetrics.hpp und OwnerPresetMetricsTest.cpp (Blobs 411f5d507c0582b05340545bbdeb3d9a7bf11b87 / 07e9084f6efa71c91401635939bd25a7e44fd733) mit g++ C++17 -O2 -Wall -Wextra -Werror -DNDEBUG sowie UBSan tatsächlich ausgeführt. Positive Prüfsignale: DC mit RMS .01 und AC nahe 0; Sinus AC .065–.075 und keine großen Sprünge; Einzelimpuls zwei große Sprünge; Blockgrenzenimpuls zwei Sprünge; fehlerhafte Eingaben/NaN abgelehnt. Vier einzeln kompilierte Testmutanten erkannt: DC als Ton (Z.16), Blockgrenzen ignoriert (Z.22), doppelte Probe zugelassen (Z.12), NaN ignoriert (Z.23). Eine erste Mutation hatte nur Compilewarnung; korrigiert und als tatsächlich fehlgeschlagene Laufzeitmutation erneut ausgeführt.
- Private WindowsOwnerPresetSweep.py (Blob 726816614035964b13a169eaa43e70c1ab02b875) speichert je Bank private JSON-Messwerte und SHA256-Identitäten der Binärdateien, optional gleicher Test gegen getrenntes Vorgängerbundle. Strikte vollständige Ergebniszuordnung, keine fehlenden/doppelten/fremden Proben, keine ungültigen Messwerte. 6 lokale Python-Unittest-Methoden ausgeführt: 25 randomisierte Rechteckfälle mit vollständigem Raster, Split/Layer-Negativkontrollen (alte einzelne Note und entfernte Überlappungszone), unzulässige Tabellen/Range-Werte, widersprüchliche numerische Reports, fehlende/mehrfache Reports; beide stummen oder kontaminierte Referenzen ausdrücklich nicht verifiziert.
- Workflow nimmt die portablen Tests und die beiden privaten Runner-Skripte ins assetfreie Testpaket auf. Native Windows-Hoständerung noch nicht kompiliert oder ausgeführt (kein Wine/nativer Windows-Host in diesem Lauf). Neuer Code ist ausschließlich Test-/Inventurcode, keine Produkt-/GUI-/Engine-Änderung; kein kostenpflichtiger Build. Windows-Screening, originaler Creative-Referenzvergleich, PCM-Fehlerbehebung, tatsächliche Hörproben und Bankwechsel-/Performance-/Regler-Abnahme bleiben offen. Frühere PASS-Meldungen sind kein Beleg dafür.

Private Ausführung nach Erstellung des konsolidierten nativen Testpakets: `python tests/WindowsOwnerPresetSweep.py --banks <entpackter SF2-Ordner> --bundle <125A AWE64 Gold.vst3> --exe <awe64_vst3_host_pcm_smoke.exe> --output <neuer privater Ergebnisordner>`. Optionale `--bank` / `--program` für gezielte Wiederholung, `--mode probes` für Zonen, `--reference-bundle` für exakten alten VST3-Stand. Kein stillschweigender Gesamt-PASS; `SCREENING_MEASURED_NOT_SOUND_APPROVAL` bedeutet nur, dass alle verlangten Messzeilen vorhanden sind. Bekannte stumme Synth-512-Presets und Bass-Synth-Sprünge zuerst prüfen. Keine proprietären Dateien oder privaten Messergebnisse ins öffentliche CI.

## 28.11.2026 — Compiled candidate: all-bank staging and targeted original-asset PCM (PARTIAL)

Actual ce4e300 Windows binaries under Wine 10.0, artifact 15259027279. Sf2LegacyStageDump.exe SHA256 9af8c0f97ef6d9e9428d64a52fdfe1a8d312bda2966a47bcc981e0ee30d9e66b. All 22 source files passed actual staging (all exit codes zero; original SHA256 before/after unchanged); 20 adapted, two byteidentical. Independent parser comparison of all original and staged files: 955 preset (bank/program/name) identities, reachable zone counts and both screening/extended probe selections unchanged; all 22 sdta/smpl chunks byteidentical. Original files untouched, no audio or source uploads. Local stage-results.json SHA256 5a8564208ead982bbc3160610de6913b82bc594a293c435b21179aaeade3b62a. These checks do not prove complete generator equivalence, loading in Creative, or individual layer audio.

Actual compiled VST3 host targeted note 60 / velocity 100, 133120 hold frames and 44032 release frames for five known cases. Three previously silent Synth-512 programs now produce dynamic PCM; both Bass-Synth cases have zero jumps above .15. One large step in Synth-512 program 12 remains a review case; large steps may be authentic attacks. Numeric report parser verified all five rows and complete sweep markers; no DOS boot error observed. The Linux wrapper did not separately retain exit status for these two completed sweeps (terminal markers present). No complete native original reference waveform comparison or full owner sweep. 5 of 2778 planned bank-0 screening probes covered; 2773 remain; 29 non-bank-0 addresses still unsupported. Logs private (SHA256 synth 8c4288d19746df53b76c35888040adbc91ad40f78b4e9b0871aeb0a0c8071d69, bass 3f5a6e39d245346af3411704d4d5073c877819b3c6f03deafa2b47672493d239). No new build. Keep all aggregate results separate from native Windows and Studio Pro release status.

## 28.11.2026 — Actual compiled VST3: all overloaded Synth 512 programs sound (PARTIAL)

Candidate ce4e300, artifact 15259027279, Wine 10.0. Identity from prior manifest checked; original Synth 512 SHA256 d5b99ec38fddf59f53f66b90921e170d207bf3bbd73960f6e4175c39ef68349b unchanged. The real host executed 15 Note60/velocity100 probes; all report numeric evidence except program 32 (CONTAMINATED after program 21, not played). Isolated fresh instance for program 32 then produced AC-RMS .122111, peak .295593, max step .105682, 0 jumps. All three completed runs retain process exit 0, fresh boot log, no ERROR, full report parser coverage. 133120 held and 44032 release frames for each measured note.

| Program | Max block-centered AC-RMS | Sample steps >.15 |
|---:|---:|---:|
|8|.0933198|0|
|12|.0539212|1|
|21|.0783117|0|
|32 (isolated)|.122111|0|
|34|.0559609|118|
|35|.0504106|99|
|45|.137364|0|
|47|.0749958|168|
|50|.079741|0|
|52|.0554139|0|
|55|.114703|0|
|57|.0822107|6|
|70|.141453|2|
|74|.134488|0|
|77|.054952|1|

Program 12 (Electric Slap Bass) separate 3-probe recheck: (key/velocity, AC-RMS, max step, jumps): 48/65 .0247196/.0212708/0, 60/100 .0558717/.058197/0, 72/115 .0658443/.0698242/0. Previous single jump reproduced once in first batch, not in this three-probe run. Active original sample dm2_a1 has raw adjacent-sample max difference 1.960449 full-scale units and W8_1 max difference .108826, so a large output step is not sufficient evidence of a playback bug. No automatic waveform smoothing. Six remaining large-step programs (34,35,47,57,70,77) require reference/periodicity checks. One initial second-Wine-prefix launch failed before loading kernel32 (exit 53, no PCM); retry used working prefix and distinct bundle copy, succeeded. No performance comparison from concurrent tests.

Private report SHA256: overloaded-15.json 3fe66cc360560eabe35ed5a97c601dddd9b85a91a628b2e70bee0523420e8359, slap-3-retry.json 4e33f30f0465b6cfbbfb8f93f2760057bb71f8e887b323313ad996b089ac14fb, p32-alone.json 4ecf884552f371f3251c21ac9cccf7dfe4ad58ccec2715fe55875ee40fecb4e3. Combined distinct planned screening coverage now 19/2778, not 19 final sound approvals. Original 29 non-bank-0 addresses still unsupported.

Additional read-only comparison over all original/staged files: independent Python zone extraction compared the sum of all 15 additive relocation candidates and all other effective generator values, ignoring implicit zero loop offsets. 952/955 preset lists match; differences only Synth 512 program 10 (sample 18), 11 (17), 68 (10), previously invalid loop-offset metadata reset to zero. Sample headers changed only for Synth 512 samples 14–18 and Vintage Drums 2001 samples 80,81,128,129, correcting loop points to sample bounds. All referring zones (Synth 512 programs 10,11,18,44,53,68; Vintage Drums 2001 programs 3,10,11 in banks 0/128) have sampleModes=0 before/after. Both pmod and imod chunks remain byteidentical in all 22 banks; per-zone modulator routing not independently verified. Sample data unchanged. This check is not a full acoustic equivalence proof; preserve original files. No product changes and no additional CI builds.

## 28.11.2026 — Repeated targeted audio comparisons and six full first-pass bank screenings (PARTIAL)

Actual ce4e300 candidate, Wine 10.0, private artifact 15259027279, unchanged VST3 SHA256 520f4410252ae420d9cc177bc62c2d40ed628b35ae99d461e5921fcdf003360d and host fb91bca8ccc71f867e568e4be490660dd680545193c12745960a220bd8ef1527. Comparisons use isolated private copies of the *same* current shell/host; only Engine.dll changes, so do not claim a whole-product revision comparison. Reference Engine 972e862 SHA256 4167cbb8ff8ee7cd344574316589466e76d3927f5f7dc1abcd6da6b3abd1c1c9 already includes the filter-bypass correction (verified patcher). Additional pre-fix Engine 65c419e SHA256 ffba0174a546d873570da02a4242d074db145d7457fb233ab08c075697de802f downloaded from existing artifact 15231538925, identity verified against its BUILD-IDENTITY.json, source patcher lacks the correction. No new compilation. All listed reports are actual completed runs, exit 0, fresh boot logfile without ERROR, strict numeric parser coverage, input hashes unchanged. Concurrent isolated host executions are not a timing benchmark.

| Synth 512 program (60/100) | Old Engine RMS | New Engine RMS | Old steps >.15 | New steps >.15 |
|---:|---:|---:|---:|---:|
|34|.0155073|.0152933|119|107|
|35|.0106264|.0106268|91|101|
|47|.0603732|.0604376|147|154|
|57|.00905255|.00903948|6|4|
|70|.0209347|.0209351|0|0|
|77|.0438294|.0437989|0|0|

Every row has nonzero AC, relative mean RMS change below 1.4%, no newly introduced large-step category; counts/peaks vary, waveform identity is not proven. Independent source zone inspection: programs 34,35,47,57 use Saw_a1 with original adjacent-sample max difference 1.99506 full-scale units, program 70 W12_1 .70233, program 77 FM5-0440 .55219. These raw samples contain intrinsic sharp transitions; do not flatten them. No claim that all steps are intentional or hardware-faithful, no PCM equality. Private report SHA256 six-current b359f0ed3b082433159b57cd834b1fe71958c56a89812435acfa71fbf008b5db; six-oldengine 2fa00f679b32bfe9167551eed65b0c3f59cac159857c27c868a8e26bbcbf4335.

| Bass Synth (60/100) | Pre-fix RMS | Corrected RMS | Pre-fix max step / count | Corrected max step / count |
|---:|---:|---:|---:|---:|
|0|.0354408|.0354462|.328613 / 24|.0119629 / 0|
|15|.0426357|.0425256|.0842285 / 0|.0751648 / 0|

Pre-fix report SHA256 209d6add03351bcf9e25aa7b1d1c63dc2a73933766fe563ff611834f6c4897b3; corrected measurements from previously recorded bass.log SHA256 3f5a6e39d245346af3411704d4d5073c877819b3c6f03deafa2b47672493d239. The pre-fix positive fault reproduces the large-step defect while the corrected version preserves mean energy; no claim of bitwise identity or exact periodicity measured by this VST3 host. Program 15 not a positive-fault case at this pitch/velocity.

| Actual owner bank, resident opt-in | Presets | All planned screening probes | Signal rows | Step reviews |
|---|---:|---:|---:|---:|
|Elisse Moans|1|3|3|0|
|PIANO|1|3|3|1|
|Boom Bass|1|3|3|0|
|Classhit|1|3|3|0|
|Perco Bass|1|3|3|0|
|JR_pad_voice|2|6|6|0|

All six actual resident runs have exactly one AWE64_RESIDENT_BANK_COMMIT and source SHA unchanged, valid complete reports, exit 0. No contamination/silence. Private report digests: Elisse 39bff5d4e149e44a18f7eb619dd9e337f91b4654097ca155c85e58e62709aad0; PIANO 20666201a44817c175ebe2ea66cb08e365f2d96574d9939db67a81701f98d6a0; Boom Bass 6d4034acec67f3f794958a27292fcc34dee04d7fefbe01ad9cab487f3171a9ac; Classhit b4f0138feb1e2d4a87bc8b07c70fe8d797f3e09222d8918042261203358a79c6; Perco Bass 48cca806ad3d6c303e675ca48e70695ff1cf7598da047e850d5b5a97e913532e; JR_pad_voice 313a7bf090b9018e34973f231597b8dfecace18eee207a6f66f0cee12d316046. Elisse actual source 23433218 bytes, SHA256 b37c3241d3db090402cad7b470ab6a5c8a0f8380c0f9d20aa678f8e364ffcb78; AC .00840696/.0282292/.027137; max steps .00656128/.0298462/.02005. Real bank resident transition and playback observed, timing not instrumented for this path.

PIANO note72/velocity115 first current run 842 jumps, max .272369, RMS .0499819. Isolated repeat with 972e862 and ce4e300 Engines (both resident mode, one actual commit each): 852 jumps each, max .288361/.288422, RMS .0499898/.0499973. At 48/65 and 60/100 no jumps in either; source stereo piano(L/R) raw sample max adjacent difference .34622/.34561, 3525/3112 source jumps over .15. Repeat reports SHA256 old 048a1f8cec15452c1eb99b722b9071f2b6c49bf3d01587406b8358c6363038a7 and new 7844f4511b11a04237f9c6e1b7ac5076e569de45952df045061589f6a71a1a77. This does not establish hardware fidelity but no evidence of a recent regression in this case; no smoothing or source edits.

Cumulative 40/2778 distinct screening probes of bank-zero owner inventory (previous 19 + 21; repeated probes/reference runs not double-counted). 2738 remaining plus extended zone testing. Six banks complete first-pass plan only. No native Windows/DAW/subjective sound acceptance, no full audit.

## 28.11.2026 — Continued private owner screening, exact cumulative coverage (PARTIAL)

Actual ce4e300 Windows candidate under Wine 10.0 (same 15259027279 artifact, engine/VST3/host SHA256 identities unchanged). Original SoundFonts read-only, exact source SHA verified before/after. All eight new candidate/reference reports below have actual process exit 0, fresh boot logfile without ERROR, strict numeric/complete report parser coverage. Resident opt-in on each. Parallel isolated Wine instances are not a performance benchmark. A first three-bank parallel invocation was interrupted by the tool execution limit after partial PCM rows; none of those aborted runs counted. Fresh completed subset runs were used instead.

| Bank | Scope | Signal | Step-review rows |
|---|---|---:|---:|
| Choral Aahs | all 5 bank-0 presets × 3 probes | 15/15 | 0 |
| Vintage Dreams Drums | all 8 bank-0 presets × 3 probes | 24/24 | 4 |
| Callanish | one valid representative probe per 23 bank-0 presets | 23/23 | 10 |
| Magnus Choir v1.6 | one valid representative probe per 13 bank-0 presets | 13/13 | 0 |
| Bass Synth | P0/P15 note60/velocity100 repeat | 2/2 | 0 |

Bass repeat P0 RMS .0354421, maximum step .0120544, P15 RMS .0424374, maximum step .0803833; no steps >.15, source unchanged. Previously measured two probes without retained exit status now have a completed independent report. Candidate report SHA256: choral-one 823b3bf3e60ad5a240a5ea72279a133c5f2faec8b41b04f3796884771be87a55; choral-rest 673d031cb3ad1cafeb9ecaca0b1cc453b8a321705ed532630795b618c926d54e; vddrums-one 393831c8fd3259d00c9bae124d881efd408633131bbd6becf0463da901a4e05d; vddrums-rest 2709869658a6a313bbf69a7dc67d2970372edc18bdbb13d5c6a810dd4a5cd2cf; callanish-one 549a3bbbc5df716f97e98f4dc4f384ed5e5f631d84ce42eb9b37980efae47695; magnus-one 2ebe12959e75feda9799e5a88e77debf00ab55f13ec82b4d0859df3f0a0282ab; bass-repeat 91988c199e568276c56ffc1d4eafb9b587b3bffa77d2a3bc455dc1a2970c174a.

Callanish worst four step reviews at note 60 / velocity 100, current shell/host with prior 972e862 Engine versus candidate. Both runs completed; prior engine includes filter fix. Source active sample step count is read-only raw PCM, not output count.

| Program | Old RMS | New RMS | Old steps >.15 | New steps >.15 | Source sample / steps >.15 |
|---:|---:|---:|---:|---:|---|
|5 Cymbals|.0349697|.0349698|2703|2701|crash01 / 8860|
|3 Hihats|.012425|.0124311|252|254|hihat28 / 3070|
|2 Snares 2|.00902704|.0090246|107|101|snare83 / 455|
|7 Percussion|.00475519|.00475555|84|81|shala02 / 311|

Old report SHA256 d947b6c9467469b81f17d9679adb51fb4969f80a1e4cf809c3c4a90b594c1a82. RMS deltas <0.05%, output step counts close. Six other Callanish step-review programs (1,13,14,15,18,22) not yet directly compared. Four Vintage Dreams Drums step-review triplets: 3/50/115 (2 steps), 4/72/115 (35), 5/47/65 (19), 7/70/115 (6). Active raw source samples include 909BD11 (2), SINE32 (1) + NOISE5 (2702), NOISE3 (7809) + FM90000 (242), white808-2 (3142) respectively. No smoothing or source modification; old-engine comparison for these four still open. Neither raw steps nor close RMS prove audible/hardware equivalence.

Private coverage helper tests/OwnerPresetCoverage.py (blob 0fc11c7d7a1582f5702b04d6b1b22a386a136795) consumes completed local wrapper JSON and corresponding same-stem raw host log. It is not a reader for the original WindowsOwnerPresetSweep.py per-bank JSON schema. Requires exact three binary hashes from manifest, original bank SHA, integer exit 0, boot status, raw log digest, valid planned bank/program/key/velocity rows and re-parses actual raw report; checks exact stored rows. Deduplicates repeated probe keys, counts no contaminated rows as measured, retains all previous review statuses and produces remaining triplets by bank. Local test (blob 0f76d92e31d5185e4a3a24b64001f62a979909e0) executed 1 unittest method with 11 negative cases (bad exit or bool exit, missing boot, boot ERROR, source/identity/log mismatch, rows mismatch, unplanned probe, modified log, missing completion despite updated hash), duplicate-report positive and contamination/retest behavior. Actual aggregate of 18 completed candidate reports: 115/2778 distinct measured and signal-observed probe keys, 73 distinct presets with signal, 8 complete bank-0 first-pass screenings, 23 keys with historic review flags (including earlier contamination and subsequently clean repeats). No rejected reports in this selected completed-candidate set. Reference-engine and interrupted reports deliberately excluded. Private coverage JSON SHA256 a3157915a12ca1e1f18cae4bb7c5386e1fbf6d5008c3a51fc0cc798bdb6d50f9. Coverage is not sound approval. 2663 screening probes remain. All private logs/reports/source samples remain local. No product changes, no CI.
