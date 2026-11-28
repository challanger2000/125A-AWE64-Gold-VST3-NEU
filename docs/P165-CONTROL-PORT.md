# P165/P175-Reglerport — verbindliche Arbeitsgrundlage

Stand 24.11.2026. Status: Quellabgleich, noch keine Implementierung oder Abnahme in DOSBox-X.

## Bereits vorhandene Arbeit

Altprojekt `125A-AWE64-Gold-VST3`, Commit `1f9051fae83f3347d96663df0b77ace58e9e13ca`, Datei `research/native-awe32emu/p009-vst3/apply_p165_original_synth_overlays.py`. Dort sind die zwölf Parameter-Overlays (acht Hüllkurven, Cutoff, Resonance, zwei Sends) implementiert. Der damalige Code arbeitet auf den fertig erzeugten VoiceParams vor StartLayers, nicht auf willkürlichen I/O-Schreibzugriffen. Als Testvorlagen dienen `P010NativeHostQA.cpp` und `P165NativeEditorHostQA.py`. Die damalige native Engine wird NICHT übernommen. Der alte Auditbericht vom 20.11. war noch keine Gesamtfreigabe; keine alten PASS-Zahlen als heutige Beweise verwenden.

## Abgleich mit gepinntem DOSBox-X

Quelle: `turican0/dosbox-x-AWE32@f3a95b9f19a7f9863295e6481f271cf04733c635`, `src/hardware/emu8000.cpp`, `src/hardware/emu8k/snd_emu8k.c` (emu8k_outw), `src/hardware/emu8k/include/86box/snd_emu8k.h`. Offsets sind relativ zur EMU-Basis 620h, nicht BLASTER-Basis 220h. Der bestehende absolute Send-Bridge-Pfad bleibt erhalten.

| Parameter | Port-Offset / cur_reg | Originaler Feldinhalt | Portierungsregel |
|---|---|---|---|
| AMP ATTACK | 400h/4, 402h/4 | ENVVOL, ATKHLDV | Delay verkürzen, Attack/Hold-Rate in Richtung 127; Triggerbit 15 und übrige Bits erhalten |
| AMP DECAY | 400h/5 | DCYSUSV low 7 | Rate nur bei Nicht-Release verändern, Engine-Bit 7 unverändert |
| AMP SUSTAIN | 400h/5 | DCYSUSV bits 8..14 | Sustain zum Nullwert reduzieren; Release-Schreibzugriff nicht als Sustain behandeln |
| AMP RELEASE | 400h/5 | DCYSUSV low 7, bit 15 | Nur originale Release-Rate in Richtung 127 interpolieren; Release- und Engine-Bits behalten |
| FILTER ATTACK | 400h/6, 402h/6 | ENVVAL, ATKHLD | Analog zur AMP-Gruppe |
| FILTER DECAY | 400h/7 | DCYSUS low 7 | Rate nur bei Nicht-Release |
| FILTER SUSTAIN | 400h/7 | DCYSUS bits 8..14 | Sustain reduzieren; Release-Schreibzugriffe unverändert lassen |
| FILTER RELEASE | 400h/7 | DCYSUS low 7, bit 15 | Nur Release-Rate verändern |
| CUTOFF | 800h/1 | IFATN high 8 | Original-Cutoff proportional schließen, Attenuation unverändert |
| RESONANCE | 402h/0 high word | CCCA bits 28..31 | Resonanz in Richtung 15; niemals Sample-Adresse/DMA-Bits 24..26 verändern |
| FILTER ADSR Route | 800h/2 | PEFE low signed byte | Originalroute unverändert, wenn vorhanden; sonst altes P175-Prinzip der begrenzten Tiefe mit Cutoff-Kompensation verwenden; Pitch-Byte unverändert |

Die Registerbits sind aus dem gepinnten Chipcode nachgewiesen. Insbesondere darf das alte replaceHigh nicht direkt eingesetzt werden, weil es das Trigger-/Release-Bit 15 löscht; replaceLow darf bei rohen DCYSUSV-Words nicht das Engine-Bit 7 löschen. Originales ENVVOL/ENVVAL mit Bit 15 gesetzt bedeutet bereits keine Verzögerung. Originale VoiceParams-Rate-0-Fallbacks nicht blind auf tatsächlich geschriebene DOS-Words übertragen. Vollständig neutrale Overlays (alle zehn Regler 0) dürfen kein einziges Registerwort verändern.

