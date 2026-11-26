# Adversarialer Nachaudit — 26.11.2026
## Urteil: NO-GO / PARTIAL AUDIT — keine FULL-AUDIT- oder Release-Freigabe

Produkt `0ff08116701efd831b9fca00970f8b059ae9b9af`, Tree `d4bab55d67be76643230d48d41a1abea2e2bd947`, Version 0.0.0-dev. Engineering `53b10bd4a81fe6c5d62e110b932aef7155172ad6`. Originaler DOSBox-X-Gitlink `f3a95b9f19a7f9863295e6481f271cf04733c635`. Auf main wurden ausschließlich neue Audit-Workflows, Testhosts und Dokumentation ergänzt. Der Produktcode, Originalchip, Creative-Integration und GUI bleiben auf der eingefrorenen Revision unverändert.

START-HERE und AUDIT-COMPLETENESS-CONTRACT vollständig erneut gelesen; die vorherigen Lektürebelege für unveränderte Engineering-Pflichtdateien wurden gegen die aktuellen Blobs geprüft. Dieselbe Agenteninstanz prüft hier eigene vorherige Arbeit kritisch nach (Self-Challenge, kein unabhängiger Zweitreviewer). Unabhängige externe Testsoftware: 125A Plugin Test Lab, exakte Revision unten. Kein neuer Claim einer vollständigen unabhängigen Quelllektüre.

## 1. Echte neue Produkt-Gegenproben

[Windows-Audit 45736699729, Job 139631557123](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45736699729/job/139631557123), Testcommit c2f22d8ac5e27dc09dacb1e5ab666920d91222cb. Unveränderter Produkt-VST3 und gepinnte original-DOSBox-X-DLL erfolgreich auf Windows-2022 gebaut. Tests verwenden ausschließlich synthetische Assets. Die native Realtime-Positivkontrolle besteht.

| ID | Unabhängiges Soll und Gegenprobe | Tatsächliches Ergebnis | Urteil |
|---|---|---|---|
| A07-O | Default-Instrument muss Offline-Bounce mit Audio liefern. Identisches synthetisches DOSBox/PC-Speaker-MIDI-Signal im funktionierenden Realtime-Test, danach kOffline mit 1000 Blöcken à 512 Samples ohne künstliche Sleeps. Kein experimentelles Umgebungsflag. | 512000 Frames, nonzero_frames=0, peak=0, elapsed_ms=11, process_errors=0. Vollständige Stille trotz API-Erfolg. | FAIL, kritisch für Export/Bounce |
| A06-I | Vor Aktivierung akzeptierte synthetische SF2 mit Programm 0. Datei am gleichen Pfad durch separat valide Bank mit Programm 17 ersetzt. Akzeptierte Identität erhalten oder explizit ablehnen, nicht still ein anderes Programm auswählen. | activate=0 (Erfolg), program=17, state_same=0. | FAIL, hoch; Projekt-/Bankintegrität |
| A01-O | Zwei MIDI-Ereignisse im selben Block: zuerst CC7 bei Offset 0, Note bei 256 (Positivkontrolle). Danach Note bei 0, CC7 bei 511. Exakter Byte-Strom an synthetische Recorder-DLL über tatsächliche VST3/Runtime-ABI. | vNVN statt vNNV: die spätere Lautstärkeänderung wird vor der früheren Note zugestellt. | FAIL, hoch; Reihenfolge/Automation |
| A26 | Tatsächlicher C++-SF2-Preflight: gültige Bank und ungültiger Loop als Kontrollen; malformed Stereo-Sample mit Selbstverweis. | self_linked_stereo.sf2 expected=0 accepted=1 adapted=0 rollback=1. | FAIL, Parser-Vertragsfehler; kein nachgewiesener Gastcrash |

Root causes: `src/DosBoxVst3Shell.cpp` setupProcessing/process: Demand-Clock im Standardpfad nicht aktiviert, asynchrone PCM-Uhr statt verbindlicher Host-Zeit; CC-Queues vor Note-Queues. setActive/startRuntimeLocked/requestSoundFont: akzeptierte Bytes werden bei erneuter Aktivierung nicht als verbindlicher Snapshot benutzt, publishCatalogLocked wählt Ersatzprogramm. `src/Sf2StructuralValidation.hpp`: Linkbereich geprüft, nicht Reziprozität, Zieltyp und Selbstverweis.

