# Aktuelle Version — verbindlicher Stand

**Stand:** 23.11.2026  
**Produkt:** 125A AWE64 Gold VST3 (DOSBox-X-Neustart)  
**Entwicklungskennung:** `0.0.0-dev` — **noch keine funktionierende VST3-Version, kein Release**  
**Einziger Entwicklungsbranch:** [`main`](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/tree/main)  
**Neuester Quellcode-Commit:** [aktuelles `main`-HEAD](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/commits/main) (immer den obersten Commit prüfen; statische SHA wäre nach jedem Commit veraltet).

## Aktueller nachgewiesener Funktionsstand

| Bereich | Aktueller Stand | Referenz |
| --- | --- | --- |
| GUI | **Gesichert; Windows-VST3-Controller mit internem Original-DOSBox-X-Ladepfad verdrahtet, Host-Klangprüfung ausstehend** | P165/P007Z, Quellcommit `988b04aa2b6a02884d17fc792244919e7f9384d4`; 73 % Regler-Abdunklung, originale Grafiken |
| Originalfrontplatte | **Gesichert, Git-Hash geprüft** | Git-Blob `7e57ef8e0c90d84fa4c951bea7815b0233762ca7` |
| DOSBox-X/AWE32 | **Als Quell-Submodul eingebunden, noch nicht im VST3 ausgeführt** | `turican0/dosbox-x-AWE32`, `masterAWE32`, `f3a95b9f19a7f9863295e6481f271cf04733c635` |
| Creative-DOS-Treiber | **Integrationsnachweis im neuen Plugin ausstehend** | Originale Creative-DOS-Software ist Ziel, keine rekonstruierte Ersatztreiberlogik |
| ROM, SYNTHGM.SBK, SF2 | **Private Testdaten in ChatGPT-Dateibibliothek auffindbar** | [TEST-ASSET-LOCATION.md](docs/TEST-ASSET-LOCATION.md); keine öffentlichen Binärdaten |
| Echtzeit-Audio-/MIDI-Transport | **SPSC-Queue-Quellcode implementiert; Linux- und Windows-C++20-Tests PASS** (FIFO, Overflow, Underrun, getrennte Instanzen, 200.000 übertragene Nachrichten im Zweithread); tatsächliche DOSBox/EMU8000-Klangfunktion NICHT GEPRÜFT | `src/RealtimeQueues.hpp`, `tests/RealtimeQueuesTest.cpp` |
| Windows-x64-VST3 DEV-Shell | **Windows-VST3-Host kompiliert** (Actions 45104901755 PASS), noch keine klanggeprüfte Runtime-/Creative-DOS-Gastabnahme. **Host-Code für echte, im VST3 enthaltene DOSBox-X-Windows-DLL programmiert.** Erster VST3-Windows-CI-Versuch 45104591680 an fehlenden vollständigen Steinberg-Event-/Parameter-Interfaces gescheitert; Include- und korrekte Event-Offset-Verwendung korrigiert, Wiederholung ausstehend. Originaldateien und Worker-Lebensdauer werden geprüft. Aktivierung scheitert ohne PRIVATE/ROM/Creative DEMO/SF2, echte Gast-/Host-Klangprüfung ausstehend. Der nächste CI-Build prüft MSVC-Kompilierung und gemeinsame Bundle-Struktur; kein Endprodukt. | `src/DosBoxVst3Shell.*` |
| Creative-DOS-Bankformat-Vorstufe | **SF2-2.01-Formatversionsanpassung an SF2LIB 3.01 implementiert; synthetische Linux-/Windows-C++20-Tests PASS (Actions 45101292255); lokale Original-SF2-Metadatenprüfung PASS; noch nicht an DOS-Gast angebunden** | `src/Sf2CreativeDosCompatibility.hpp` |
| Original-Creative-DOS-Gast-Kommunikation | **Protokoll/Quellvorbereitung implementiert**, kein DOSBox-I/O-Handler eingebunden und kein DOS-Guest-Binary gebaut; Mailbox-/Patchtests unter Linux und Windows PASS (CI 45101919395); Creative-DOS-API-Dispatch-Mock nach Windows-Intrinsic-Korrektur Linux+Windows PASS (Actions 45102214476); ausschließlich synthetische API-Mocks, kein echter Gastprogramm-Test | `src/CreativeDosMailbox.hpp`, `guest/AWE64_BRIDGE.H` |
| Funktionsfähiges Windows-x64-VST3 | **Noch nicht gebaut** | Kein freigegebener Installer, kein bestandener Hosttest |
| MIDI/Audioklang/Projekt-Recall | **Noch nicht nachgewiesen** | Tests erst nach Runtime-Einbindung |
| Fender Studio Pro | **Nicht geprüft** | Praxis-Abnahme erst am Ende |

## Neu: echte DOSBox-X-Quellintegration (23.11.2026)