## Gebündelte Umsetzung und Nachweis

1. Tatsächliche Creative-NoteOn/Off-Registerfolge und MIDI-Lesegrenze belegen. Original `DOS/DEMO/DEMO.C` verarbeitet `ProcessMidi((BYTE) _inp(MPUPort(0)))` nach dem Statuspoll synchron; ruft original awe32NoteOn/Off/Controller auf. Eine bloße UART-Ready-Flagge belegt noch keinen sicheren NoteOn-Registerkontext. Boot, SoundFont-RAM-Upload, DMA, Effekt-/Hardwareinitialisierung und reservierte Voices 30/31 dürfen nicht verändert werden. Insbesondere darf eine Resonanzkorrektur nicht auf Adress-/DMA-Operationen wirken. Vor Eingriff genaue Reihenfolge von IFATN/PEFE und Voice-Start prüfen; keine doppelten Trigger oder Read-Modify-Write-Aufschaukelung.
2. Portierte gemeinsame Regelmathematik separat testen: 0 exakt identisch, 1 maximal, monotone Zwischenwerte, Bitmasken, alle 16-bit-Randfälle, bestehende positive/negative PEFE-Route, dunkle Cutoff-Basis, Release/Engine-Flags, DMA und nichtmusikalische Register. Testfälle der alten P175/P165-Version übernehmen, soweit der heutige DOS-Weg dies erlaubt.
3. Alle zehn Parameter und MOD RESET zusammen über VST3/Host-ABI/Originalregisterwriter anbinden. Atomare Übergabe an Guest-Thread, kein Heap/I/O im Audio-Callback. Kompatibler Projekt-State (alte Zustände lesen, neu gespeicherte zehn Werte exakt wiederherstellen), GUI erst mit echter Wirkung aktivieren. Reset setzt die Modifikationen neutral, ohne Master/Preset oder Originalbank zu verändern; bestehende P165-Reset-Semantik vorher abgleichen.
4. Ein gebündelter nativer Windows-Lauf: unveränderte Pflichtgates (47 SDK-Tests, Original-PCM, SF2, 128 Cheezoids, Timing, Stabilität, P165), zusätzlich pro Regler echte Register- und PCM-Gegenprobe einschließlich neutraler Referenz, Zwischenwert, Maximalwert, Recall/Automation und MOD RESET. Kein einzelner Windows-Gesamtbuild je Regler. Keine Bezeichnung als fertig, wenn die Regler lediglich Parameterwerte ändern.

Noch offen: sichere konkrete Note-/Voice-Schreibgrenze, alle zehn Implementierungen und neue Windows-Nachweise. Keine Veränderung an der EMU8000-Synthese oder Creative-DEMO-Binärdatei für diesen Quellabgleich.

## Lokaler Mathematik-Nachweis

`research/p165-control-port/VoiceOverlay.hpp` und `VoiceOverlayTest.cpp`: tatsächlich kompiliert und ausgeführt mit `c++ -std=c++17 -Wall -Wextra -Werror -O2 VoiceOverlayTest.cpp -o test && ./test`. PASS über alle 65.536 Registerwörter; keine produktive Anbindung. Test ohne `NDEBUG` ausführen. SHA-256 Header e1b7d59d5af8dcd43580cfc1960bd62dbd01df8994fdbdcaef231f2ad43a8dbb, Test 3a16c51ac6642a2546dcb2167a056e5cad1ed8965794249aa7175ebd44eed4b0. Die mathematische Funktion der Regler ist nur ein Teilnachweis; echter Note-/Release-Kontext und PCM fehlen weiterhin.

## Produktport (noch nicht abgenommen)

