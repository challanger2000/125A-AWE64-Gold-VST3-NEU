# AKTUELLE VERSION – verbindlicher Stand

**Produkt:** 125A AWE64 Gold (Windows x64 VST3, DOSBox-X)  
**Stand:** 23.11.2026  
**Entwicklungskennung:** `0.0.0-dev`  
**Release / abgenommenes Instrument:** **NEIN – noch nicht gebaut**  
**Einziger Branch:** `main`  
**Exakter neuester Quellstand:** [oberster Commit auf main](../../commits/main) (nicht auf eine veraltende SHA im Text verlassen)

Alle nachfolgenden Aussagen beziehen sich auf **nachgewiesene Prüfungen oder klar gekennzeichnete Implementierung**. Ein erfolgreicher Windows-Build oder ein DOS-Shell-Test beweist keinen AWE64-Sound.

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