- Implementiert: Quellpatch in `integration/patch_dosbox_runtime.py` mit echten emulierten Host-MIDI-I/O-Ports, PCM-Abgriff am originalen DOSBox-X-Mixer und exportiertem Emulator-Einstiegspunkt für eine interne Windows-x64-DLL.
- Implementiert: `integration/DosBoxHostAdapter.hpp`, welcher die vorhandenen SPSC-/Creative-DOS-Brücken mit den C ABI-Aufrufen verbindet. Originaler EMU8000-Code und Creative-Synthesizerlogik unverändert.
- **Originaler DOSBox-X-AWE32-Quellcode als Windows-x64-DLL kompiliert und drei Exporte nachgewiesen** (GitHub Actions 45103209697: PASS). Das beweist nur die native DOSBox-Komponente, **keinen** Creative-DOS-Gaststart, keinen VST3-Klang.
- **Neue bevorzugte MIDI-Anbindung:** hostseitige DAW-MIDI-Nachrichten in das originale emulierte MPU-401-UART-Gerät (0x330/0x331), welches das unveränderte Creative-DOS-DEMO.EXE bereits liest und an die originalen Creative-SF2LIB-Funktionen weitergibt. Quellcode implementiert, zugehöriger Windows-DLL-Kompilierungs-/UART-Integrationstest noch ausstehend.
- **Neu:** Export `AWE64_RequestStop()` implementiert, der das bestehende DOSBox-X-Shutdown-Signal über den emulierten Eventloop auslöst. Start/Stop-Race geschlossen: der Gaststart löscht keine bereits vor dem Workerstart eingegangene Stop-Anforderung. Windows-Gaststopp noch nicht end-to-end abgenommen. **Nicht implementiert/abgenommen:** echte DOS-Gastlaufzeit im VST3, ROM/SBK/SF2-Klang, Host-Rate-Konvertierung, zuverlässiger Shutdown, Mehrfachinstanzisolierung, DAW-Recall und Studio-Pro-Praxistest.

- **Headless host (Quellstand):** originale SDL-Emulationsfenster der internen DOSBox-X-DLL mit `SDL_WINDOW_HIDDEN` angelegt, Startargumente `-nogui -nomenu` ergänzt. Nur originale VST3-P165-Bedienoberfläche sichtbar. Windows-Gast-Regression bei diesem Commit noch ausstehend.

- **DAW-Audioisolierung (neuer Stand, CI noch offen):** interne DOSBox-X-SDL2-Laufzeit über originalen SDL-`dummy`-Treiber statt zusätzlicher Windows-Audiohardware. Der originale DOSBox-Mixer erzeugt weiter PCM, das als einziger Ausgang im VST3 erscheint. Windows-Prozess-Smoke verlangt einen nachweislich laufenden Dummy-SDL-Treiber; ohne diesen Test kein PASS.

- **Neues vollständiges MPU-401-I/O-Gate (noch nicht durchgelaufen):** Eigenes 16-bit-DOS-COM-Testprogramm schaltet die originale DOSBox-X-MPU-401 in UART-Modus, erwartet den originalen 0xFE-ACK, liest Note-On über Hardwareports 331h/330h und schreibt die realen drei MIDI-Bytes per DOS-Dateioperation in MIDI.BIN. Windows-Test speist `90 3C 64` über den echten Host-Callback ein, verifiziert Datei und Verbrauch exakt einmal. Dies beweist bei PASS originalen Host→DOS-MIDI-Pfad, noch nicht Creative-SF2-/EMU8000-Klang.

## Unveränderliche Versionsregeln

1. **Diese Datei ist die zentrale Versions- und Statusreferenz.** Sie wird im **selben Commit** wie jede technische Änderung an `src/`, `gui/`, dem DOSBox-X-Submodul oder dem VST3-Buildsystem aktualisiert. Ein GitHub-Actions-Check prüft, ob sie bei entsprechenden Änderungen mitgeändert wurde.
2. **`CHANGELOG.md`** hält die tatsächlichen technischen Änderungen und Nachweise chronologisch fest; jeweils exakt mit Datum und geändertem Modul. Keine allgemeinen Erfolgsmeldungen ohne belegtes Testergebnis.
3. Ein **erfolgreicher Quellcommit ist nicht automatisch eine funktionierende VST3-Version**. Build, Installer, Steinberg-Validator, originale Creative-Klangerzeugung und abschließende Studio-Pro-Abnahme sind getrennte Prüfgates.
4. Eine **veröffentlichte Versionsnummer** (Tag, Release, ZIP-Dateiname, Installer und Plugin-internes Versionsfeld) darf erst nach erfolgreichem Abnahme-Gate gesetzt werden und muss überall übereinstimmen. Bis dahin bleibt `0.0.0-dev`.
5. Es gibt **nur `main`**, keine Phasen- oder Forschungsbranches. Neue Entwicklungsstände ersetzen nicht stillschweigend einen geprüften Stand; sie erhalten einen nachvollziehbaren Commit und den Status **geprüft / ungeprüft / gescheitert**.
6. Vor weiterer Arbeit immer zuerst **diese Datei, `START-HERE.md` und den aktuellen `main`-HEAD** prüfen. Bei erneuter Prüfung der Upstream-Version den zuletzt bestätigten DOSBox-X-AWE32-Commit ebenfalls aktualisieren.

## Letzte dokumentierte Änderung

**23.11.2026:** Windows VST3-DEV-Shell kompiliert und GUI-Originalgrafiken eingebunden (Actions 45101292261 PASS). Die Steinberg-Validator-Abnahme und echte Creative-DOS-Audioausgabe sind **nicht** bestanden/nicht nachgewiesen; Host-Aktivierung bleibt absichtlich blockiert. Eine nicht verbundene DOSBox-Engine wird ausdrücklich nicht als lauffähiges Plugin ausgegeben.\n\n**23.11.2026:** Erste Audio-/MIDI-Transportkomponente (pro Instanz getrennte, begrenzte SPSC-Ringe) implementiert; lokaler C++20-Test inklusive tatsächlicher Thread-Kommunikation bestanden; GitHub-Tests unter Linux und Windows bestanden (Actions-Lauf 45099994417). **Noch keine integrierte DOSBox-Laufzeit, Creative-DOS-Software oder VST3-Audiofunktion.**