Alle zehn Parameter sind gemeinsam über den Host, ABI und ursprünglichen EMU8000-Registerwriter angebunden. Das FIFO-Tag wird erst beim letzten konsumierten Byte aktiv und beim nächsten Statuspoll beendet. Das ist eine aus dem synchronen Original-DEMO-Quellcode abgeleitete, noch zu verifizierende Schreibgrenze; kein Nachweis durch echte Trace-/PCM-Tests. Neue v5-State- und Altzustands-Recall-Gates. Originale MOD RESET-Funktion setzt zwölf Parameter auf 0. Windows-Klanggegenproben aller Regler, Automation, GUI-Reset und gehaltene/stimmbasierte Grenzfälle bleiben Pflicht vor Freigabe.

## Windows-Zwischenabnahme und neuer PCM-Gate

45494869093 SUCCESS: komplette bisherige Regression bei neutralen Reglern, echte v5-Parameter-Recall-Prüfung, 47/47 Validator. Noch keine Klangabnahme nichtneutraler Werte. Ein zusätzliches Windows-Host-Szenario spielt dieselbe selbst erzeugte SF2-Welle mit 23 NoteOn/Off-Paaren (drei neutrale Referenzen, zehnmal Mitte/Maximum), echte ParameterChanges. Per-Regler-PCM-Differenz und Neutralrestore Pflicht; keine Aussage über genaue musikalische Richtung oder isolierte Registerwirkung vor ergänzendem Registervergleich. Alle Trace-Dateien temporär, keine Originaldateien veröffentlicht.

## Erster nichtneutraler Klanglauf

45502431315: 7/10 PASS, Neutralrestore PASS. Filter Decay/Sustain/Release unter den vorab definierten PCM-Schwellen. Neue getrennte Program-17-Fixture isoliert Filterbewegung von Amp-Hüllkurve; Original-Program 0 für sieben bisher bestandene Regler unverändert. Keine Produktänderung, keine gelockerten Schwellen. Erneuter Nachweis ausstehend; falls drei Regler weiter scheitern, Original-DCYSUS-Registerkontext und SF2-Generatorübersetzung untersuchen statt Grenzen zu senken.

45504615247: Program-17-Referenz zu leise; drei Filterfälle noch nicht ausgeführt. Neuer diagnostischer Versuch mit tieferer Note, angehobener Grundfrequenz des Filters und längerem Amp-Release. Die Gültigkeitsschwelle bleibt zwingend. Alle sechs nichtneutralen Filter-Trials auch bei ungültiger Referenz messen, plus fallbezogene DCYSUS-Felddeltas soweit eindeutig aus Trace rekonstruierbar. Keine Produktänderung.

45508070522: 26 eindeutige Tracepaare bestätigen alle drei fehlenden DCYSUS-Felder und abgestufte Richtung. PCM-Gate noch FAIL wegen großer Neutralabweichung. Vor jedem Filtertrial tatsächliche Ruhe messen statt CC120 als wirksame Stummschaltung vorauszusetzen; nur Fixture-Amp-Release 4 s, gleiche PCM-Grenzen. Trace-Feldnachweise künftig Pflicht. Keine Produktänderung und keine klangliche Abnahme.

45509839214: tatsächliche native Klangprüfung 10/10 PASS, alle Ruhefenster und Neutralrestore PASS, alle drei DCYSUS-Feldgates PASS. Gesamtlauf FAIL ausschließlich im nachgeschalteten CRLF-Parser, spätere Gesamtregression nicht durchgeführt. Parser auf zeilenweise Prüfung umgestellt, portable positive und negative Tests verpflichtend. Keine Produktänderung.

45512094711: vollständige Windows-Gesamtregression SUCCESS, einschließlich 10/10 nativer PCM-Reglergates, 3/3 Registergates, aller bisherigen Prüfungen. Nächster Test: echter MOD RESET Klick im bestehenden Editorhost, zwölf Nullwerte und Host-Gesten, unveränderte fünf unabhängige Parameter sowie v3-State. Noch kein neuer GUI-Reset-Nachweis; keine Produktänderung.

## 28.11.2026 — Bedienkorrektur: absolute aktive Hüllkurven und Filter

Diese Korrektur ersetzt ausdrücklich die oben dokumentierten historischen Regelrichtungen, nicht die geprüften Registeradressen, Bitmasken oder den originalen MIDI-/EMU8000-Pfad. Nutzer meldet falsche Filterrichtung, teilweise kaum hörbare Attack-Wirkung und verlangt kurze, knackige Sounds.