## 2. Testqualität — tatsächlich ausgeführte Gegenproben

[Portable Audit 45736816238, Job 139631924485](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45736816238/job/139631924485). Tests/adversarial-current/probes.py benutzt die echten vorhandenen Python-Orakel, die echte Validator-Regex und den kompilierten Produkt-SF2-Parser. Alle sechs nachstehenden falschen Testeingaben werden tatsächlich akzeptiert. Gültige Kontrollfälle und explizite Fehlerkontrollen (Loop, Queue-Drop, Deadline außerhalb Zeitfenster, Validator-Fehler) bestätigen, dass die Proben nicht bloß durch kaputte Fixtures fehlschlagen. Lokal mit exakt verifiziertem Archiv wiederholt.

| ID | Fehlerhafte Messdaten, die das Gate passieren |
|---|---|
| A16 | 128 stumme Referenzpresets UND 128 stumme Pluginpresets |
| A23 | DC-Plateau statt 441-Hz-Note |
| A23 | sechs identische Eventnachweise (0,0) |
| A15 | 99 Deadline-Überschreitungen |
| A15 | fehlende numerische Rate-, p95-, CPU-, Deadline- und Sustain-Evidenz |
| A20 | Validator-Text 1 bestanden / 0 fehlgeschlagen anstelle von 47 / 0 |

Das sind sechs nachgewiesene Orakelfehler, keine sechs nachgewiesenen hörbaren Produktdefekte. Der tatsächliche ursprüngliche Windows-Lauf meldete 47/47 und die dort tatsächlich gemessenen Werte bleiben gültig.

Weitere wiederholte isolierte Gegenproben auf Grundlage unveränderter Quellfragmente: A13 (C++-Modell tatsächlicher lo/hi-Atomic-Sequenz, erzwungene legale Interleaving-Reihenfolge): 255 255 255 255 255 255 255 255 0 0 statt einer kohärenten Generation. A24/A25 (isolierte C++-Messlogik, exakt unveränderter Test-Blob ffb6e928f923a3d79005f529c77ace7265483980): hängende Note, rechter Kanal null, dominanter falscher 10-kHz-Anteil passieren RAM-PCM-Orakel; B->A-Leakage passiert Dual-RMS-Gate. Reine Stille und alleiniger falscher 735-Hz-Ton werden im RAM-Orakel abgelehnt. Keine Behauptung, dass diese akustischen Mutationen im realen EMU8000 beobachtet wurden.

Erster portabler Job 139631557468: Infrastrukturfehler (HEAD^ bei shallow checkout nicht verfügbar), kein Produktbefund. Separater portabler Lauf 45736816238 auf exakt gepinntem Auditquellbaum korrigiert diesen Fehler, ohne erneut Windows-Originalsuite auszulösen.

## 3. Gültige positive Resultate und deren Grenzen
- [Original-Gesamtlauf 45731174304](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174304): 47/47 echter Steinberg-Validator; 128/128 Cheezoids-Hüllkurvenvergleich; Original-Creative-Archivklang; p95 bei drei geprüften Raten 18.9569 / 18.6042 / 18.625 ms, gemessen null Drops/Underruns. Originalklang im geprüften Umfang PASS. Keine Besitzerhardware-/DAW-Abnahme.
- [45731174350](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174350): acht native Runtime-Fälle PASS, einschließlich drei Kaltstarts, asynchroner Bankfehler-Rollback, aktuelle Bus-Schnittstelle. [45731174401](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174401): deterministische native ABI-Gates PASS. [45731174280](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174280): SF2-State PASS in bisheriger Matrix. [45731174284](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174284): portable Tests PASS. [45731174298](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45731174298): Version PASS.
- Lokaler erneuter Build aus source.tar.gz des Audit-Artefakts 15078609236 (136 Blobs anhand Git-Blob-SHA1 geprüft, 0 Abweichungen; sieben zusätzliche Audit-Blobs, 129 Produkt-Blobs). Release 22/22 CTest, Python unittest 44/44, separate Paket-Unit-Tests 2/2. Clang Debug mit AddressSanitizer + UndefinedBehaviorSanitizer 22/22; keine Meldung in diesen Tests. Das ist kein Windows- oder Race-Sanitizer-Nachweis.
- Aktuelle Linux-Paketgegenprobe: gültiges PRIVATE-Paket, direkte Außenpfade, Dotdot und Symlink-Eltern, rekursive Gitignore für ROM/Demo/SF2 PASS, failures=0; vorherige native Windows-Reparse-Gates der unveränderten Paketkorrektur ebenfalls PASS. Keine allgemeine Freigabe der Produktdistribution.

