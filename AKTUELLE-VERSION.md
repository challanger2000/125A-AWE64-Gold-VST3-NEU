# AKTUELLE VERSION – verbindlicher Stand

**Produkt:** 125A AWE64 Gold (Windows x64 VST3, DOSBox-X)  
**Stand:** 23.11.2026  
**Entwicklungskennung:** `0.0.0-dev`  
**Release / abgenommenes Instrument:** **NEIN – Entwicklungsversion gebaut, Original-Creative-Klang/DAW-Abnahme offen**  
**Einziger Branch:** `main`  
**Exakter neuester Quellstand:** [oberster Commit auf main](../../commits/main) (nicht auf eine veraltende SHA im Text verlassen)

Alle nachfolgenden Aussagen beziehen sich auf **nachgewiesene Prüfungen oder klar gekennzeichnete Implementierung**. Ein erfolgreicher Windows-Build oder ein DOS-Shell-Test beweist keinen AWE64-Sound.

## 24.11.2026 — Effektumschaltung im echten VST3 bestanden

- Windows 45309812893: alle drei RAM-Spektralfälle bestanden, unveränderter Originalgast Effekt-Register 0/0, korrigierter Originalgast 1/1, tatsächlicher VST3 `EFFECT_TRACE_PLATE=True; EFFECT_TRACE_FEEDBACK=True`. Damit sind beide getesteten originalen Effekttypen im VST3 nachgewiesen. Der ältere Lauf scheiterte noch an den unveränderten CC-Sends, nicht an der Effekttyp-Umschaltung.
- Windows 45310753140: neuer Cheezoids-/Absolute-Send-Lauf noch aktiv. Linux-Quellpatchtest scheiterte ausschließlich an einer unvollständigen synthetischen EMU8000-Testvorlage (fehlende neue Zeitbasis-/Write-/Mix-Anker). Vorlage ergänzt, Send-Export und Registerwrite-Aufruf explizit geprüft, keine Änderung des Produkt-DSP. Versions- und Transportregressionen 45310753256/45310753337 bereits PASS. Keine Freigabe.

## 24.11.2026 — Cheezoids: gezielte Regression aller 128 Programme

- Nutzer meldet aus früherem Stand mehrheitlich falsche kurze Bleep-Klänge. Die konkrete private Datei war in der Bibliotheksliste nicht vorhanden. Öffentlich freigegebene Originalbank von Analoguesque Sound Designs (Ian Wilson) im Autor-Repository `Analoguesque/freepatches`, Commit `681907983ec024c1994f6a02615d650877913696`, Archiv-Git-Blob `c12461b3e016f70b36e39eca2f78eac5685a0fbf`. Nur temporär herunterladen, keine Bankdaten, PCM oder Traces hochladen. Identität zur früheren privaten Kopie nicht behaupten.
- Autor beschreibt kurze Single-Cycle-Wellenformen und Kompatibilitätsprobleme mancher SF2-Player. Deshalb ausdrücklich kein einzelner Peak als Klangbeweis: alle 128 tatsächlichen Bank-0-Programme, Note 60 Velocity 100, etwa 1,75 s gehalten, frühe/späte RMS-Fenster und Original-Creative-DLL-Direktvergleich mit identischen trockenen Hardware-Sends; VST3-Programmauswahl und Bankwechsel über echten Host. Meldet verlorenen Sustain, abweichende Programmpegel und kurze Originalreferenzen getrennt; Mehrheits-Bleep auch im direkten Originalpfad bleibt Fehler zur Untersuchung. Originalquelle unverändert. Noch kein ausgeführter Cheezoids-Klang-PASS.

## 24.11.2026 — Absolute Hardware-Sends statt additiver Creative-CCs

- Die bereits nachgewiesene Nicht-Null des originalen CC93 trotz Wert 0 wird nicht als Produktfunktion ausgegeben. Die P165-Sends 1110/1111 setzen jetzt ausschließlich die beiden originalen EMU8000-Registerbytes PTRX[15:8] und CSL[31:24] für Stimmen 0–29, via originaler Register-Schreibroutine im DOSBox-Gerätewrapper. Keine Änderung der gepinnten Chip-DSP-Implementierung. Der Host publiziert nur ein atomares Wertepaar; kein Chipzugriff vom DAW-Callback. Nach Gastbereitschaft wird 0/0 als Standard gesetzt, bei Live-Änderung auch bestehende Stimmen aktualisiert; Original-Referenz ohne gesetzte Host-Overrides unverändert. Reservierte Stimmen 30/31 und andere Registerfelder bleiben unangetastet.
- GUI-Kompatkopie aktiviert die beiden bestehenden Sendregler, Prozentwerte, State v4 (v1–3 weiter lesbar, Default dry). Neue Windows-Tests am echten VST3 mit eingebetteten hohen SF2-Sends: 0/0, 255/0, 0/255, jeweils echte RAM-Voice-Register und Controller-State-Recall. Original-CC-Referenz bleibt als separate Beobachtung erhalten. Native Ausführung und Wirkungsprüfung stehen noch aus.
- Quelltext-Regressionsprüfung nach der bewiesenen SysEx-Korrektur an den tatsächlich dynamisch zusammengesetzten DOS-Aufruf angepasst; vorheriger Stringvergleich war veraltet. Kein Release.

## 24.11.2026 — Original-SysEx-Korrektur tatsächlich bewiesen und in VST3 integriert

- Windows 45306739416: originale unveränderte Creative DEMO.EXE liefert beide Registersignaturen 0/0, die ausschließlich temporär um ein Byte korrigierte Kopie liefert Plate=1 und Short Delay Feedback=1. Damit ist der konkrete Vergleichsfehler experimentell bestätigt, nicht mehr nur Disassemblierungshypothese. Beide RAM-Programme, 48-kHz-Klang und Sparse-Recall weiter PASS.
- VST3-Start übernimmt nun die exakt geprüfte Korrektur ausschließlich für die bekannte 40197-Byte-Originaldatei nach vollständiger Git-Blob-SHA1-Prüfung via Windows BCrypt und Instruktionsanker. Temporäre FXDEMO.EXE im ohnehin privaten, pro Start angelegten Verzeichnis; Original bleibt byteidentisch. Kein neuer Treiber, keine Änderung des EMU8000-Kerns, keine Veröffentlichung von Creative-Binärdaten. Eigener synthetischer C++-Regressionstest. Echter VST3-Effektregister-Nachweis dieser Integration steht noch aus.
- Unveränderte Original-CC91/93-Endpunktprüfungen weiterhin FAIL. Kein Release.

## 24.11.2026 — Original-SysEx-Vergleichsfehler auf Instruktionsebene isoliert

- Korrektur des neuen Prüfskripts: Git-Blob-Header mit echtem NUL-Byte und eigener YAML-Testschritt; keine Änderung der Originaldaten oder Produktlaufzeit.
- Windows 45301207140: Byte-exakter echter 33-Byte-MPU-UART-Test PASS; alle drei RAM-Spektralfälle und Sparse-State-Negativtests PASS. Original-Direktgast empfängt beide GS-Makros, aber beide Effektregister fehlen, ebenso im VST3. CC91/93=0 liefert bei expliziten SF2-Sends weiterhin Chorus 250, 127 liefert 255; Reverb-Messung 0/0. Keine Send-Freigabe.
- Analyse der hashverifizierten originalen Creative DEMO.EXE (MZ-Datei, 40197 Bytes, dokumentierter Git-Blob): bei Datei-Offset 21834+0x35 lädt die SysEx-Musterprüfung AL aus CS:[SI], führt CBW aus und vergleicht danach AX mit nullerweitertem DX. Statusbyte F0 wird FFF0, empfangenes F0 wird 00F0, also zwangsläufig ungleich. AH ist beim Einstieg in beide Muster durch AX=0x12/0x28 bereits 0; eine lokale NOP statt CBW (Offset 21892) erhält AH=0. Keine Änderung der Effektalgorithmen, ROM-Daten oder Chipemulation.
- Nur für isolierten privaten Archiv-Referenztest: exakter Original-Git-Blob und Instruktionsanker vor der Änderung, temporäre FXDEMO.EXE mit einem Byte Unterschied, Original niemals überschreiben, kein Upload. Original unverändert als Negativkontrolle, korrigierte Kopie muss die originalen INIT-Registersignaturen zeigen. Der Produktions-VST3-Pfad bleibt bis zum Nachweis unverändert. Kein Release.

## 24.11.2026 — Original-Send-Endpunkte prüfen (0 % muss tatsächlich aus sein)

- Original ADIP DOS awe32Controller dokumentiert CC91/CC93, nicht aber eine verlässliche absolute Nullsetzung für bereits im SF2 enthaltene Sends. Vor Aktivierung der beiden P165-Sendregler wird dies im originalen Creative-Treiber nachgemessen.
- Erweitert ausschließlich selbst erzeugtes RAM-Testmaterial um explizite 1000-Promille-SF2-Sendgeneratoren (15/16), getrennt vom unveränderten trockenen Frequenztest. Zwei unabhängige Originalgast-Läufe setzen CC91/93 = 0 bzw. 127. Original-PTRX/CSL-Register an späten RAM-Voice-CCCA-Schreibzugriffen ausgewertet; 0 verlangt beide Register-Sends exakt null, 127 beide größer null. Kein approximierter Effekt und kein fiktiver Regler. Windows-Prüfung offen.
- Windows 45299958012 bestätigte erneut beide 44,1-kHz-RAM-Klänge und 48 kHz, Effektregister weiterhin FAIL. Keine Gesamtfreigabe.

## 24.11.2026 — Sparse-Preset-Recall ohne falsche Programmauswahl

- Bestehender Fehler: nach SF2-Wechsel konnte setState ein nicht vorhandenes Preset oder unbewiesene Banknummer erneut setzen. Browser konnte vor Aktivierung noch die alte Liste zeigen. Jetzt vor jedem State-Wechsel tatsächliches phdr-Preset validieren, ungültige Bank/Preset-Kombination ohne Mutation zurückweisen; auch inaktive Auswahl aktualisiert Browser-Snapshot. Kein ROM-Padding in externen SF2.
- Echtzeit-Programmautomation akzeptiert nur vorhandene Programme; kBank bleibt 0, solange höhere Banken nicht nachgewiesen sind. Controller-Readback für Bank/Programm/Master/Effekttypen direkt aus dem tatsächlichen atomaren Zustand. Native negative Tests prüfen nicht vorhandenes Programm 1 und Bank 1 in der 0/17-RAM-Bank, einschließlich unverändertem gültigen State. Windows-Nachweis noch offen.
- Original-P165-Grafik, originaler Creative-Treiber und EMU8000 unverändert.

## 24.11.2026 — SysEx-Fehler am Originalpfad isolieren

- Windows 45296517847 bestätigt die RAM-Programme 0/17 und 48-kHz-Audiowiedergabe; keine Änderung dieser bestandenen Pfade. Original-Effekt-Recall funktioniert, aber beide späten INIT-Registersignaturen fehlen.
- Originale Creative ADIP-DOS-API dokumentiert beide GS-Makros; die originale SF2LIB/RAWE32S.LIB enthält tatsächlich die SysEx-Erkennung sowie Reverb-/Chorus-Funktionen. Kein Ersatz-DSP und keine Änderung des Originaltreibers.
- Neuer bytegenauer 33-Byte-UART-Test (Bank/Programm, zwei 11-Byte-Effekt-SysEx, NoteOn/Off), größer als der 32-Byte-FIFO; prüft vollständige Übertragung einschließlich Backpressure. Zusätzlich direkter Original-DLL-Vergleich derselben Effektbefehle ohne VST3/Controller: vollständige Paketfolge und originale späte INIT1/INIT3-Signaturen. Beide Gates bleiben verbindlich, kein Freigabetrick. Windows-Ergebnis offen.

## 24.11.2026 — Windows-RAM- und Hostrate-Nachweis