- **0:** Einzelparameter-Bypass, exaktes originales Registerwort. **1–255:** absoluter aktiver Regelbereich; bewusst keine stetige Interpolation zwischen Originalwert und erster aktiver Stellung. Damit lässt sich auch ein langsames Original-Preset verkürzen. MOD RESET behält die bisherige Nullsemantik und alle Parameter-IDs bleiben gleich.
- AMP/FILTER Attack: Rate 127 → 6, nach gepinntem `snd_emu8k.c` 3134–3150 ca. 6 ms → 1,98 s; nur bei aktiver Attack zusätzlich Delay/Hold auf null. Rate 0 niemals als schnellste Zeit verwenden (bedeutet Stillstand). Trigger/Reserved-Flags erhalten.
- AMP/FILTER Decay und Release: Rate 127 → 12 (ca. 24 ms → 3,96 s für 100 dB, keine pauschale Behauptung tatsächlicher Notenlänge). Separate Release- und Nicht-Release-Felder; Original-Release-Ziel und Engine-Bit erhalten. Sustain aktiv von 0 bis 127 (steigender Haltepegel bzw. Modulationspegel).
- Cutoff aktiv 0 → 255; Resonance aktiv 0 → 15. Attenuation, Sample-/DMA-Bits und bestehende vorzeichenbehaftete PEFE-Route/Pitch-Byte unverändert. Nur ohne originale Filterroute wird bei aktiver Filter-ADSR ein konstanter, durch Cutoff begrenzter positiver Depth bis 64 mit Basis-Kompensation erzeugt. Dadurch ändert die einzelne Hüllkurvenposition nicht zugleich die künstliche Tiefe. Im unteren aktiven Cutoff-Bereich auf höchstens einen Registerwert nach unten quantisiert, damit sowohl Basis als auch Tiefe monoton bleiben. Die Werte sind ein musikalischer Korrekturkandidat; PEFE-Kompensation arbeitet in angenäherten Registereinheiten, kein gemessener Frequenznachweis.
- Die Änderungen liegen im bestehenden `VoiceOverlay.hpp` (vom unveränderten Runtime-Patcher eingebettet). Keine Änderung am gepinnten EMU8000-C-Kern, an Creative SF2LIB, den Performance-Patches oder am GUI. Wie im bisherigen Register-Overlay wirken die Zeit-/Filterwerte bei den zugehörigen NoteOn-/Release-Schreibzugriffen; Live-Änderung einer gehaltenen Stimme wird damit nicht zugesichert. Alte gespeicherte Nichtnullwerte werden identisch gelesen, klingen aber absichtlich anders.

### Tatsächlich lokal ausgeführte Nachweise

`g++ -std=c++14 -O2 -Wall -Wextra -Werror -DNDEBUG VoiceOverlayTest.cpp`: PASS, 65.536 Originalwörter, alle 255 aktiven Positionen über alle ursprünglichen Ratecodes, 16.711.680 Kombinationen aus Cutoff/PEFE/Position. Test verwendet CHECK unabhängig von NDEBUG. Zusätzlich UBSan mit Abbruch bei erstem Fehler PASS. Acht separat kompilierte absichtliche Fehlervarianten wurden tatsächlich abgelehnt: umgekehrte Zeitrichtung, Attack-Rate 0, umgekehrter Sustain, umgekehrter Cutoff, gelöschtes Releaseflag, überschriebene signed Route, verändertes Neutralwort, gekoppelter Filter-Depth. Git-Blobs Header e99b271ce477248423f252878944f71400a61ccb, Test 1d83f41834ec40c15b1e3d0bc4967b24519b08fa stimmen mit lokal kompilierten Dateien überein.

### Noch offene Abnahme