### Unabhängiger Test Lab: genaue Auswertung statt pauschaler Interpretation
[45734851189, Job 139625494800](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45734851189/job/139625494800), Artefakt 15077579270 tatsächlich heruntergeladen und Reports gelesen. P74 Test Lab `7213fe1f81f4036f74431cb5b2d7dbd09b73ee5c`. Produktmodul SHA256 18ee55db8fbc24dea4013d30d068680212f1e5feb59deda45f6e8918d14a3a68; Runtime SHA256 c9cfd8cff70cd9f08d234dae2b3428a5893ad6bc126e0fa6d0af3bafc0cb82b8. TECHNICAL_BASELINE_PASS ist korrekt innerhalb der deklarierten technischen Grenzen.
- P32: 44.1/48/96 kHz x 64/512, sechs Fälle, 15000 process-Callbacks, 3000 State-Saves, 0 Callback-/Save-Fehler, keine gemeldeten Overruns. Domain correctness NOT_COVERED, kein Audio-Orakel.
- P34: 352800 Stereo-Frames, 32 MIDI-Ereignisse, Renderpeak **0**, explizite Warnung vor stummer WAV. 11-Byte-Default-State. Zwei Recall-Prozesse ohne MIDI-Ereignis, beide stumm; musikalische Richtigkeit und State-Integrität NOT_COVERED. Kein Beleg für Audio-Recall externer SF2.
- ABI: 209 Parameter (17 reguläre, 192 versteckte Controllerparameter; 12 x 16), ein Stereo-Output und ein Event-Input mit 16 MIDI-Kanälen. Editor erkannt; GUI-Kandidaten 0, keine Klicks, Feature-Coverage NOT DECLARED. Kein Ersatz für GUI-/Browser-Matrix und musikalische Prüfung.

## 4. Konsolidierte Befundmatrix (vorherige A01–A25 auf aktuelle Revision bezogen)

W=nativ tatsächlich ausgeführt, P=portable Produktprobe, O=Orakel-Gegenprobe, S=Quellbefund, N=nicht nachgewiesen. PASS bezieht sich ausschließlich auf benannten Fall. Alte Defekte, deren Korrektur nachgetestet ist, werden nicht als unverändert behauptet.