- Windows 45296517847: Original-RAM-Referenz und VST3-Programme 0/17 bei 44,1 kHz sowie Programm 17 bei 48 kHz mit erwarteter Frequenz nachgewiesen; 48-kHz-Leistungsverhältnis >223. Temporäre Referenzbereinigung erfolgreich. Effektregisterprüfung weiterhin FAIL (Plate=False, Feedback=False).
- Separater Quellpatch-Test scheiterte an veralteter exakter UART-Guard-Zeichenfolge. Assertion auf neuen Guard, Bereitschaftsexport und ACK-Reihenfolge aktualisiert. Transport- und Versionsworkflows PASS. Keine Gesamtfreigabe.

## 24.11.2026 — RAM-Klang bei 44,1 kHz nachgewiesen; Initialisierungs-PCM unterdrücken

- Windows 45294125748: originale RAM-SF2 Programme 0/17 im echten VST3 mit korrekten 441/735-Hz-Signaturen (Leistungsverhältnis >114), RMS 0,0249977/0,0277133. Direkte Original-DLL-Referenz RMS 0,025432038/0,027798085; RAM-Adressschreibzugriffe vorhanden. Direkter Test scheiterte nach der Messung ausschließlich an Windows-Freigabe von RAMBOOT.LOG. Parent besitzt nun das temporäre Verzeichnis und entfernt es nach Prozessende.
- 48-kHz-Fall scheiterte an PCM-Grenzwert vor vollständigem Bankwechsel; natives 44,1-kHz-Startsignal erreicht bereits im direkten Originalmixer Vollaussteuerung. Sinc kann bei solchen Initialisierungssignalen überschwingen. Statt Limiter/Clipping oder Änderung des Chips: Produktions-PCM erst nach UART-ACK und anschließendem echten MIDI-Statuspoll aus dem unveränderten DEMO-Empfangsloop in den Ring übernehmen. Vorher Stille; Bank/Programm und MIDI erst bei Bereitschaft. Exakte Startfolge anhand DEMO.C überprüft. Neu: Block-/Sample-/Rate-Diagnose bei PCM-Grenzverletzung; Schwelle unverändert.
- Effekt-Type-State-Roundtrip PASS, erwartete Originalregister jedoch nicht nachgewiesen. Separate Bool-Diagnose; SysEx-Format im Creative ADIP bestätigt (https://www.manualslib.com/manual/2031598/Creative-Awe32.html?page=20). Keine Freigabe, Windows-Regression notwendig.

## 24.11.2026 — Eigenes RAM-Test-SoundFont korrigiert und unabhängig validiert

- Lokale unabhängige Prüfung mit libfluidsynth.so.3: Vorheriges selbst erzeugtes SF2 wurde wegen ungerader INAM-Chunklänge (21 Byte statt 22) vom strengen Parser zurückgewiesen. Fehler im Testmaterial bestätigt; keine Änderung der Originalemulation oder des Originaltreibers.
- Korrektur: NUL-Ausrichtung innerhalb der INAM-Länge; Dateigröße unverändert 15972 Byte. Korrigierte SF2 lokal erfolgreich geladen, Programm 0 und 17 jeweils eine Sekunde bei MIDI 60/100 gerendert, RMS 0,01864656 / 0,01864633; richtige Frequenz gegenüber jeweils anderer mit Leistungsverhältnis >479000. Dies prüft ausschließlich die Gültigkeit des eigenen Testmaterials, **kein** Original-Creative-Klangnachweis.
- Struktureller Regressionstest für gerade Chunklängen, SF2 2.00, zwei RAM-Sampletypen, korrekte sparse Programme. Windows-Direktvergleich und natives Original-VST3-Gate weiterhin zwingend.

## 24.11.2026 — RAM-Fehler isolieren, Original-Effektprüfung unabhängig ausführen

- Windows 45292853463: Original-ROM-GM und externer ROM-GM-Bankwechsel erneut nachgewiesen (Held-RMS 0,00845848 bzw. 0,00839579). Erste selbst erzeugte RAM-SF2 (441 Hz) dagegen **FAIL**, Held-RMS 0,0000127707, 0 aktive Notenblöcke. Keine Gleichsetzung von ROM-Bankwechsel mit RAM-Klang.
- Neuer direkter Original-DLL-Referenztest derselben selbst erzeugten SF2 für Programme 0/17, unabhängig von VST3-State und Hostrate. Aggregierte EMU8000-DRAM-Datenregister-, Eigen-PCM-Signatur- und späte RAM-Adress-Schreibdiagnose, keine Originaldaten, Roh-Traces oder Audio veröffentlichen. RAM-Fälle werden getrennt ausgewertet; Fehler blockieren die unabhängige Original-Effektprüfung nicht und verhindern weiterhin den Gesamt-PASS.
- Portable Hostrate-/SysEx-Tests und Version 45293022107/45293391865/45293391664 PASS. Windows 45293391812 noch ausstehend. Keine Klangbehauptung ohne Auswertung; unveränderte Chipemulation.

## 24.11.2026 — Original-Creative-Reverb-/Chorustyp über DOS-SysEx

- Originales ADIP v3.01, Part II, awe32Sysex (https://doczz.net/doc/8351592/developer-s-information-pack): dokumentiert 11-Byte-GS-Effektvariationsbefehle; die unveränderte Creative-DEMO empfängt SysEx bereits. Erweiterung ausschließlich der Host-MIDI-Transportlänge/FIFO: vollständige Nachricht oder warten, keine Teilpakete, normale MIDI-Nachrichten unverändert.
- Aktiviert beide vorhandenen P165-Typmenüs im SDK-Kompatibilitätsquelltext, Hall 2 = 4, Chorus 3 = 2, echte Projekt-/Parameteranbindung; v3-State für abweichende Effektvariationen, v1/v2 kompatibel. Kein Eingriff in Chip-DSP. NRPN laut ADIP und originalem DEMO ausdrücklich **nicht** durch SF2LIB unterstützt; ADSR/Filter und exakte Send-Stärken bleiben offen. Keine falsche Funktionsfreigabe.
- Neuer nativer Originaltest: Plate/Short Delay Feedback über IEditController und State-Roundtrip, anschließende Original-Registertrace-Kontrolle (INIT1 3 = B4FF, INIT3 9 = E6C0; unabhängige Referenz https://codebrowser.dev/linux/linux/sound/isa/sb/emu8000.c.html). Noch unbestätigt. Keine Besitzerdateien/Audio/Traces veröffentlichen, nur main.

## 24.11.2026 — Native 44,1 kHz unverändert; Hostrate-Konverter

- 44,1-kHz-DOSBox-Mixer unverändert, bittransparenter Bypass bei 44,1-kHz-DAW. Für höhere Hostraten bis 192 kHz: 32-Tap-Blackman-Sinc, 1024 Phasen mit Interpolation, exakte rationale Phase, 16 native Samples FIR-Verzögerung, keine Allokation im Callback. Keine Änderung des EMU8000-Takts oder Creative-Treibers. Asynchrone Transportlatenz und Pufferstabilität bleiben separate offene Prüfungen.
- Portable Tests prüfen 48/88,2/96/192 kHz gegen analytischen 3-kHz-Sinus, Stereo-Isolation, exakte Eingangsverbrauchsrate und Blockgrößenunabhängigkeit. Windows-Original-RAM-Preset-735-Hz-Gate zusätzlich bei echter VST3-Hostrate 48 kHz. Beide neuen Prüfungen noch offen. Vorheriger RAM-Test bleibt in CI-Warteschlange; keine Freigabe.

## 24.11.2026 — Original-SF2-Wechsel PASS; separate RAM-Sample-/Preset-Klangprüfung

- **[Windows 45290310337](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45290310337) vollständig SUCCESS** auf 0f7588a1; Transport 45290310165 und Version 45290310749 PASS. Tatsächliches IComponent-State-Roundtrip, aktiver Originalgast-Neustart, externe GM-Referenzbank, NoteOn/Off, Held-RMS 0,00828747 (ROM-GM 0,00837682), EMU8000-Writes 12903, Quelle unverändert. Vorheriger Timeout nach Umstellung auf temporäre Ausgabespools behoben. Kein bloßer synthetischer PASS.
- Jetzt zusätzliche selbst erzeugte SF2-2.00 mit zwei RAM-Sinus-Samples und exakt zwei Presets (0/17), keine ROM-Samples und keine fremden Audiodaten. Nativer Original-VST3-Test muss je Programm die passende 441/735-Hz-Signatur deutlich über der anderen Frequenz messen, zusätzlich zu den bisherigen Note/Ruhe/Release-Gates. Fixture nur temporär erzeugt, SHA unverändert. Originaler Creative-Lader und EMU8000 bleiben unverändert. Neue RAM-Prüfung noch nicht bestätigt. Keine Freigabe.

## 24.11.2026 — Aktiver SF2-Test: Timeout mit sicherer Log-Erfassung untersuchen

- Windows 45288976627: Build, synthetische Positiv-/Negativkontrollen und musikalischer Original-GM-Test erreicht (Held Note RMS 0,00795573). Separater aktiver SF2-/Recall-Test nach 90 s Timeout; Ursache nicht nachgewiesen. Transport/Version PASS.
- Beseitigt einen realen Deadlock-Pfad im Python-Testhost: Windows-Subprozess schrieb in nicht kontinuierlich geleerte stdout/stderr-PIPEs, während Elternprozess nur pollte. Temporäre lokale Dateien statt Pipes, begrenzte Diagnose bei Timeout, sofort geflushte native Marker vor/nach Wechsel. Keine Änderung der Klangschwellen oder Emulation. Neuer aktiver Windows-Test offen.

## 24.11.2026 — Native Recall-Testhost korrekt mit SDK MemoryStream linken

- Windows 45287430560 und 45287808293: Build des VST3 selbst nicht als fehlerhaft ausgewiesen; nativer Testhost scheiterte beim Linken an fünf undefinierten Steinberg-MemoryStream-Symbolen. Kein SF2-Klangurteil aus diesen Läufen. Transporttests 45287430562 und 45287808294 sowie Versionsprüfungen PASS.
- CMake bindet die Implementierung `public.sdk/source/common/memorystream.cpp` aus demselben gepinnten SDK explizit im Testhost ein. Keine Änderung an EMU8000, Creative oder GUI. Neuer Windows-Lauf erforderlich; kein Release.

## 24.11.2026 — Aktiver SF2-/ROM-Wechsel und echte sparse Presetliste

- Transport 45287430562 und Version 45287430420 PASS; Windows-Originalgate 45287430560 läuft noch. Neuer Lauf darf die ausstehende Originalprüfung nicht abbrechen (CI-Warteschlange statt Abbruch).
- SF2-/ROM-Knopf und State-Wechsel starten denselben originalen Creative-DOS-Gast im selben VST3 neu. Kein externer Prozess, keine DSP-Nachbildung. Lifecycle-Mutex ausschließlich blockierend außerhalb des Audiothreads; process() liefert beim nicht blockierenden try_lock-Ausfall Stille. Vorherige Bank bei synchronem Startfehler erneut versuchen. Originalgrafik byte-identisch; kompiliertes Kompatibilitätsdialog-Verhalten ersetzt. Wiederholter Gaststart im selben Prozess ausdrücklich **noch nicht nachgewiesen**, möglicher Blocker: globaler DOSBox-/SDL-Zustand.
- Bank-0-Presets aus tatsächlichem phdr, keine Auffüllung externer SF2. Höhere Bänke nicht freigegeben. Browser erhält getrennten Snapshot über UI-Revision, nicht vom Audiothread. Alte Presetnummern ohne vorhandenen Eintrag werden auf ersten vorhandenen Eintrag gesetzt. Kein Anspruch auf vollständige Treiber-Banksemantik.
- Der Windows-Originaltest lädt nun zuerst den ROM-Gast und wechselt über die echte IComponent-State-Schnittstelle bei aktivem Plugin zur externen GM-Referenz; danach vollständiges Note/Ruhe/Release-Gate. Dieser neue Test steht aus. Keine Besitzerdateien publizieren, kein Release, nur main.

## 24.11.2026 — SF2-Prüfskript korrigiert

- Windows 45287143543 und Transport 45287143763 brachen vor dem Klangtest wegen eines fehlerhaften PowerShell-Regex-Einfügevorgangs ab. Doppelt eingefügten Skriptabschluss entfernt; Trace-Filter wieder korrekt. Keine Änderung an Plugin, DSP oder Originaltreiber. Externer SF2-Recall-/Klangtest weiterhin offen.

## 24.11.2026 — Echter VST3-Recall-/SF2-Ladepfad als separates Windows-Gate

- [Windows 45284478927](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45284478927), [Transport 45284478906](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45284478906), [Version 45284478902](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45284478902) SUCCESS auf 17bb039b. Bisheriger GM-Originalpfad bleibt funktionsfähig; dieser Lauf hat noch keine externe Bank geladen.
- Neuer separater Originaltest: tatsächliches IComponent::setState/getState mit externer UTF-8-SF2-Datei, fehlender Pfad muss fehlschlagen, State bytegenau zurücklesen, danach realer VST3-Notentest mit externer temporärer GM-Referenzkopie über D:\\BANK.SF2. Creative-Bootdiagnose, EMU8000-Trace und unveränderter SHA-256 der Quelle erforderlich. Identische GM-Referenz prüft den Ladeweg, **nicht** RAM-Sample-Klang, andere Banken oder Live-Wechsel. Keine Besitzerdateien hochgeladen. Neuer Testlauf nicht bestätigt.
- Original-P165-Grafik, Creative-Treiber und EMU8000 bleiben unverändert. `0.0.0-dev`, nur main, kein Release. Keine ungefragten Zwischenberichte; Abnahme erst bei vollständigem Instrument.

## 23.11.2026 — Externe SF2-Startauswahl und Projektpfad

- [Windows 45281711814](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45281711814), [Transporttests 45281711812](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45281711812), [Version 45281711986](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45281711986): SUCCESS auf d4bf7111; Mapping und Original-GM-Pfad bestanden.
- Externe SF2-Auswahl über vorhandenen P165-SF2-Knopf im generierten SDK-Kompatibilitätsquelltext; Original-GUI-Blob und Grafik unverändert. Auswahl wird validiert und beim nächsten Aktivieren über die originale Creative-DEMO geladen (DOS-Pfad D:\\BANK.SF2, private temporäre Kopie, Quellbank nie überschrieben, SF2 2.01 nur Versionsheader angepasst). Keine Behauptung eines laufenden Bankwechsels. DBXD-State v2 speichert UTF-8-Pfad; v1 bleibt lesbar. Fehlende/inkompatible Bank wird abgelehnt, nicht still ersetzt. Katalog bleibt vorerst MIDI-Nummern, keine falschen Presetnamen/ROM-Mischung; aktive Live-Auswahl, vollständige Banksemantik und tatsächlicher RAM-Sample-Klang noch offen.
- Neue Implementierung nicht Windows-abgenommen. Originaler EMU8000 und Treiber unverändert, kein Release, nur main.

## 23.11.2026 — Original-GM-Klangvergleich PASS; VST3-MIDI-Controller angebunden

- **[Windows 45280056019](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45280056019) SUCCESS.** Direkter DOSBox-Mixer und VST3 (gleiche SHA-verifizierte Originaldateien, CC0/PC0/C4 Vel100): gehaltene Note RMS 0,008382091 vs 0,008381980, Ruhe 0,000100778 vs 0,000118569, Release 0,000309144 vs 0,000300079. 90 aktive Notenblöcke, keine Near-Full-Scale-Frames während Note. Direkter Mixer bereits beim Start Peak 1,0; kein DSP-Eingriff begründet. 12.904 EMU8000-Writes, Original-Gate und Negativkontrolle PASS. Keine samplegenaue oder allgemeine Banktreue behauptet.
- Neue produktive Host-Anbindung: standardkonformes IMidiMapping für 16 Kanäle, Modulation, Channel Volume, Pan, Expression, Sustain, CC91/93, Kanalmodus, Channel Pressure und 14-bit Pitch Bend über originalen MPU-401. Kein eigener Controller-DSP, keine künstlichen CC-Defaults beim Boot. Native Windows-Hostprüfung fragt Mapping über echte VST3-Schnittstelle ab. Musikalische Wirkung der Controller und samplegenaue Ereignisreihenfolge noch nicht abgenommen. Neue Windows-Prüfung ausstehend.
- Externe SF2-Auswahl, Recall, vollständige GUI-Regler und Host-Timing bleiben offen. Nur main, `0.0.0-dev`, keine Release-Freigabe.

## 23.11.2026 — Direkter DOSBox-Mixervergleich vor der VST3-Klangabnahme

- [Windows 45276546802](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45276546802): synthetischer Positivtest (1002 nicht-stumme Frames) und strikte Nullsignal-Negativkontrolle PASS. Musikalischer Originaltest FAIL: Ruhe-RMS 0,0761665, gehaltene Note 0,0283831, Release 0,00548934, 100/100 aktive Notenblöcke, 0 Near-Full-Scale-Frames während der Note. Ursache offen; kein Emulationsfehler behauptet. [Linux/Windows 45276546834](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45276546834) und [Versionsprüfung 45276546950](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45276546950) PASS.
- Der Launcher erfasst vor dem VST3-Gate den direkten PCM-Ausgang derselben DOSBox-DLL ohne VST3-Queue: identisches ROM, GM-SF2, Creative-EXE, 44,1 kHz, MIDI CC0=0/PC0/Note60 Vel100/NoteOff. Vier Fenster (Warmup, Ruhe, Note, Release) mit RMS/Peak/Framezahl, vollständige MIDI-Lieferung und regulärer Stop. Keine samplegenaue Gleichheitsbehauptung oder musikalischer PASS aus der Referenzmessung.
- Beide Wege jetzt 800 Warmup-, 80 Ruhe-, 100 Noten- und 250 Release-Blöcke à 512 Samples. Native Host-Taktung absolut bei 44100 Hz statt kumulativer Sleep-Rundung. Musikalische Note/Ruhe-Anforderung unverändert. Neuer Lauf noch ausstehend. Kein DSP-/Treiber-/GUI-Eingriff, nur main, `0.0.0-dev`, kein Release.

## 23.11.2026 — Negativkontrolle vom musikalischen Originaltest getrennt

- [Windows 45275384157](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45275384157) FAIL ausschließlich in der synthetischen Negativkontrolle: gemessene Ruhe-, Note- und Release-RMS sämtlich exakt null. Die neue musikalische Prüfung meldete jedoch einen anderen erwarteten Fehlertext; deshalb wurde der eigentliche Original-Archivtest nicht mehr erreicht. Windows-Build und synthetischer Positivtest (783 nicht-stumme Frames) PASS; [Linux/Windows 45275384197](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45275384197) und [Version 45275384621](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45275384621) SUCCESS.
- Korrektur: eigenständiger nativer `--negative`-Modus mit identischem MIDI-Stimulus wie im synthetischen Positivtest. Prüft alle Frames einschließlich Start, verlangt exakt null Spitzenpegel und null nicht-stumme Frames, keine vorzeitige Erfolgsmeldung. Der echte musikalische Originaltest bleibt unverändert streng. Neuer Windows-Lauf noch nicht bestätigt.
- **Verbindlicher Klangmaßstab:** zuerst VST3 gegen dieselbe originale DOSBox-X-AWE32-Laufzeit mit identischer Bank, MIDI-Sequenz und Mixer-Konfiguration vergleichen. SoundFont-, Creative-Treiber- und Transportfehler voneinander trennen. Der anhand echter AWE32-Hardware kalibrierte Chipkern ([AWETEST-Dokumentation](https://github.com/turican0/AWE32Emu/blob/main/docs/TESTING.md)) bleibt unverändert, bis eine reproduzierbare Abweichung eine gezielte Änderung begründet. Kein Normalisieren oder klangliches Nachregeln auf Verdacht.
- `0.0.0-dev`, nur `main`, P165 und EMU8000 unverändert. Erster Original-PCM-Nachweis 45267065002 bleibt gültig, musikalische Abnahme offen.

## 23.11.2026 — Original-Creative-ROM/SF2 erzeugt erstmals PCM im tatsächlichen Windows-VST3

- **[Windows 45267065002](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45267065002) vollständig SUCCESS**, Original-Archiv-Gate tatsächlich ausgeführt, nicht übersprungen: SHA-verifiziertes AWE-ROM, originale Creative-DEMO/SF2, tatsächliches VST3-PCM: 52.224 Stereo-Frames, 818 nicht-stumm, Peak 0,999969, zwei NoteOn und ein NoteOff gesendet; 12.823 EMU8000-Writes. Separater DLL-Test: 12.902 Writes an sechs Ports, PCM16-Peak 6.539, vier MIDI-Pakete, SDL-Dummy-Isolation und Stop/Join PASS. Positiv-/Negativkontrollen PASS; [Versionsprüfung 45267064799](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45267064799) SUCCESS. Keine privaten Besitzerdateien hochgeladen.
- **Beweisgrenze:** Der erste VST3-Test belegt Original-PCM, jedoch noch keine saubere musikalische Wiedergabe. Der fast volle Peak und die kurze Messzeit verlangen getrennte Messung nach dem Start. Die neue native Originalprüfung wartet 250 Blöcke auf Initialisierung, misst 80 Blöcke Ruhe, spielt einen einzelnen Ton 100 Blöcke und erfasst 250 Blöcke Release. Sie verlangt mindestens acht aktive Note-Blöcke und einen klaren RMS-Anstieg gegenüber Ruhe; sie protokolliert Near-Full-Scale-Frames ohne den Chip oder die Verstärkung zu verändern. Keine Frühfreigabe aufgrund eines Startimpulses mehr. Synthetisches Gate unverändert.
- **Neue musikalische Prüfung noch ausstehend**, kein DAW-/Hardwaretreue-Nachweis, keine SF2-Bankwechsel-Abnahme. `0.0.0-dev`, nur `main`, unveränderter originaler EMU8000 und P165.

## 23.11.2026 — Original-Creative-Archivmaterial im echten Windows-VST3: Forschungsprüfung nach P30

- Nutzerentscheidung: den früheren P30-Prüfweg weiterverwenden, statt die Originalklangprüfung an den Besitzer zu delegieren. P30 hat originales DEMO/SF2-Laden gezeigt, P31 dokumentierte weiterhin stumme bzw. ungültige Notenversuche. Keine Klangfreigabe aus diesen Befunden ableiten.
- Windows-CI führt nach den synthetischen Positiv-/Negativkontrollen einen **isolierten Archiv-Referenztest** mit bereits öffentlich auffindbaren Referenzdateien durch. ROM-Quelle ist der festgeschriebene libretro-pcem-Commit `e07923e902dcedcf65a502fb3f14e75b9ded5271`, ROM-Größe und SHA-256 müssen exakt dem bisherigen Referenzwert entsprechen. Der unveränderte bestehende Originaltest verifiziert Creative-EXE/SF2 und prüft die echte VST3-PCM-Ausgabe plus EMU8000-Trace und separaten DLL-Lauf. Keine privaten Benutzerdateien, keine Veröffentlichung von ROM, EXE, SF2, Audio, Trace oder befülltem Bundle. Temporäre Forschungsdateien werden gelöscht. Diese ausdrücklich abgegrenzte Ausnahme ersetzt die frühere Beschränkung auf rein synthetische CI nur für die öffentlich auffindbaren Archivreferenzen.
- Vorangegangene Windows-Paketvorbereitung [45265939318](../../actions/runs/45265939318) und Versionsprüfung [45265939138](../../actions/runs/45265939138) erfolgreich. **Neuer Original-Archivtest noch nicht abgeschlossen; kein Klang-PASS**, keine Studio-Pro-Abnahme, `0.0.0-dev`, nur `main`. P165 und EMU8000 unverändert.

## 23.11.2026 — Vorbereitete Windows-Testpaketierung ohne private Originaldaten

- Der bestehende Windows-Integrationsworkflow bereitet das tatsächlich kompilierte VST3, den nativen Steinberg-Testhost, die beiden privaten Python-Prüfer und den lokalen PowerShell-Launcher **vor** dem Einbringen synthetischer DOS-Testdateien in einem separaten Verzeichnis vor. Ein expliziter Dateicheck verweigert ROM/SF2/SBK/DEMO.EXE/PRIVATE-Inhalte. Der CMD-Starter verwendet den lokalen Besitzer-ROM und den vorgebauten Testhost, ohne erneuten C++-Build.
- Ein GitHub-Actions-Download wird **nur bei einem privaten Repository** und nach erfolgreichen bestehenden Windows-Gates bereitgestellt. Das aktuelle Repository ist öffentlich; kein öffentlicher Binärdownload, keine Veröffentlichung proprietärer Bestandteile und kein Release. Windows-CI für die neue Paketierung noch nicht bestätigt.
- Der authentische Original-Creative-Klangtest bleibt offen. `0.0.0-dev`, nur `main`, P165 und originaler EMU8000 unverändert.

## 23.11.2026 — Private Original-Creative-DOS-Startdiagnose: Linux-/Windows- und VST3-Tests bestanden

- **Technische Lücke geschlossen:** Die unveränderte originale Creative-`DOS/DEMO/DEMO.EXE` meldet im DOS-Quellcode ausdrücklich `ERROR: SB AWE32 not detected`, `ERROR: SB AWE32 initialization failed`, `ERROR: MPU401 initialization failed`, `ERROR: Cannot load SoundFont file` oder `ERROR: Invalid SoundFont file`. Im bisherigen unsichtbaren DOS-VST3-Gast erschienen diese echten Fehler im privaten Klangtest nur als stiller Output/Timeout, also ohne bekannte Ursache.
- **Nur im privaten Besitzer-Abnahmemodus:** `DosBoxRuntimeHost.hpp` lenkt die **authentischen, nicht veränderten DOS-Programmausgaben** in eine pro Lauf einzigartige, geprüfte **DOS-8.3-Datei** `B???????.LOG` auf dem lokal gemounteten privaten VST3-Gastlaufwerk um. Diese Datei wird per `AWE64_CREATIVE_BOOT_LOG` aus `tools/Run-Private-Creative-AWE-Test.ps1` festgelegt; fehlerhafte Namen werden vor DOSBox-Start abgewiesen. Normaler Produktstart und öffentliches synthetisches MIDI-/Audiotestprogramm behalten ihre vorherige DOS-Startkommandovariante.
- **Privater Windows-Python-Controller:** `WindowsRunVst3HostPcmSmoke.py` überwacht die private Original-Logdatei während des tatsächlichen Windows-VST3-Prozesses; beim originalen `ERROR:` wird der isolierte Prozess beendet und die **wirkliche Creative-Ursache** ausgegeben, statt nur `VST3 silence`. Fehlende oder alte Logs können nicht als bestandener Originalklangtest zählen. Die Logdatei bleibt ausschließlich im **lokalen VST3-Besitzer-Bundle**, nicht im öffentlichen Git oder GitHub-Actions-Output.
- **[Linux-/Windows-Regression 45263904252](../../actions/runs/45263904252): vollständig SUCCESS.** Die neuen Python-Regressionsfälle beweisen mit ausschließlich **generierten Textdateien und kurzlebigen eigenen Kindprozessen**, dass der Original-Creative-DOS-Startfehler sofort gemeldet wird und alte, fehlende oder ungültig benannte Logdateien nicht akzeptiert werden. **[Windows-Original-DOSBox-/VST3-Gesamtintegration 45263904174](../../actions/runs/45263904174) ebenfalls vollständig SUCCESS:** native Windows-VST3-Build erfolgreich, synthetischer echter VST3-Positivtest **62.464 Float32-Stereoframes, 921 nicht-stumme, Peak 0,304871, 8 NoteOn**, unmittelbar folgende echte Creative-only-Negativkontrolle **0 nicht-stumme Audioframes**, zusätzlich originale EMU8000-Register-, MPU401- und DOSBox-Mixer-Prüfungen PASS. [Versions-/Changelog-Konsistenz 45263904207](../../actions/runs/45263904207) SUCCESS. **Diese öffentlichen Tests enthalten weiterhin keine authentische Creative-EXE, kein reales Besitzer-ROM und keine originale SF2-Bank.** Der neue private Diagnoselog-Pfad wurde für den echten Original-Creative-Gast noch NICHT unter Besitzer-Windows ausgeführt, und **Original-Creative-Sound im VST3 ist weiterhin NICHT nachgewiesen**. `0.0.0-dev`, kein Release, nur `main`.

## 23.11.2026 — Echte VST3-Negativkontrolle gegen falschen AWE64-Klang: Windows-PASS

- **Nicht bloß Quelltext:** `tests/WindowsVst3CreativeOnlyNegative.py` startet das **tatsächlich kompilierte Windows-VST3 und die echte originale DOSBox-X-DLL** mit **exakt denselben öffentlichen synthetischen Null-ROM-/DOS-PC-Speaker-Testdateien** wie der positive MIDI→Audio-Gate. Anders als beim positiven Kontrolllauf setzt der isolierte Prozess `AWE64_ORIGINAL_CREATIVE_ONLY=1` und `SDL_AUDIODRIVER=dummy`, sodass der originale DOSBox-X-Gast **PC-Speaker und OPL ausschalten muss**.
- **Harte Negativbedingung:** Der ursprüngliche 16-Bit-DOS-NoteOn-Gast versucht weiterhin, über den echten MPU401 den PC-Speaker anzuspielen. Trotzdem darf am **VST3-PCM-Ausgang kein einziges nicht-stummes Frame** gemessen werden. Nur ein Rückgabefehler mit der **exakten** `process() emitted only silence`-Diagnose und `nonzero=0` zählt – ein Absturz, VST3-Aktivierungsfehler, Timeout oder bloß deaktivierte Testausführung ist **kein PASS**.
- Diese Negativkontrolle ist jetzt Teil der öffentlichen Windows-Original-DOSBox-/VST3-Integration, **direkt nach** der bestehenden positiven Stereoklangprüfung. In öffentlicher CI wird weiterhin **keine originale Creative-`DEMO.EXE`, kein echtes Besitzer-ROM und keine `SYNTHGM.SF2`** ausgeführt. Der Vergleich testet nur, dass synthetischer PC-Speaker-Ton nicht als Original-AWE-EMU8000-Sound ausgegeben werden kann.
- **[Windows-Original-DOSBox-X-/VST3-Lauf 45260688380](../../actions/runs/45260688380) vollständig SUCCESS:** Positivkontrolle mit realem VST3-`process()`: **63.488 Float32-Stereoframes**, **1.011 nicht-stumm**, Spitzenpegel **0,304871**, **8 MIDI-NoteOn**. Direkt danach bestand die echte **Negativkontrolle** mit denselben synthetischen DOS-Dateien und explizitem Creative-only-Modus: **0 nicht-stumme VST3-Stereo-Frames** nach positivem MIDI-Rundweg; Marker `REAL_VST3_PRIVATE_CREATIVE_ONLY_SILENCES_PC_SPEAKER_PASS` tatsächlich im GitHub-Windows-Jobprotokoll. Fehlstarts/Abstürze hätten diesen Negativtest nicht bestanden. Originale EMU8000-Register-/MPU401-/DOSBox-Audio-/DLL-Tests im selben Run ebenfalls PASS. **[Linux-/Windows-Regression 45260688317](../../actions/runs/45260688317) SUCCESS**, [Versionsprüfung 45260688373](../../actions/runs/45260688373) SUCCESS. Öffentliche CI enthält nur selbst erzeugte synthetische Dateien; **echte Creative-`DEMO.EXE`/`SYNTHGM.SF2`/Besitzer-ROM-Klangausgabe im VST3 nach wie vor NICHT nachgewiesen**. `0.0.0-dev`, Release-NO-GO, nur `main`.

## 23.11.2026 — Falsche Creative-Klang-PASS-Möglichkeit im echten VST3-Test geschlossen

- **Produkt-VST3 geändert, originaler Chip NICHT:** `src/DosBoxRuntimeHost.hpp` aktiviert `speaker pcspeaker=false` und `sblaster oplmode=none` jetzt **auch im tatsächlich geladenen VST3**, aber ausschließlich bei bewusst gesetztem privaten Prüfmodus `AWE64_ORIGINAL_CREATIVE_ONLY=1`. PC-Speaker und OPL dürfen beim echten Creative-EMU8000-Klangnachweis nicht als falsche Tonquelle durchgehen. Im **normalen Produkt und öffentlichen synthetischen VST3-Audiotest bleiben diese Geräteinstellungen unverändert**, damit dessen PC-Speaker-MIDI-Rundweg weiter funktionieren kann.
- **Schutz im echten Steinberg-Host:** `WindowsVst3HostPcmSmoke.cpp --original` verweigert die Ausführung, solange `AWE64_ORIGINAL_CREATIVE_ONLY=1` und `SDL_AUDIODRIVER=dummy` nicht beide bestätigt wurden. `tools/Run-Private-Creative-AWE-Test.ps1` setzt diese beiden Prozessvariablen nur für die authentische VST3-Klangprobe, stellt sie selbst bei Fehlern wieder her und führt danach den unabhängigen Original-DLL-Test aus.
- **Datenfreie Regression** `tests/test_synthetic_dos_exe.py` kontrolliert den Unterschied zwischen Original-Creative- und öffentlichem PC-Speaker-Prüfmodus sowie die Environment-Gates. Öffentlich werden weiterhin keine ROM-/SF2-/Creative-Bytes getestet oder veröffentlicht. Bis zur tatsächlichen neuen Windows-CI-Abnahme sind alle hier neu hinzugefügten Gateprüfungen **noch offen**. Echte originale AWE-SoundFont-Klangausgabe im VST3 wurde **noch nicht nachgewiesen**; `0.0.0-dev`, keine Releasefreigabe.

## Korrektur vom 23.11.2026 – Creative-DOS-Bankwahl (neuer Quellstand)

- **Implementiert:** Der VST3-Host sendet Bankwahl jetzt als MIDI **CC 0 (Bank Select MSB) mit dem echten Bankwert** an die unveränderte Creative-DOS-Runtime, statt bisher stets CC 0=0 und CC 32=Bank. Quelle: originales Creative `DOS/DEMO/DEMO.C`, dessen Tastatursteuerung Bankwahl ausdrücklich mit `awe32Controller(15, 0, k)` ausführt.
- **Neue Regression:** `CreativeMidiBankSelectTest.cpp` prüft Bankwerte 0–127, Kanalgrenzen, MIDI-Status, Bank-Controller und Program-Change-Paket. Der korrigierte C++20-Helper und die vollständigen Bank-/Programm-Regressionsprüfungen sind in Linux und Windows tatsächlich bestanden: [Actions 45175793161](../../actions/runs/45175793161).
- **Keine Klangbehauptung:** Originale Creative-DEMO/ROM/SF2 innerhalb des neuen VST3 sowie korrektes Bank-/Program-Recall sind weiterhin **nicht** end-to-end nachgewiesen; Entwicklungskennung bleibt `0.0.0-dev`.

## Regressionstests – 23.11.2026

Der neue reine MIDI-Bankwahltest prüft 16 Kanäle × 128 Banken und 16 Kanäle × 128 Programme (4096 Eingabekombinationen). Er verwendet bewusst keine C-Standard-`assert`-Makros, da diese in Windows-/Linux-CMake-`Release`-Builds mit `NDEBUG` entfallen. Die Tests sind mit vollständigem Linux-/Windows-Release-Gate erfolgreich: [Actions 45175793161](../../actions/runs/45175793161). Der zuvor bereits ausgeführte Linux-/Windows-CI-Lauf hatte die Konstanten durch `static_assert` geprüft; die 128er Runtime-Schleife war unter `NDEBUG` inaktiv.

## Beendete Windows-/Linux-Gates – 23.11.2026

- **[45175109078](../../actions/runs/45175109078) vollständig erfolgreich:** Windows-x64-Original-DOSBox-X-SDL2-DLL, Shell-Start und Rückkehr, DOSBox-Worker-Stop/Join, reale MIDI-Bytes bis in einen 16-Bit-DOS-Gast, originales Mixer-PCM und gemeinsamer Windows-VST3-Bundle-Build. Kein Original-Creative-Gast-Klang-Nachweis.
- **[45177530826](../../actions/runs/45177530826) vollständig erfolgreich:** Linux- und Windows-Regressionen einschließlich synthetischer Privat-Klangtest-Harness-, Git-Blob-/SHA-, Original-Konfigurations-, MIDI- und EMU-Trace-Prüfungen. Keine Originaldateien auf Actions. **Original-Creative-Klang im VST3 weiterhin offen.**
- **[45177530845](../../actions/runs/45177530845) erfolgreich:** Versions-/Changelog-Konsistenz; Dokumentation zu den nun abgeschlossenen Integrationsläufen präzisiert. `0.0.0-dev` unverändert, kein Release.

## Original-Creative-Klang-Gate – private Windows-Prüfung vorbereitet

- `tests/WindowsPrivateCreativeGuestSmoke.py` führt ausschließlich in einem **privaten Windows-Testprozess** die tatsächlich im VST3-Bundle liegende originale DOSBox-X-DLL aus. Es prüft exakt die Originaldateien per SHA-/Git-Blob, startet `DEMO.EXE /SBK:SYNTHGM.SF2`, sendet CC0/Programm/Note über den echten MPU-401, misst Original-Mixer-PCM und verlangt EMU8000-Portschreibspuren, SDL2-Dummy-Isolation und kontrolliertes Beenden.
- Der Test erzeugt **keinen** Ton und keine Ersatz-EMU8000-Engine. Trace/PCM/Originaldateien bleiben privat; Child-Prozess mit hartem Timeout schützt gegen Hänger. Öffentliche CI führt **nur synthetische Quell-/Harness-Regressionstests** aus.
- **Wichtig:** Der private Originalklang-Gate-Test wurde hier mangels lokal ausgeführter Windows-AWE-Laufzeit **noch nicht erfolgreich ausgeführt**. Sein Vorhandensein ist weder echter Klang-PASS noch ein VST3-Host- oder Fender-Studio-Pro-Test. Erst die entsprechenden echten Befunde können eine Freigabe rechtfertigen.

### Eingebaute Prüfungssicherung
Die neue private Windows-Prüfung enthält zusätzlich überprüfte Original-Git-Blob-/SHA-Hashberechnung, korrekte binäre MIDI-Testbytes und Original-EMU-Porttrace-Erkennung. Sie wird öffentlich nur mit synthetischen Testdaten validiert, nicht als Klangerfolg gewertet.

Die synthetischen Tests dieses privaten Originalklang-Testers sind jetzt in den Linux-/Windows-Transportworkflow eingebunden. Ein echter Original-Creative-Klangnachweis ist davon strikt getrennt.

### Windows-Actions-Bezeichnung nach tatsächlich bestätigtem VST3-PCM korrigiert

Der Workflow `.github/workflows/windows-dosbox-runtime.yml` hieß historisch `(NO VST3 PLAYBACK)`. Weil [45201802002](../../actions/runs/45201802002) inzwischen **echte MIDI- und Audio-`process()`-Ausgabe im geladenen VST3** nachweist, ist diese alte Bezeichnung objektiv überholt. Korrigiert zu **`DOSBox-X original + VST3 MIDI/PCM end-to-end (synthetic DOS)`**. **Kein Beleg für Creative-EMU8000-Originalton**; kein Produktcode geändert und kein Release.

## 23.11.2026 — Lokales Original-Creative-Build-/Prüfverfahren: CI-Quell- und Syntaxabnahme bestanden

- [Linux-/Windows-Transport-/Contract-Lauf 45215251617](../../actions/runs/45215251617) **vollständig SUCCESS**: echte synthetische C++-Regressionssuite, Python-Checks des **Besitzer-only Windows-Repro-Builders** und **PowerShell-AST-Parserprüfung beider privaten Original-Creative-Skripte** unter Windows bestanden. Der vorherige rein textuelle `SHA-256`/PowerShell-`SHA256`-Testfehler ist damit abgearbeitet.
- [Versions-/Changelog-Konsistenz 45215251656](../../actions/runs/45215251656) **SUCCESS**. **Einziger Branch `main`**. Beide private Besitzer-Skripte bleiben lokal; keine Original-ROM-, `DEMO.EXE`-, `SYNTHGM.SF2`-Bytes wurden in öffentlicher CI verwendet oder hochgeladen.
- **Beweisgrenze unverändert:** Ein Syntax-PASS und datenfreie Regression beweisen **keinen** realen originalen Creative-EMU8000-Sound. Das neue `Build-And-Test-Private-AWE64.ps1` ist zwar einsatzbereit geprüft, aber **nicht auf dem Besitzer-Windows-PC mit authentischen Dateien ausgeführt**. Weiterhin `0.0.0-dev`, Release-NO-GO, Fender-Studio-Pro-Endabnahme offen.

## 23.11.2026 — Neue statische Testbehauptung anhand realer PowerShell-Syntax korrigiert

- [Linux-/Windows-Regression 45214831987](../../actions/runs/45214831987) **FAIL** im neu hinzugefügten **Text-Contract-Test**, nicht im DOSBox-/VST3-Code: Der Test verlangte die wörtliche Zeichenfolge `SHA-256`, obwohl das PowerShell-Skript korrekt `Get-FileHash -Algorithm SHA256 -LiteralPath $romPath` ausführt. Die Negativkontrolle prüft jetzt den **exakten tatsächlichen SHA256-Prüfaufruf** statt eines Kommentars. Der private Windows-Build/Creative-Klangtest wurde auch durch den fehlgeschlagenen Test **nicht ausgeführt**.
- Der restliche Source-Code, Original-P165, originaler AWE32-Kern und `main` sind unverändert. Erst nach den erneut beendeten Linux-/Windows-Prüfläufen darf dieser Fix als PASS markiert werden. **`0.0.0-dev`, kein Release.**

## 23.11.2026 — Original-Creative-Klangprüfung als lokaler Windows-Gesamtlauf vorbereitet (KEIN Klang-PASS)

- **Neuer Besitzer-only Komplettablauf:** `tools/Build-And-Test-Private-AWE64.ps1` baut auf dem Besitzer-Windows-PC den gepinnten **unveränderten DOSBox-X-AWE32-Kern** (`f3a95b9`) als eigene x64-SDL2-DLL, den exakten **Steinberg-VST3-Host 3.7.12** und die zugehörige reale `IAudioProcessor`-Testhost-EXE. Die unveränderte P165-Oberfläche wird über die verpflichtende Artwork-Git-Blob-Prüfung in CMake erzwungen.
- **Wichtige Reproduzierbarkeit/Ordnung:** Es wird **ausschließlich ein lokaler Checkout unter dem gitignorierten `PRIVATE/working-upstream/dosbox-x-AWE32` gepatcht und gebaut**. Originales Submodul, `main`, P165-GUI, Original-EMU8000-Code und gesamte andere Repository-Historie bleiben erhalten; kein neuer Branch, kein automatisches `git reset/clean`, keine Originalbytes in GitHub.
- Nach erfolgreichem Build führt der Runner `tools/Run-Private-Creative-AWE-Test.ps1` aus. Dieser validiert das vorhandene echte ROM per SHA-256, die archivierte Original-`DEMO.EXE` per Git-Blob und `SYNTHGM.SF2` per SHA-256 und startet **den echten VST3-Host mit NoteOn/NoteOff + EMU8000-Porttrace + nicht-stummem PCM**, anschließend einen unabhängigen Original-Creative-DOSBox-DLL-Test. Ein `PASS` darf **erst nach allen tatsächlich ausgeführten Gating-Schritten** ausgegeben werden.
- Datenfreie Python-Regression und Windows-PowerShell-**Syntax**prüfung wurden in den kurzen Linux-/Windows-Testworkflow aufgenommen. **Dieser reale private Windows-Originalklang-Lauf ist in dieser Sitzung mangels Zugriff auf einen Besitzer-Windows-Host nicht ausgeführt worden.** Die bisherigen erfolgreich ausgeführten GitHub-Tests verwenden weiterhin ausschließlich synthetische DOS-Signale; sie beweisen keinen Creative-SF2-Klang. **`0.0.0-dev`, kein Release, kein Fender-Studio-Pro-Endtest.**

## 23.11.2026 — Original-Creative-DLL-Klang-Gate gegen Fremdklang und falsche Registerzählung gehärtet

- **Originalquellcode geprüft:** Das gepinnte DOSBox-X besitzt `speaker pcspeaker=false` und `sblaster oplmode=none` als offizielle Konfigurationsoptionen. Im separaten **privaten Original-Creative-DLL-Audiotest** sind diese beiden anderen Klangquellen jetzt ausgeschaltet; unverändert aktiv bleiben `sblaster sbtype=sbawe`, Original-EMU8000 und `midi mpu401=intelligent`. Ein Bestehen kann damit nicht mehr durch emulierte PC-Speaker- oder OPL-Töne erklärt werden. Produktstart und öffentliches PC-Speaker-Transportgate bleiben bewusst unverändert.
- Der private `EMU8K_TRACE`-Parser zählt jetzt **ausschließlich Original-EMU8000-I/O-Schreibzugriffe** an 620h/622h/A20h/A22h/E20h/E22h (keine Reads, MPU, OPL oder andere Ports), und die private Abnahme fordert **mindestens 16 reale Writes auf mindestens 2 unterschiedlichen EMU8000-Ports**. Bei fehlendem Trace ist das Ergebnis ausdrücklich `(0,0)`, also FAIL.
- **Datenfreie Regression** `tests/test_private_creative_guest.py` verifiziert beide Hardware-Stummschaltungen sowie echte/unechte Porttraces unter Linux und Windows. **Ein erfolgreicher synthetischer Test ersetzt weiterhin nicht die noch ausstehende private Windows-Klangprobe mit authentischer `DEMO.EXE`, `SYNTHGM.SF2` und `awe32.raw`.** Nur `main`, `0.0.0-dev`, keine Änderung am originalen ROM/EMU8000/DSP oder P165-GUI.

## 23.11.2026 — Privaten Original-Creative-VST3-Hostmodus unter Windows erfolgreich kompiliert; Public-CI-PASS

- **[Windows 45205416871](../../actions/runs/45205416871) vollständig SUCCESS**: Das wirklich gebaute VST3 und das um einen **privaten `--original`-Modus** erweiterte Windows-SteInberg-Hosttestprogramm wurden im Release-Modus erfolgreich kompiliert; die öffentliche synthetische MIDI→Original-DOSBox-MPU→PC-Speaker→VST3-`process()`-Regression bleibt vollständig grün. Im Host **60.928 Float32-Stereoframes**, davon **979 nicht-stumm**, Peak **0,304871**, **7 NoteOn**. Public-AWE32-Original-EMU8000-`AWEUTIL`-Registerwrites, MPU401, PCM und sauberer Stop weiter PASS.
- **[Linux-/Windows-Transportregression 45205416646](../../actions/runs/45205416646) SUCCESS**; neuer datenfreier Sicherheits-/Funktionsquelltest sichert den separaten `--original`-Modus, echte NoteOff-Eventbehandlung, getrennten Owner-ROM-/SF2-/EXE-Hash-/EMU-Trace-Prüfweg und verbietet die Ausführung privater Originalbytes in der öffentlichen CI. **[Version/Changelog 45205416611](../../actions/runs/45205416611) SUCCESS.** Der private Windows-Launcher meldet einen Gesamt-PASS **erst nach** echtem VST3-Audio, Original-EMU8000-Trace und zusätzlichem unabhängigen Creative-DLL-Klangtest.
- **Keine falsche Freigabe:** Alle nachgewiesenen Windows-PCM-Werte stammen weiterhin aus dem **synthetischen DOS-PC-Speaker-Gast**. Die separate private Ausführung von Original-Creative-`DEMO.EXE`, echten `awe32.raw` und `SYNTHGM.SF2` **hat noch nicht stattgefunden**. `0.0.0-dev`, Original-P165-GUI und gepinnter EMU8000-Kern unverändert; nur `main`.

## 23.11.2026 — Echten Creative-DOS-/ROM-/SF2-Klang über tatsächliche VST3-Audioschnittstelle privat prüfen

- **Neues owner-only Originalklang-VST3-Gate:** Der bereits mit echten Steinberg-Audioschnittstellen geprüfte native Windows-Host `tests/WindowsVst3HostPcmSmoke.cpp` bietet jetzt zusätzlich `--original 15..120`. Dieses Flag wird **niemals** in der öffentlichen GitHub-CI verwendet. Es lädt genau das echte `AWE64GoldDOSBoxDev.vst3` mit originaler DOSBox-X-AWE32-DLL und den ausschließlich lokal verifizierten Besitzerdateien `awe32.raw`, Creative-`DEMO.EXE` und `SYNTHGM.SF2`.
- Im privaten Modus sendet der echte VST3-Host wiederholte **Note-On- und Note-Off-Ereignisse** über Steinberg-`IEventList` (C4, Velocity 100), kontrolliert Stille **vor** dem ersten Note-On und verlangt nicht-stumme Stereo-`process()`-Audioframes mit konservativem Peak-Gate 0,002. Der Original-Creative-Gast erhält Zeit zum Initialisieren der Soundbank; der Test läuft in einem isolierten Kindprozess mit Zeitgrenze.
- `tools/Run-Private-Creative-AWE-Test.ps1` stellt den vollständig privaten Prüfweg bereit: echte Originaldateien mit SHA-256/Git-Blob verifizieren und nicht überschreiben, `--original`-VST3-Host ausführen, **tatsächliche EMU8000-Registerwrites an mehreren Originalhardwareports** aus dessen lokalem Trace verlangen und danach eine zweite unabhängige Original-Creative-DOSBox-DLL-Prüfung ausführen. Öffentliche CI kontrolliert ausschließlich Quellcode, PS-Syntax und synthetische Daten. **Niemals Original-ROM, Creative-Binary, SF2 oder Klangtrace in das öffentliche Repository / Actions hochladen.**
- **WICHTIG: Dieser neue echte Original-Creative-VST3-Klangtest ist bis zur privaten Ausführung auf Windows NICHT bestanden.** Synthetische NoteOn→PC-Speaker→VST3-PCM-PASS [45201802002](../../actions/runs/45201802002) bleibt unabhängig gültig, darf aber nicht als Creative-SF2-Klangnachweis verkauft werden. Keine Fender-Studio-Pro-Abnahme, `0.0.0-dev`, weiterhin kein Release; Original-P165-GUI und unveränderter Original-EMU8000-Kern erhalten.

## 23.11.2026 — Vollständiger VST3-MIDI→DOS-MPU→Hardware-Audio→VST3-PCM-Rundweg: PASS

- **[Windows-Actions 45201802002](../../actions/runs/45201802002): vollständig SUCCESS**. Der echte Windows-VST3-Host (`GetPluginFactory`, `IComponent`, `IAudioProcessor`) sendete durch `IEventList` **7 echte NoteOn-Events** bei wiederholten `process()`-Aufrufen (Kanal 0, Ton 60, Velocity 100).
- Das eigenständig kompilierte **16-Bit-DOS-MZ-Testprogramm** prüft den realen emulierten MPU-401 (UART-Befehl 3Fh, ACK 0FEh; I/O-Ports 331h/330h) und schaltet die echte DOSBox-X-PIT-/PC-Speaker-Hardware (43h/42h/61h) **ausschließlich nach** passender MIDI-Note ein. **Vor dem ersten MIDI-NoteOn muss der VST3-Host Stille messen.** Damit ist der bidirektionale Audio/MIDI-Pfad in **einem echten Pluginprozess** nachgewiesen, nicht bloß getrennte Unit-Tests.
- Der Host erhielt **59.904 Float32-Stereoframes**, davon **982 nicht-stumm**, maximaler Float32-Peak **0,304871**, ohne NaN/Inf/Überlauf; nachfolgend VST3-Stop, Release und DOSBox-Worker-Beenden bestanden. Das CI-Log enthält `SYNTHETIC_DOS_FIXTURE_REAL_VST3_HOST_PCM_PASS` und `VST3_HOST_NOTE_ON_EVENTS_SENT=7`.
- Im selben Windows-Lauf erneut originale DOSBox-X-DLL, `AWEUTIL /S` mit **9.688 echten EMU8000-Registerwrites**, originales 16-Bit-MPU401-Gast-Smoke, Mixer-Nicht-Stille und echter VST3-Build **PASS**. [Datenfreie Linux-/Windows-Release-Regressionsprüfung 45201802160](../../actions/runs/45201802160) **SUCCESS**, [Dokumentationsprüfung 45201801967](../../actions/runs/45201801967) **SUCCESS**. Originaler Quellstand und P165-GUI unverändert; nur `main`.
- **Strikte Abgrenzung:** Trotz realem MIDI↔Audio-Rundweg erklingt ein **eigenes DOS-PC-Speaker-Testprogramm**, NICHT der Creative-`DEMO.EXE`-/EMU8000-SoundFont. Im öffentlichen Test sind Wave-ROM (Nullen) und `SYNTHGM.SF2` (Fake-Text) **keine** Creative-Originaldateien. Der private Klangtest mit originaler Creative-DOS-`DEMO.EXE`, echtem ROM und echter `SYNTHGM.SF2` steht noch aus. DAW-/Preset-/GUI-/Projekt-Recall-Abnahme offen, **`0.0.0-dev`**, kein Release.

## 23.11.2026 — Erster echter Windows-VST3-Host-`process()`-Klangnachweis: PASS

- **[Windows-Original-DOSBox-X-/VST3-Integrationslauf 45197768158](../../actions/runs/45197768158): vollständig SUCCESS.** Die echte kompilierte `AWE64GoldDOSBoxDev.vst3` wurde über die reale Steinberg-`GetPluginFactory`, `IComponent`, `IAudioProcessor`, `setActive`, `setProcessing` und den wiederholt aufgerufenen originalen `process()`-Callback geladen, gestartet, mit Ton betrieben, deaktiviert und freigegeben.
- Die echte DOSBox-X-AWE32-SDL2-DLL erzeugte über echte emulierte PIT-/PC-Speaker-DOS-Ports in einer **eigenen, nicht-proprietären DOS-Test-EXE** originale Mixer-Samples und leitete sie nachweislich durch die **VST3-Prozessor-Audio-Ausgänge**. Gemessen im Host: **80.384 Float32-Stereoframes**, darunter **568 nicht-stumme Frames**, **Float32-Peak 0,304871**; keine geklippte oder nicht-endliche Probe. CI-Kennzeichnung `SYNTHETIC_DOS_FIXTURE_REAL_VST3_HOST_PCM_PASS`.
- Im selben beendeten Windows-Lauf erneut **9.688 echte EMU8000-Registerwrites** (`AWEUTIL /S`), originale MPU-401-MIDI-Gasteinspeisung, separates DOSBox-Mixer-PCM und sauberer DOSBox-Worker-Stop/Join **PASS**. Die Windows-DOSBox-DLL wurde erfolgreich aus Originalquellen gebaut und der gesamte VST3-Host-Build bestanden. Separates GitHub-Actions-Cache für **nur quelloffene DOSBox-DLL ohne Owner-Dateien** erfolgreich geschrieben.
- **[Audio-Bridge-Regressionen 45197768094](../../actions/runs/45197768094): Linux und Windows SUCCESS**; [Versions-/Changelog-Prüfung 45197768039](../../actions/runs/45197768039): SUCCESS.
- **Strikte Beweisgrenze:** Die PC-Speaker-EXE war eine **selbst erzeugte synthetische DOS-EXE**, `awe32.raw` enthielt absichtlich **nur Nullen**, `SYNTHGM.SF2` einen **Fake-Textmarker**. Die echte Creative-`DOS/DEMO/DEMO.EXE` + authentisches Besitzer-ROM + die echte `SYNTHGM.SF2` wurden **nicht** im aktuellen VST3 gespielt. Original-EMU8000-SoundFont-Klang, Benutzer-Preset-/GUI-Bedienung, Fidelity, DAW-Projektrecall und Fender-Studio-Pro-Endabnahme weiter **OFFEN**; `0.0.0-dev` und Release-**NO-GO**.

## Windows-VST3-Host-Praxisgate: PowerShell-Automatikfehler nach Original-Build behoben

- [Windows-Integrationslauf 45195522027](../../actions/runs/45195522027) **FAIL NACH erfolgreich gebauten echten VST3-Plugin und C++-Hosttestprogramm**; der Gate-Schritt erreichte den tatsächlichen Instrumenten-`process()`-Aufruf **noch nicht**. Ursache war die Zuweisung `$host='...'` in GitHub Actions: PowerShells **`$Host` ist eine schreibgeschützte automatische Variable** (case-insensitive). Deshalb endete der Workflow unmittelbar beim Vorbereiten des Testaufrufs.
- Auf `$hostTestExe` umgestellt und Regression in `test_synthetic_dos_exe.py` eingebaut, die `$host =` künftig unter Linux und Windows abweist.
- Um die wiederholten vollständigen 6-Minuten-Builds des unveränderten Quellkerns zu vermeiden, nutzt GitHub Actions jetzt `actions/cache/restore@v4`/`actions/cache/save@v4` **ausschließlich für die aus öffentlichem DOSBox-X-Quellcode erzeugte interne DLL**. Cache-Schlüssel bindet gepinnten Submodulstand und tatsächlich verwendetes Quellpatchskript ein; keine ROMs, Creative-Programme oder SF2 werden gecacht. Der neue Cache darf nur nach ausgeführtem Lauf als funktionierend gemeldet werden.
- Im genannten Windows-Lauf: originaler EMU8000-`AWEUTIL /S`-Registertest, Original-MIDI-Gast, eigener PC-Speaker-Mixer/PCM-Test und tatsächlicher Windows-x64-VST3-Host-/Runtime-Build **alle PASS**. Die Linux-/Windows-Datenfrei-Tests sind bereits [45195522141](../../actions/runs/45195522141) PASS.
- **Originale Creative-`DEMO.EXE` mit Besitzer-ROM und `SYNTHGM.SF2` bleibt PRIVAT und UNGETESTET im aktuellen VST3**. Kein Release, Entwicklungskennung `0.0.0-dev`.

## Historischer Windows-VST3-Hosttest: Compilerfehler im vorigen Quellstand behoben

- **[Windows-Original-AWE-Build 45193354491](../../actions/runs/45193354491): FAIL** in neuem *eigenem* Testhost `WindowsVst3HostPcmSmoke.cpp`, **nicht** in originaler DOSBox-X-AWE32-Hardware: Microsoft Visual C++ meldete `Steinberg::TChar` als nicht existent sowie `std::max`-Parserfehler durch `windows.h`-Makros `min/max`. Die vorangehenden originalen DOSBox-X-DLL-, EMU8000-, MIDI- und PC-Speaker-PCM-Prüfungen im selben Windows-Lauf waren alle PASS.
- Der kleine Testhost verwendet jetzt korrekt **`Steinberg::Vst::TChar`** sowie Windows-Makro **`NOMINMAX`** vor `windows.h`. Echte Produkt-DSP-/Creative-/EMU8000-Codepfade und originale P165-GUI bleiben unverändert. Bis zum erneut komplett beendeten Windows-Lauf darf der neue VST3-Host-PCM-Gate nicht als PASS gelten.
- **[45193355496](../../actions/runs/45193355496): Linux- und Windows-Unit-Regressionen PASS**; [45193355170](../../actions/runs/45193355170): Versionskonsistenz PASS.
- Damit der finale Klangtest tatsächlich mit **privat beschafften Originalbytes** und ohne Veröffentlichung möglich ist, steht `tools/Run-Private-Creative-AWE-Test.ps1` bereit. Das Skript validiert privat das bereits vorhandene Besitzer-ROM per SHA-256, liest die beiden originalen Creative-`DOS/DEMO`-Dateien nur lokal aus dem veröffentlichten `qb40/sound-blaster-awe32`-Archiv, prüft exakte Checksummen, überschreibt niemals bereits vorhandene Besitzerdateien und startet `tests/WindowsPrivateCreativeGuestSmoke.py`. **Dieser echte Originalklang-Test wurde mangels Windows-Host mit Owner-Dateien hier noch nicht ausgeführt.** Keine Daten in öffentlichen GitHub-Actions oder Git, weiterhin `0.0.0-dev`.

## Historische Einführung: Instrumenten-Host-Test (Nachweis im aktuellen Abschnitt oben)

- `tests/WindowsVst3HostPcmSmoke.cpp` lädt unter Windows die **tatsächlich gebaute** `AWE64GoldDOSBoxDev.vst3`-Bibliothek und führt über die **echte Steinberg-IPluginFactory**, `IComponent`, `IAudioProcessor`, `setActive`, `setProcessing` und wiederholte `process()`-Aufrufe den vollständigen Windows-VST3-Audiohostpfad aus. Er verlangt messbare PCM-Frames, begrenzte Amplitude und reguläre Deaktivierung.
- **Klare Originaldaten-Grenze:** In öffentlicher GitHub-CI wird ausschließlich eine **synthetisch aus eigenem Code erzeugte MZ-DOS-EXE** namens `DEMO.EXE` in einem temporären Testbundle verwendet. Sie spielt per echten emulierten PIT-/PC-Speaker-Ports 43h/42h/61h – **keine** Creative-`DEMO.EXE`, Creative-SF2, User-ROM oder emulierter SoundFont. Die ursprüngliche DOSBox-X-DLL und das VST3 selbst bleiben echt. `MakeSyntheticDemoExe.py` verweigert Überschreiben schon vorhandener Originaldateien.
- Das tatsächliche **Original-Creative-Klang-Gate** bleibt ein getrenntes ausschließlich privates Verfahren mit der echten Creative-`DEMO.EXE`/`SYNTHGM.SF2` und dem echten Wave-ROM. Die öffentliche Synthetic-PCM-Prüfung darf diesen Klang **niemals** als bewiesen melden.
- Neue datenfreie Linux-/Windows-Regressionsprüfungen validieren DOS-EXE-MZ-Header, Hardware-Port-Instruktionen und Fixture-Schutz. **Ergebnisse dieser neuen VST3-Hostintegration vor beendetem Windows-Actions-Lauf ausdrücklich offen.** Entwicklungskennung `0.0.0-dev`, kein Release.

## 23.11.2026 — Nicht-stummer DOSBox-Mixer/PCM-Originalpfad tatsächlich PASS

- **[Windows-Original-DOSBox-X-Lauf 45189311093](../../actions/runs/45189311093): vollständig SUCCESS**. Im Gast lief die eigens erzeugte 52-Byte-16-Bit-`SPEAKER.COM`; sie spielte über **Original-PIT-/PC-Speaker-Ports 43h/42h/61h**, nicht über einen vom Testhost erzeugten Ton.
- Der **originale DOSBox-X-Mixer** leitete dabei **68.608 Stereoframes** an das ursprüngliche Host-PCM-Callback weiter, davon **17.633 tatsächlich nicht-stumm**, 16-Bit-Spitzenpegel **9.990**. Internes SDL2-Audiogerät nachweislich **dummy**, DOS-Gast exit 0; Original-EMU8000-AWEUTIL-Registerprüfung (9.688 Writes) und echter MPU-401-MIDI-Gast-Test ebenfalls bestanden.
- **Original-DOSBox-X-Windows-x64-DLL und VST3-Host** mit originaler P165-Oberfläche wurden **im selben erfolgreichen Windows-Lauf** kompiliert und in ein VST3-Bundle zusammengefügt; dessen tatsächliches Spielen innerhalb eines VST3-Hosts/DAW wurde dadurch **nicht** nachgewiesen.
- Die datenfreien regressiven Python-/C++-Tests sind auf Linux und Windows im Release-Modus **[45189310952](../../actions/runs/45189310952) erfolgreich**; die Versionsprüfung **[45189310913](../../actions/runs/45189310913) erfolgreich**.
- **Abgrenzung:** Der hörbare Signalursprung war **PC-Speaker-Hardware**, nicht der EMU8000. Das 1-MiB-ROM war synthetisch mit Nullen gefüllt. **Original-Creative-`DEMO.EXE`, echtes `awe32.raw`, `SYNTHGM.SF2` und deren hörbarer AWE-Klang im VST3 bleiben unbewiesen.** Keine Schutzdaten in öffentlicher CI; `0.0.0-dev` und Release-**NO-GO**.

## Originalhardware erstmals nachweislich aktiv – Windows-PASS 23.11.2026

- **[Windows-Integrationslauf 45186047184](../../actions/runs/45186047184): vollständig SUCCESS.** Original-DOSBox-X-AWE32-SDL2-DLL, echter DOSBox-Start, interne SDL2-Isolation, kontrollierter Gaststopp, originaler MPU-401-MIDI-Nachweis und vollständiger Windows-VST3-Bundle-Build mit unveränderter P165-Oberfläche erfolgreich.
- **9.688 reale EMU8000-Hardware-Registerschreibzugriffe** in der Original-DOSBox-X-Kernemulation durch deren unverändertes DOS-`AWEUTIL /S` im neuen Hardware-Smoketest nachgewiesen (`ORIGINAL_AWE32_HARDWARE_WRITE_GATE_PASS`). Die Hardware wurde mit einem **synthetisch mit Nullen gefüllten 1-MiB-ROM** gestartet; keine geschützten ROM-/Creative-/SF2-Dateien in öffentlicher CI.
- **Ursache und Korrektur:** Im unveränderten Upstream setzt `control->opt_silent` den Sound Blaster auf `SBT_NONE`. Die Entfernung von `-silent` aus der echten VST3-Startliste und beiden AWE-Testern hat die reale Hardwareinitialisierung ermöglicht.
- **[Linux- und Windows-Transport-/Negativregressionen 45186047103](../../actions/runs/45186047103): SUCCESS**; `-silent` kann nicht unbemerkt in die Produkt- oder AWE-Test-Startlisten zurückkehren. [Versionskonsistenz 45186047049](../../actions/runs/45186047049): SUCCESS.
- **Weiter offen:** Originale Creative `DEMO.EXE` + echte `awe32.raw` + `SYNTHGM.SF2` innerhalb des VST3 **nicht end-to-end auf echten hörbaren Klang geprüft**. Keine SF2-Preset-/GUI-/State-/44,1–96-kHz-/Mehrfachinstanzen-Freigabe; `0.0.0-dev`.

## Blockierender Fehler im AWE64-DOSBox-VST3-Start behoben – 23.11.2026

- **Fehler anhand unveränderten Upstreams belegt:** `sblaster.cpp` im gepinnten `turican0/dosbox-x-AWE32` setzt mit `if (control->opt_silent) type = SBT_NONE;` die gesamte Sound-Blaster-Karte außer Betrieb. Das produktive VST3 und beide Original-AWE-Testharnesses verwendeten jedoch fälschlich **`-silent`**.
- **Konkreter Fehlbefund:** [Originalhardware-Windows-Lauf 45184081242](../../actions/runs/45184081242) endete **FAIL** mit fehlender `EMU8K_TRACE`-Datei. Das vorherige erfolgreiche Starten des DOSBox-Main und des MPU-401-Bus konnte wegen `-silent` niemals einen Sound-Blaster- oder EMU8000-Klangtest ersetzen.
- **Korrektur:** `-silent` aus `DosBoxRuntimeHost.hpp`, `WindowsAweHardwareSmoke.py` und `WindowsPrivateCreativeGuestSmoke.py` entfernt. Die Oberflächenflags `-nogui -nomenu` bleiben gesetzt. Originaler EMU8000-Code, Creative-Software und P165-GUI unverändert.
- **Rückfallsperre:** synthetische Linux-/Windows-Regressionen prüfen, dass der echte VST3-Start und beide AWE-Tests `-silent` nicht erneut übergeben.
- **Nachweisgrenze:** Dieser Commit repariert die Ursache; erst der **neue beendete Windows-Hardwaretest** darf EMU8000-Portschreibvorgänge als bestanden bestätigen. Original-Creative-ROM-/SF2-Klang innerhalb des VST3 bleibt unbewiesen, Entwicklungskennung `0.0.0-dev`, keine Freigabe.

## Original-EMU8000-AWE32-Hardware-Gate – aktuelle Prüfphase

- Windows-DLL-Integration testet zusätzlich mit einem **synthetisch leeren 1-MiB-ROM** den realen `sblaster sbtype=sbawe`-Start (nicht bloß `sb16`), den originalen DOSBox-X-Befehl `AWEUTIL /S`, seine tatsächlichen EMU8000-Registerwrites an 620h/A20h/E20h und die Rückkehr zum Host. Ein hartes Windows-Prozess-Timeout schützt gegen Hänger.
- Öffentliche Linux/Windows-Tests prüfen nur den Argumentpfad und originalen Trace-Parser ohne ROM-/Creative-Lizenzdateien; **der tatsächliche Windows-AWE32-Integrationserfolg bleibt bis zum beendeten Windows-Actions-Lauf unbestätigt**. Synthetisches ROM erzeugt ausdrücklich keinen Originalklang-Nachweis. `0.0.0-dev` und Original-GUI bleiben unverändert.

Die ursprünglichen zwei Einführungs-Quelltests enthielten zu stark maskierte Python-Regex-/String-Newline-Literale. Diese wurden vor dem Windows-EMU8K-Hardwaretest korrigiert, damit Trace-Records tatsächlich erkannt werden und der portable Test echte mehrzeilige Originaltraces nachstellt. **CI-Nachweis erst nach Abschluss.**

- **Korrektur:** Ein plattformübergreifender Original-ROM-Pfadtest verwendete fest vorgegebene POSIX-Schrägstriche, obwohl Windows `pathlib.Path` korrekterweise Backslashes liefert. Der Test prüft nun die jeweils tatsächlich generierte Windows-/Linux-Pfadangabe; der Emulator-Code blieb unverändert. Vor bestandener Windows-CI keine PASS-Aussage.

## Festgelegte Architektur

Ein einziges Windows-VST3-Paket mit Original-P165-GUI, originalem DOSBox-X-AWE32/EMU8000-Quellcode als **intern geladener DLL im VST3-Prozess**, originaler Creative-DOS-SF2LIB/DEMO.EXE als 16-Bit-DOS-Programm im emulierten PC. **Kein** externes DOSBox-Programm, **kein** 86Box-/Windows95-Gast und **kein** nativer Ersatz-SF2-Player.

- Upstream-DOSBox-X-Fork: `turican0/dosbox-x-AWE32`, `masterAWE32`, fest auf **`f3a95b9f19a7f9863295e6481f271cf04733c635`** (30.09.2026). Letztes AWE32-Release: `2026.08.31-AWE32-0.92`. Neuere offizielle DOSBox-X-Versionen sind **nicht** automatisch kompatibel mit dem AWE32-Fork.
- Steinberg VST3 SDK: `v3.7.12_build_20`.
- GUI: **Original P165/P007Z**, 1000×708 Pixel, mattschwarze Reglergruppen mit `CColor(7,11,16,186)` (73 %), unveränderter Frontplatten-Git-Blob `7e57ef8e0c90d84fa4c951bea7815b0233762ca7`. Die GUI-Quellen bleiben unverändert; der Build erzeugt nur eine temporäre SDK-Kompatibilitätskopie.
- Host-MIDI: bevorzugt über **originale MPU-401-UART-Ports 330h/331h** zur originalen unveränderten Creative-DOS-Software. Die ältere BEE0/BEF0-Gastpatch-Alternative ist **nicht** parallel aktiviert. **Wichtig: MPU401 startet in `intelligent`-Modus und die originale Creative-DOS-Software schaltet mit 0x3F nach UART.** Ein von Beginn an auf `uart` gesetztes DOSBox-Gerät ignoriert 0x3F ohne ACK und lässt die originale Creative-Initialisierung scheitern.
- Host-Audio: Original-DOSBox-X-Mixer-PCM per SPSC in den VST3-Callback. **Aktuell nur 44,1 kHz**; 48/96 kHz nicht behaupten. Ein DOSBox-Fenster wird nur im **internen SDL2-DLL-Build** verborgen; die Abtrennung eines zusätzlichen Windows-Audioausgangs nutzt einen internen SDL2-`dummy`-Treiber. **Das klassische `Release|x64` erzeugt SDL1 und darf nicht für das VST3 verwendet werden.** Das SDL2-/CRT-Linking und die Original-DOSBox-X-DLL-Laufzeit sind unter Windows vollständig nachgewiesen: [45109673741](../../actions/runs/45109673741) und [45175109078](../../actions/runs/45175109078). Keine separate Soundkarte, korrekter Shutdown und originaler MIDI-/Mixer-Transport. Kein Original-Creative-Klangtest.

## Tatsächlich geprüfte Meilensteine

| Gegenstand | Ergebnis und belastbarer Nachweis |
| --- | --- |
| Alte P165-GUI und Frontplatte gesichert | Git-Blob geprüft; Fenster und Grafiken in realem Windows-VST3-Compile/Link enthalten |
| Audio/MIDI-SPSC und Gast-ABI-Unit-Tests | **PASS Linux und Windows**, z. B. [Actions 45107619265](../../actions/runs/45107619265) (nur Transport, kein Originalklang) |
| Gepinnter Original-DOSBox-X-AWE32-Kern als interne Windows-x64-DLL | **PASS**; Exporte und DLL-Load nachgewiesen, z. B. [Actions 45106428911](../../actions/runs/45106428911) |
| Original-DOSBox-X-main im Windows-Prozess ausgeführt | **PASS**; DOS-Shell-Befehl `exit` und Rückkehr zum Windows-Testhost, z. B. [Actions 45105475614](../../actions/runs/45105475614) |
| Original-DOSBox-X-Hintergrundthread beendet/gejoint | **PASS** im isolierten DOSBox-Worker-Test, [Actions 45105974571](../../actions/runs/45105974571) |
| Windows-x64-VST3-Bundle mit P165-GUI **und** Original-DOSBox-X-DLL | **PASS**; beide Binärdateien im selben `.vst3/Contents/x86_64-win/`-Ordner, [Actions 45105475614](../../actions/runs/45105475614) |
| Original-DOSBox-X x64 SDL2 + internes CRT-DLL-Linking | **PASS** in [Actions 45109673741](../../actions/runs/45109673741), nach Beseitigung des SDL2-Doppelteinstiegs |
| Originaler emulierter 16-Bit-MPU-401-UART und echte Host-MIDI-Daten | **PASS** in [Actions 45109673741](../../actions/runs/45109673741): echtes DOS-COM-Programm liest am realen emulierten 330h/331h I/O die vom Host eingespeisten Bytes `90 3C 64` aus und schreibt sie als DOS-Datei `MIDI.BIN` |
| Interner SDL2-Dummy-Audiotreiber ohne weitere Windows-Soundkarte | **PASS** in [Actions 45109673741](../../actions/runs/45109673741): interner Audio-Driver `dummy` |
| Tatsächlicher Stereo-PCM-Rückkanal vom Original-DOSBox-X-Mixer zum Host | **PASS** in [Actions 45109673741](../../actions/runs/45109673741): **1024 originale Mixer-PCM-Frames** an real registrierten Windows-C-Callback; **kein Creative-Klangnachweis**, testweise darf Stille enthalten sein |
| Reproduzierbarer VST3-Bundle-Build nach SDL2-Korrektur | **PASS** in [Actions 45109673741](../../actions/runs/45109673741): P165-GUI, Original-DOSBox-X-SDL2-DLL und Windows-VST3-Host in demselben Bundle |
| Authentische Original-Creative-DOS-SF2LIB-Bankladen | Historischer **privater P30-DOSBox-Versuch** belegte SF2-2.00-Bankladen. **Noch nicht innerhalb des neuen VST3 nachgewiesen** |

## Implementiert, aber noch nicht endgültig nachgewiesen

| Bereich | Stand |
| --- | --- |
| Original-DOSBox-X-SDL-Fenster unsichtbar | Quellpatch `SDL_WINDOW_HIDDEN`, VST3-Start `-nogui -nomenu`; Windows-SDL2-DLL/Bundletest PASS, **kein grafischer Studio-Pro-Praxistest** |
| Kein zusätzlicher Windows-Audioausgang | **PASS** mit der richtigen Original-DOSBox-X-`Release SDL2|x64`-Variante: interner Audio-Dummy, 1024 PCM-Stereoframes, MPU-401-UART und sauberer Gaststopp in [45109673741](../../actions/runs/45109673741); integrierte Wiederholung [45175109078](../../actions/runs/45175109078) erfolgreich |
| Exakte Host-MIDI-Bytes bis zum 16-Bit-DOS-Programm | **PASS in Windows-Lauf 45109673741:** eigenständiges `MPUPROBE.COM` schaltet den Original-MPU401 in UART, liest `90 3C 64` über Hardwareports 331h/330h und beweist die Daten per `MIDI.BIN`. **Noch kein originales Creative-DEMO/ROM-/SF2-Instrument.** |
| Originale Creative-Standardbank im Gast | `DEMO.EXE /SBK:SYNTHGM.SF2` korrekt nach Original-C-Quellcode; echte private Dateien beim neuen VST3-Start noch nicht nachgewiesen |
| VST3-Host-Activate/Deactivation | Code vorhanden, aber keine abgeschlossene DAW-/Creative-DOS-Laufzeitabnahme; fehlende private Dateien führen zu bewusstem Aktivierungsfehler |
| SF2-2.01-Kompatibilitätsvorstufe | Ein-Byte-`ifil`-2.01→2.00 nur in **privater Kopie**, Unit-Tests PASS; **noch nicht** live an die DOSBox-Gastbankauswahl angeschlossen |

## NO-GO bis zur echten Produktabnahme

- **Creative-Originalprogramm muss innerhalb des VST3 starten, ROM und echte SF2 laden und musikalisch korrekt erklingen.** ROM-GM-Standardbank ist **`SYNTHGM.SF2` (42.726 Bytes)** aus dem Original-Creative-ADIP-Archiv, nicht mit `SYNTHGM.SBK` (34.832 Bytes) verwechseln.
- Echte EMU8000-Portschreibfolgen, PCM-Ausgabe, Anschlag/ADSR/Filter, Reverb-/Chorus-Sends (0 % = aus), Tempo/CPU und Bank-/Program-/MIDI-Sustain-Verhalten messen.
- SF2-Ladebedienelement, GUI-Regler, Preset-Browser und alle erforderlichen Funktionen tatsächlich ans Audio anbinden. Nur aktuell gebundene Master-Lautstärke/MIDI-Noten/Bank-/Programmwerte **nicht** als vollständige GUI-Funktionalität ausgeben.
- Projekt-Save/Recall, 44,1/48/96 kHz, Mehrfachinstanzen (originale DOSBox-X ist derzeit global und nur **eine aktive Instanz** sicher), Buffer-/CPU-/Realtime-Regression und Steinberg-Validator nachweisen.
- **Fender Studio Pro-Praxistest zuletzt**, erst danach eine eindeutige Versionsnummer, Tag, Release und Download-Paket. Der Stand bleibt `0.0.0-dev`.

## Private Testdateien: Wo sie liegen

[docs/TEST-ASSET-LOCATION.md](docs/TEST-ASSET-LOCATION.md) nennt exakte Dateinamen, private ChatGPT-Bibliothek, Original-Creative-ADIP-Quellfundort und Prüfsummen. **Nie** ROM, private SoundFonts, Creative-DOS-Binaries in öffentliches GitHub, Actions-Artefakte oder Logs einchecken; vorher private Bibliothek durchsuchen, statt erneuten Upload zu verlangen.

**GUI-Kompatibilität (build-only):** Alte P165-Win95-/86Box-ROM-/SF2-Popups werden in der CMake-generierten **temporären GUI-Quelldatei** durch korrekte DOSBox-X/Creative-DOS-Hinweise ersetzt. Original P165, Grafiken, Reglerabdunklung und Hash bleiben byteidentisch. Kein falsches Live-SF2-Versprechen. Original-GUI-Kompilierung mit temporärer Kompatibilitätskopie im integrierten Windows-Build [45175109078](../../actions/runs/45175109078) erfolgreich.

- **Neuer Soundtransport-Gate:** Ein Windows-DOSBox-X-Smoke fordert echte vom originalen Mixer an den Windows-Host gelieferte PCM-Stereo-Frames, bei ausschließlich internem SDL-`dummy`-Treiber. **PASS** Original-Mixer-Stereo-PCM-Transport im Windows-Lauf [45109673741](../../actions/runs/45109673741), auch im integrierten Run [45175109078](../../actions/runs/45175109078). Stille erlaubt; **kein Creative-Tonbeweis**.
- **CI-Beschleunigung:** Pro `main`-Branch bleibt nur der jüngste vollständige DOSBox-X-Windows-Build aktiv; die vorherigen werden mittels GitHub-Concurrency abgebrochen.

- **Privates Originaldaten-Paketieren vorbereitet (keine Veröffentlichung):** `tools/prepare_private_vst3.py` kopiert die bereits existierende gepackte Windows-VST3-DEV-Version zusammen mit dem Original-ROM, der originalen Creative-DOS-`DEMO.EXE` und **`SYNTHGM.SF2`** in den gitignorierten `PRIVATE/`-Ordner. Dabei werden Original-ROM-Hash/Sz., Creative-`DEMO.EXE`-Git-Blob, Standard-SF2-Hash/Sz. und GUI-Frontplatten-Git-Blob verifiziert; es werden **keine** privaten Bytes hochgeladen. Die portablen Stagingtests verwenden ausschließlich erfundene Fake-Bytes und prüfen nur den Paketierungspfad, nicht musikalische Funktion. **Linux- und Windows-PASS: [Actions 45110560473](../../actions/runs/45110560473).**

## Verbindlicher Workflow für jeden Codex und neuen Chat

1. [AGENTS.md](AGENTS.md), [START-HERE.md](START-HERE.md), diese Datei und [CHANGELOG.md](CHANGELOG.md) lesen; neuestes `main`-HEAD und aktuelle Actions-Prüfläufe nachsehen.
2. Ausschließlich `main`. Quell- und Teständerungen im **gleichen Commit** in dieser Datei **und** `CHANGELOG.md` dokumentieren. Keine zusätzlichen Branches.
3. Aussagen zu PASS/FAIL ausschließlich mit tatsächlich beendeten Testläufen und exakt geprüften Stufen belegen. Keine falsche fertige Version.
4. Ziel: **ein funktionierendes AWE64-VST3**, keine weiteren Nebenprojekte und keine ungefragten Zwischenberichte.