Kein Windows-Build ausgelöst. Die vorhandene native PCM-Differenzprüfung allein beweist die musikalische Richtung nicht. Im gebündelten Windows-Gate alle zehn Parameter mit realen Register-Traces und getrennten Klangprofilen (kurzer/mittlerer/langer Attack/Decay/Release, Sustain-Pegel, Cutoff/Resonanzspektrum, neutraler Ausgangsklang) prüfen; außerdem Hold/Release, extreme Werte, Recall, MOD RESET, alle Besitzerbanken. Die ursprüngliche Engine-DLL bleibt für den residenten Referenztest gültig. Für den konsolidierten, neu gebauten privaten Kandidaten ist zusätzlich `AWE64_TEST_RESIDENT_ENGINE_SHA256` mit dem unabhängig verifizierten SHA-256 aus dessen Build-Manifest zulässig; exakt 64 Hexzeichen und Bytevergleich erforderlich, kein automatisches Akzeptieren unbekannter DLLs. Neue DLL, Windows-Integration und dieser Diagnosepfad sind noch NOT RUN. Keine endgültige Freigabe.

## 28.11.2026 — Vollständiger Regler-Zuordnungsnachtest (lokal / Quellpfad)

- Alle 13 Drehregler abgeglichen: 1100–1103 AMP ADSR, 1104–1107 FILTER ADSR, 1108 Cutoff, 1109 Resonance, 1110 Reverb-Send, 1111 Chorus-Send, 1001 Master. Beide Effekt-Typen 1200/1201; GUI-Index, CMake-generierter aktiver Editor, Parameterbeschriftung, normalisierte Richtung, Legacy-/Demand-Hostpackung, Recall und ABI-Byte-Reihenfolge stimmen. 61 statische Quellchecks PASS. MOD RESET adressiert alle zwölf Mod-Regler, nicht Master/Typen/Bank/Programm; tatsächlicher Editor-Klick noch ausstehend.
- Lokaler erneuter C++14-Lauf der exakten VoiceOverlay-Blobs e99b271ce477248423f252878944f71400a61ccb / 1d83f41834ec40c15b1e3d0bc4967b24519b08fa: PASS. Neu: produktive VoiceWrite/AbsoluteSendWord/ApplyAbsoluteSends-Fragmente mit Fake-Registerspeicher kompiliert. 10 Regler einzeln bei 1/128/255, beide IFATN/PEFE-Schreibreihenfolgen, NoteOn/Off, Engine-/DMA-/Reserviert-Flags der Voice-Overlays, 9 unabhängige Reverb-/Chorus-Send-Kombinationen über 30 Stimmen; fremde Felder und reservierte Stimmen erhalten. MasterGainPlan mit exakten Originalblobs PASS (beide Kanäle, 0, Unity, Ramp/Automation); alle 16 Effekt-Typ-SysEx-Makros und Prüfsummen lokal PASS. Keine vertauschte Zuordnung gefunden.
- Echter Defekt reproduziert: Legacy-MPU-Pump zog einen Folgesteuerbefehl (der keine UART-Bytes emittiert) bereits bei noch ungearbeitetem vorherigem MIDI-Paket. Bisher nur im Host-Demand-Modus verhindert. Korrektur: in beiden Modi erst am leeren UART nach abgeschlossenem synchronem Originalhandler einen Paket-/Steuerbefehl ziehen. Originaler Legacy-Code FAIL im neuen Regressionstest, korrigierter Code PASS; zusätzliche UBSan-Prüfung PASS. Vier gezielte Mutationen (AMP/FILTER Attack vertauscht, Cutoff/Resonance Byte vertauscht, Release/Decay vertauscht, Reverb/Chorus vertauscht) wurden tatsächlich abgelehnt. Neuer portabler Test im Workflow vor Windows-Build. Lokaler kompilierter Test-Blob 6d262cc8a376303fc5fceea40b73d540baecf73b; geprüfte extrahierte Fragmente FNV-1a32 Voice 5120 ASCII-Bytes aed9a591, korrigierter Pump 2402 ASCII-Bytes d9745318, mit GitHub-Quelltext abgeglichen. Der vollständige Python-Patch-/Workflow-Lauf ist noch NOT RUN.
- Grenzen: 0 ist bewusst Preset-Bypass (Sends hingegen absolute 0 = aus). Erste aktive Stellung kann je nach Originalpreset springen; innerhalb des aktiven Bereichs korrekte Regelrichtung. Filter-ADSR verwendet die reale gemeinsame EMU8000-Modulationshüllkurve und kann deshalb bei vorhandener Pitch-Route auch Pitch modulieren. Das ist Hardware-Semantik, keine vertauschte Zuordnung. Voice-Overlays wirken an NoteOn/Release-Schreibzugriffen, keine garantierte Live-Cutoff-/Resonance-Änderung an bereits gehaltenen Noten. Legacy-Host quantisiert Overlays pro Block; FIFO-Korrektur ist kein Nachweis samplegenauer Legacy-Automation. Echte DSP-/PCM-Richtung, akustische Effekt-Typen, Live-Verhalten, Editor-Interaktion, native CPU/Latenz einschließlich dichter Polyphonie/MIDI nach Pump-Korrektur und alle Besitzerbanken weiterhin NOT RUN. Keine Änderung der Regelmathematik, der Original-GUI, des gepinnten EMU8000 oder beider Performance-Optimierungen. Kein neuer Windows-Build, keine Freigabe.