| ID | Aktueller Status | Beleg / verbleibendes Risiko |
|---|---|---|
| A01 | FAIL W | MIDI-Reihenfolge im Standardpfad, siehe A01-O; Master-Gain-Interpolation separat portable PASS. Samplegenauigkeit aller übrigen Parameter nicht bestätigt. |
| A02 | Teil-PASS W/P | CC7-Rückholung und MIDI-Recovery-Gates bestehen; Reset/Overflow aller Controller/Kanäle/clocked path noch N. |
| A03 | Teil-PASS W/P | Bisher ungültige Loop- und Generatorgrenzen abgewiesen; Stereo-Link-Lücke A26 und ROM-Adressausnahmen noch offen. |
| A04 | Teil-PASS W | Pause/Resume-Overflow behoben und neuer Test bestanden; virtual guest läuft im Legacy-Pfad während Pause weiter, umfassende Transport-/Loop-Semantik N. |
| A05 | Teil-PASS W | Bounded shutdown und Quarantäne/Reaper bei injiziertem Stall; generelle Fatalisolation N (A22). |
| A06 | FAIL W | Same-path-Neuladen und asynchroner Rollback bestanden; erneute Aktivierung kann akzeptierte Bankidentität verlieren (A06-I). Kohärenter getState während Bank/Param-Commit und erfolgreicher Wechsel unter MIDI-Last N. |
| A07 | FAIL W/S | Standard-Offline-Export stumm. getLatencySamples meldet im Legacy-Pfad nur SRC statt nachgewiesener Gesamttransportverzögerung; native PDC-Abnahme N. |
| A08 | Teil-PASS W | CI-Performance und Deadline im Originaltest gültig; i9-Besitzerlast, reale ASIO und andere Blockgrößen unter tatsächlicher polyphoner Last N. |
| A09 | Teil-PASS W | Drei Kaltstarts, ungültige MIDI-/Buspfade in bisheriger Matrix; try_lock-Transition kann Eingaben verlieren, vollständige Sentinel-/NaN-/Race-Matrix N. |
| A10 | Teil-PASS W | ROM-Preflight und Originalarchiv PASS, normaler Produktpfad akzeptiert abweichend große DEMO.EXE ohne Originalhash-Prüfung; Besitzerassets, Default-SF2-Identität und Load-Failure N. |
| A11 | NOT VERIFIED S/N | 12 CC-Spezifikationen x16 vorhanden, keine pauschale GM/RPN-/Bank-/Program-Channel-Vollabnahme; Bank0 begrenzt. |
| A12 | OFFEN S/P/N | GUI ChangeParameter beachtet Host-Ablehnungen nicht vollständig; früherer Mock-Fehler, aktuelle native Ablehnung nicht ausgeführt. P165 Editor/MOD RESET W PASS. |
| A13 | FAIL P/S | Separat atomare Voice-Overlay-Hälften, reproduziertes gemischtes Snapshot-Modell. Nativer Register-/PCM-Race noch N. |
| A14 | NOT VERIFIED N | Vollständiger Upstream-/SDK-/SDL-/PE-Link-/Lizenzabschluss nicht vorhanden. |
| A15 | FAIL O | 99 Overruns und fehlende numerische Evidenz akzeptiert. |
| A16 | FAIL O | Beidseitige Stille im 128er Vergleich akzeptiert; bisheriger Originaltest bleibt benannter Positivnachweis. |
| A17 | FAIL S | CI-Pfadfilter erfassen Gitlink/Abhängigkeiten nicht lückenlos. |
| A18 | PASS P/W begrenzt | Kanonischer Privatpfad, dotdot, Symlink/Windows Reparse früher und aktuell geprüft. Kein genereller Dateisystem-Angriffsschutz. |
| A19 | OFFEN S | Build-And-Test-Private-AWE64.ps1 prüft Original-HEAD und Marker AWE64_HostRegisterPCM, kann alte lokale Patch-Arbeitskopie wiederverwenden; kein Binärnachweis für Besitzerbuild. |
| A20 | FAIL O | 1/0 statt 47/0 akzeptiert; aktuelle tatsächliche 47/47 bleiben gültig. |
| A21 | PASS P/W begrenzt | Rekursiver Gitignore für 3 private Dateien, positive/negative Tests. Keine privaten Dateien im Audit verwendet. |
| A22 | CRITICAL S/N | Upstream support.cpp E_Exit zeigt MessageBox, SDL_Quit, exit(1); SDL-Init-/EMU8000-Allokations-/Mixerpfade. ROM-Fatalweg vor Eintritt abgefangen, verbleibende Fatalpfade nicht allgemein isoliert. Kein aktueller Windows-DAW-Absturz ausgelöst oder behauptet. |
| A23 | FAIL O | DC und doppelte Eventnachweise passieren Test. |
| A24 | FAIL O | Stuck Note, fehlender rechter Kanal, falscher dominanter Ton passieren isolierte PCM-Logik. |
| A25 | FAIL O | B->A-Leakage passiert Dual-RMS-Logik; reziproke native Isolation N. |
| A26 | FAIL P | Ungültiger Stereo-Sample-Link akzeptiert. |

