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