## 28.11.2026 — Adversarialer Regler-Nachweis: Soll/Ist statt bloß PASS

**Vertragsstatus: PARTIAL / NOT VERIFIED für die tatsächliche Produkt-/Klangabnahme.** Selbst-Challenge, keine unabhängige Zweitprüfung. Exakter Produktquellstand `9bdde4a15bc8d29a8809fd3234b2964dc4ac0819` (main, Windows x64 Entwicklungsinstrument); Engineering `9e4814158db40970d177e657cfda1ec4cd3975a1`, Audit-Vertrag Blob `1bb7508e1fd482503d0c1198f2a8e38f40e0a88f`. Scope 16 UI-Funktionen: 13 Drehregler, zwei Effekt-Typen und MOD RESET. Die übrigen Browser-/Lade-/Navigationsfunktionen sind nicht Teil dieses begrenzten Regler-Nachtests. Kein FULL AUDIT; VST3-Factory/Lifecycle/Paketierung und vollständige DAW-/State-/Automation-/GUI-Prüfung nicht neu abgeschlossen. Linux g++ 14.2.0, kein Windows/Wine/ROM-Host in diesem Lauf. Keine kostenpflichtigen Builds. Tatsächlicher Dispatcher mit künstlichem Registerspeicher, keine Synthese; unabhängige Zeitumrechnung anhand des gepinnten EMU8000-Quelltexts, keine PCM-Messung.

### Beobachtete Registerwörter (hexadezimal)

Die Testinstrumentierung hat die tatsächlichen Rückgabewerte der extrahierten produktiven VoiceWrite-Funktion ausgegeben. Ein Regler aktiv, alle anderen neutral; jeweils Originalwort bei 0, dann aktive quantisierte Werte 1/128/255. Die anderen Bits bleiben nach den zugehörigen Registerassertions unverändert, abgesehen vom ausdrücklich gewählten Attack-Hold-Override. Bei 0 bleibt der Originalwert; 0→1 kann deshalb bewusst springen.

| Regler / ParamID | Register | 0 | 1 | 128 | 255 | Gezielte falsche Zuordnung / tatsächliche Assertion |
|---|---|---|---|---|---|---|
| Amp Attack 1100 | ATKHLDV | 4050 | 7F7F | 7F42 | 7F06 | Filter-Attack-Index; FAIL Z. 60 |
| Amp Decay 1101 | DCYSUSV | 5A55 | 5A7F | 5A45 | 5A0C | Sustain-Index; FAIL Z. 62 |
| Amp Sustain 1102 | DCYSUSV | 5A55 | 0055 | 4055 | 7F55 | Decay-Index; FAIL Z. 62 |
| Amp Release 1103 | DCYSUSV | 8051 | 807F | 8045 | 800C | Decay-Index; FAIL Z. 72 |
| Filter Attack 1104 | ATKHLD | 3055 | 7F7F | 7F42 | 7F06 | Amp-Attack-Index; FAIL Z. 61 |
| Filter Decay 1105 | DCYSUS | 4B50 | 4B7F | 4B45 | 4B0C | Sustain-Index; FAIL Z. 63 |
| Filter Sustain 1106 | DCYSUS | 4B50 | 0050 | 4050 | 7F50 | Decay-Index; FAIL Z. 63 |
| Filter Release 1107 | DCYSUS | 8052 | 807F | 8045 | 800C | Decay-Index; FAIL Z. 73 |
| Cutoff 1108 | IFATN | C05B | 005B | 805B | FF5B | Resonance-Byte gelesen; FAIL Z. 69 |
| Resonance 1109 | CCCA high | 302A | 002A | 802A | F02A | Cutoff-Byte gelesen; FAIL Z. 64 |