## 5. Scope, sichtbare Funktionen, Abnahmegrenzen
Exakter Produktbaum: 129 eigene Blobs, 1 Gitlink. Davon src 21, integration 4, guest 2, research 2, gui 11, tests 57, tools 11, workflows 10, docs 3, root 8. Inventar verifiziert; nicht sämtliche Blobs und transitive Upstream-Dateien unabhängig vollständig geprüft. 17 reguläre Parameter: 1000/1002 Programm/Bank, 1001 Master, 1100–1111 ADSR/Filter/FX-Sends, 1200/1201 Effektwahl. 192 verborgene Parameter für Controller. GUI-Funktionsgruppen: PREV/NEXT, BROWSER/SYNTH, Suchtext/Bankfilter, ROM/SF2, aktive Bank/Programm, Tabellenzeile, MOD RESET, Master, 12 Regler, zwei Effektmenüs. Bestehende native Editor- und Reset-Gates bestätigen nur einzelne dieser Funktionen. Lücken: alle realen Maus-/Tastatur-/Host-Gesten mit PCM und Recall, DPI, Resize, mehrfacher Editor; keine vollständige UI-Matrix.
- Produktquellen und zentrale Implementierungspfade mit statischer/gezielter dynamischer Prüfung; ausstehend: unabhängige vollständige aktuelle Lektüre sämtlicher produktiver Blobs, tatsächliche generierte GUI-/Upstream-Dateien, kompletter transitive Build/Link-Abschluss, proprietäre Originalsoftware und Lizenzprüfung. Git-Archiv enthält Gitlink, nicht den vollständigen Upstream.
- Windows-CI synthetische und genehmigte Archivfixtures, lokale Linux-Proben; kein eigener Windows/MSVC-Host lokal. Physische ASIO, tatsächliche Besitzer-SF2, Studio Pro, vollständiger 125A Plugin Tester, unabhängiger Zweitreviewer nicht ausgeführt. Test Lab als unabhängiges Host-Werkzeug ist nicht mit Plugin Tester oder zweitem Audit gleichzusetzen.
- Keine Freigabe des proprietären Bundles und keine Besitzerdateien hochgeladen. Keine Änderungen an produktiven Dateien.

## 6. Priorisierte Abnahmesperren
1. A22 Hostprozess-Schutz und Fehlerpfad-Nachweise; A06-I/A26 Bankidentität und sichere SF2-Prüfung. Keine Architektur- oder Klangänderung ohne Notwendigkeit.
2. A01-O/A07-O Standard-MIDI-Zeitmodell und Offline-Render, PDC, erfolgreiche Übergänge und Recovery. Der bereits existierende opt-in Clock-Pfad ist eine mögliche Grundlage, aber kein bereits produktiv nachgewiesener Fix.
3. A13 kohärente Regler-Snapshots, A12 Host-Ablehnung, A15/16/17/19/20/23/24/25 Orakel und Build-Provenienz fail-closed. Keine bestehende Testschwelle zur Erzielung eines PASS lockern.
4. Vollständige übrige Funktionsmatrix nativ, Quellen-/Abhängigkeitsabschluss, 125A Plugin Tester und unabhängiger Nachaudit; Originalklangregression und vereinbarte Besitzer-Studio-Pro-Abnahme zuletzt.

**Schluss:** Drei echte native neue Vertragsfehler und ein portabler Parserfehler sind nachgewiesen; sechs direkte Orakel-Gegenproben plus weitere isolierte Modelle zeigen, warum grüne Workflows nicht genügen. Der frühere Originalklangnachweis wird dadurch nicht widerrufen. Die festgelegte Auditvollständigkeit ist wegen der konkret benannten Nachweislücken nicht erreicht. NO-GO für FINAL/Studio-Pro-Abnahme. Keine Produktkorrektur im Audit.

## 26.11.2026 — Korrekturpaket 1: Nachteststatus

Produktänderungen 8368fd9; Workflow-Korrektur 49de443 (identische Produktionsblobs). [Portable Nachprobe 45738889998](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738889998) SUCCESS, alle sieben bisherigen Gegenproben abgewiesen; [ABI 45738813528](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738813528) SUCCESS. Der erste Gesamtlauf 45738811961 war ein YAML-Infrastrukturfehler durch fehlerhafte Regex-Ersetzung; die unveränderte Produktrevision wird jetzt in [45738890137](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45738890137) geprüft. Die erste Unit-CI 45738813520 scheiterte am alten synthetischen Python-Kindprozess ohne neuen Richtungsmarker; dessen positive Kontrolle ergänzt, zusätzlicher Negativtest fordert beide Richtungen. Keine Schwächung des Gates. Native Produkt-Nachweise noch offen. Kein erneuter Original-DLL-Build allein für diese Fixturekorrektur erforderlich. 