Unabhängige Sollwerte aus `turican0/dosbox-x-AWE32@81d0fde73917b57a1716e610dec2605c36def555`, `snd_emu8k.c` 3134–3163 (Ratecodes) und 362–371 (Chamberlin-Filter): Attack-Codes 127/66/6 entsprechen ungefähr 5,987/82,486/1979,667 ms; Decay/Release-Codes 127/69/12 ungefähr 23,948/282,815/3959,417 ms pro 100 dB. Tatsächliche Noten-Abklingzeit hängt vom Ausgangs-/Zielpegel ab. Aktive Cutoff-Codes 0/128/255 entsprechen unmoduliert ca. 101,81/893,96/7350 Hz (letzteres geklemmt auf fs/6; tatsächliches Filterverhalten einschließlich Bypass/Modulation bleibt gesondert zu prüfen). Sustain-Codes 0/64/127 steigen; Resonance-Codes 0/8/15 steigen. Das ist der Nachweis der korrekten aktiven Registerrichtung, nicht der hörbaren DAW-Kurve.

| Weitere Funktion | Beobachtung / positives Ergebnis | Test-only Fehler / beobachtetes Ergebnis |
|---|---|---|
| Reverb Send 1110 | PTRX 0/128/255; CSL jeweils 0; 9 kombinierte Send-Paare über 30 Stimmen | falsches High-Byte gelesen; FAIL Z. 89 |
| Chorus Send 1111 | CSL 0/128/255; PTRX jeweils 0 | falsches Low-Byte gelesen; FAIL Z. 89 |
| Master 1001 | MasterGainPlan prüft PCM L=.5 / R=-.25 bei Unity, beide exakt 0 bei 0, 512 Samples Ramp, Flush/Grenzfälle | rechte Gain-Multiplikation weggelassen; FAIL Z. 12 |
| Reverb Type 1200 | GS-Adresse 30h, Typen 0–7, Checksummen | Adresse 38h; Assertion Z. 7 |
| Chorus Type 1201 | GS-Adresse 38h, Typen 0–7, Checksummen | Adresse 30h; Assertion Z. 7 |
| MIDI-Steuerreihenfolge | zuerst Note 60, dann Overlay-Update, dann Note 61; alter Wert vor vollständigem Konsum | ursprünglicher 8-Paket-Pump; FAIL Z. 120, weil Folge-Overlay vor erster Note verarbeitet |
| MOD RESET | 12 Parameter-IDs statisch geprüft, Master/Typen/Bank/Programm nicht adressiert | echte Klick-/Host-Gesten-/Recall-/PCM-Gegenprobe NOT RUN |

### Tatsächliche Testqualität / Reproduzierbarkeit

Alle 16 oben genannten negativen Varianten einzeln in temporären Kopien kompiliert; positive Dispatcher-, Master- und Effektprüfungen vorher erfolgreich. **16/16 negative Varianten an den vorgesehenen Laufzeitassertions abgelehnt**, 15 SIGABRT und ein Returncode 1 (Master). Kein Compilefehler als erfolgreicher Negativnachweis gezählt. Der erste positive Versuch für Master war mit C++14 nicht kompilierbar (std::clamp); danach mit dem benötigten C++17 neu kompiliert und tatsächlich ausgeführt. Separate Tests mit C++14 für die Overlay-Mathematik und UBSan wiederholt: alle 65.536 Wörter und 16.711.680 Filterkombinationen. Geprüfte Blobs: `ControlRouteContract.cpp` 6d262cc8a376303fc5fceea40b73d540baecf73b, `VoiceOverlay.hpp` e99b271ce477248423f252878944f71400a61ccb, `VoiceOverlayTest.cpp` 1d83f41834ec40c15b1e3d0bc4967b24519b08fa, `MasterGainPlan.hpp` 5625c641ec045706dc21e4a1f445d4b22c201b02, `AuditMasterGainTest.cpp` d5bd19606e393e05e6746fdf5c6b46a543f0669d, `CreativeEffectMidi.hpp` 64d73a576fe53fc22c5ff4fa522cd40de95c5591. Vollständiges lokales Challenge-Log SHA256 7ec0bdb6bfe634652f2f13e1e6f75ec4c2b45deae0bc0a9df4c9f9ee487a9683. Die tatsächlichen Assertionstellen und beobachteten Werte stehen oben; keine bloße Workflow-Zählung. Die Mutationen wurden ausschließlich lokal ausgeführt; keine absichtlich fehlerhaften Produktdateien committed.

Bestehende native `VOICE_PCM_RESULT`-Prüfung bewertet primär absolute Differenz gegenüber neutraler Referenz; ein vertauschter oder invertierter Regler kann damit trotzdem hörbar unterschiedlich sein und PASS erhalten. Deshalb sind in der gebündelten nativen Abnahme richtungsbezogene unabhängige Soll/Ist-Messungen Pflicht (zeitliche Hüllkurven bei geeignetem Signal/NoteOff-Fenster, stationärer Sustain, Spektralwirkung/Cutoff/Resonanz, isolierte Send-Pfade, Master-Amplitudenverhältnis, neutrale Referenz einschließlich MOD RESET und realem Recall). 0/klein/mittel/max pro Regler, kombinierte Einstellungen, gehaltene Stimmen, Program-/Bankwechsel, echte Automation, mehrere Instanzen und Polyphonie/CPU/Latenz einschließlich neuem Pump. Ohne gültige Referenzsignale und nachgewiesene Fehlersensitivität kein Produkt-PASS. Bestehende Originalroute der Filter-ADSR kann Pitch beeinflussen; Live-Änderung gehaltener Note weiterhin nicht zugesichert. 

**Getrenntes Urteil:** Quellzuordnung für die betrachteten Parameter statisch geprüft; Register-Dispatcher/Mathematik, Stereo-Master und SysEx isoliert ausgeführt, jeweils lokale Evidenz vorhanden. Vollständige Produktionspfade / Fehlerfälle / Recall / nativer GUI-Klick einschließlich Negativkontrollen: PARTIAL. Windows-Build, Validator, 125A Plugin Tester, echte VST3-PCM, DAW und musikalische Prüfung: NOT RUN für diesen Stand. Gesamtfreigabe: NO-GO, kein neuer Build und keine Produktänderung in diesem Nachweischeckpoint.

## 28.11.2026 — PCM-Bericht darf Messfehler nicht als PASS akzeptieren

- Native Regler-Differenzprüfung: bisheriger Ergebnisparser akzeptierte sämtliche 14 widersprüchlichen Testberichte trotz PASS-Text (Nullwirkung, Delta gleich Schwelle, Rauschen oberhalb Schwelle, Schwelle null, unendliche/NaN/negative/fehlende Zahlen, falsche Probeprogramme, nicht stille oder ungültig gezählte Ruhefenster). Originalparser gegen exakt dieselben Gegenbeispiele ausgeführt: 14/14 falsch akzeptiert. Korrigierter Parser weist alle 14 zurück; beide gültigen Zeilenendformate und 10 bisherige Struktur-Gegenproben ebenfalls geprüft.
- Korrektur im bestehenden WindowsRunVst3HostPcmSmoke.py, keine neue Paketabhängigkeit. Lokaler Python-Test mit exakt extrahiertem Parserfragment (2645 ASCII-Zeichen, FNV-1a32 8a9277c3) und Testblob e04f5b52c418b4e6df62dfcc3ef0b53cab3d2c4e ausgeführt: drei Unittest-Methoden OK. Diese Prüfung beweist ausschließlich Zahlenvalidierung und Differenzschwellen, NICHT akustische Richtung, tatsächliche PCM oder native Funktion. Windows-Lauf, Richtungsgates, Presetprüfung und Produktfreigabe bleiben offen. Keine Produktänderung und kein kostenpflichtiger Build.

