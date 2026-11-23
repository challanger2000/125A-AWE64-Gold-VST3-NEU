# Änderungsverlauf

## 23.11.2026 — Race zwischen VST3-Deaktivierung und DOSBox-Start geschlossen

- `AWE64_RequestStop` darf vor dem Start des Emulatorthreads gesetzt werden. Die vorherige `AWE64_StartGuest`-Implementierung hätte die Stop-Anforderung beim Eintritt wieder gelöscht und `std::thread::join()` unbegrenzt blockieren können.
- Der Gaststart löscht deshalb **keine** Stop-Anforderung mehr; jedes VST3-Aktivieren lädt eine neue DOSBox-DLL-Instanz, deren Atomic-Stopflag initial false ist.
- Separater Quellpatch-Test erzwingt diese Guardrail. **Kein end-to-end Nachweis für plötzlichen DAW-Host-Shutdown.**

## 23.11.2026 — Originale DOSBox-X-Windows-DLL erstmals wirklich starten (neues CI-Gate)

- `tests/WindowsDosBoxGuestSmoke.py`: Windows-Kindprozess lädt die echte DOSBox-X-DLL im eigenen Prozess, ruft **den originalen DOSBox-X-main()** via `AWE64_StartGuest()` auf, führt nur den öffentlichen DOS-Kommandozeilenbefehl `exit` in einer SB16-VM aus und muss wieder nach Python zurückkehren. Mit 35-s-Timeout gegen dauerhaftes Hängen.
- Der Test startet **keine externe DOSBox-EXE**; Python ist nur der isolierte Windows-Tester, nicht Teil des ausgelieferten VST3. Keine Original-Creative-Dateien und kein AWE-ROM in CI.
- Nachweisstatus des neuen Gates bis zum tatsächlichen Windows-Actions-Ergebnis **offen**.

## 23.11.2026 — Windows-VST3-Host-Kompilierung bestanden; originale Creative-SF2LIB-Kommandozeile

- **GitHub Actions 45104901755: PASS.** Der echte Windows-VST3-Host kompiliert mit dem DOSBox-X-Laufzeitloader und dem unveränderten P165-GUI-Quellcode. Dieser Build enthält **noch keinen nachgewiesenen Creative-DOS-Start oder hörbaren Originalklang**.
- Nach erneuter Prüfung des originalen `DOS/DEMO/DEMO.C` verlangt Creative SF2LIB entgegen dem im Demo gedruckten `/SF2:` tatsächlich **`/SBK:`**. Guest-AUTOEXEC-Start korrigiert auf `DEMO.EXE /SBK:SYNTHGM.SF2`.
- `DEMO.C` fragt tatsächlich MPU-401-Port `0x331` ab und leitet Bytes aus `0x330` an originale `ProcessMidi/SendMidi/awe32...`-Funktionen weiter; keine eigenen Creative-Treiber nötig.
- Host-GUI meldet das Vorhandensein des originalen DOSBox-X-Emulator-Workers, **nicht** voreilig einen erfolgreich gestarteten Creative-DOS-Gast; dieser echte Nachweis steht weiter aus.
- Creative-Original-Referenzbank `DOS/DEMO/SYNTHGM.SF2` (42.726 Byte) ist die für den SF2LIB-Gast passende ROM-GM-Bank; die bisherige `SYNTHGM.SBK` (34.832 Byte) ist ein anderes, nicht austauschbares SBK-Format.

## 23.11.2026 — Windows-VST3-Event-Header und MIDI-Zeitstempel korrigiert

- Run 45104591680: echte Windows-VST3-Integration an nicht vollständig inkludierten VST3-SDK-Interfaces IEventList/IParameterChanges gescheitert. Vollständige VST3-Event- und ParamChanges-Header ergänzt.
- Gemäß Original-Steinberg-`ivstevents.h` trägt `Event::sampleOffset` den MIDI-Sampleoffset (nicht `NoteOnEvent` oder `NoteOffEvent`); korrigiert. Host-Klang weiterhin nicht nachgewiesen.

## 23.11.2026 — VST3-Host mit tatsächlich eingebetteter DOSBox-X-Windows-DLL verbinden

- `src/DosBoxRuntimeHost.hpp`: Windows-Host lädt ausschließlich eine echte `AWE64DOSBoxRuntime.dll` **innerhalb desselben VST3-Bundles**. Prüft private Original-AWE-Wave-ROM-Datei (exakt 1 MiB), Original-Creative-DOS-`DEMO.EXE` und privates `SYNTHGM.SF2` vor jeder Aktivierung. Kein extern gestarteter Emulatorprozess.
- Kein gefälschter DSP: Gaststart auf separatem Worker-Thread, originale MPU-401-Bytes via SPSC, ausschließlich echtes DOSBox-X-Mixer-PCM per SPSC zum Audio-Callback. Bank/Programchange und 16 MIDI-NoteOn/Off-Kanäle werden über denselben Guest-UART übertragen.
- Host bei ≠44,1 kHz noch absichtlich abweisend, bis echte Sampleratenkonvertierung gemessen wurde. Mehrfachinstanzen bis zur DOSBox-Global-State-Isolation absichtlich abweisend. Der originalen GUI wurde kein neues Design gegeben.
- CMake und Windows-Actions bauen den Host; Windows-DOSBox-Workflow führt beide DLLs **im VST3-Bundle** zusammen. Zum Commit **keine** Aussage über bestandene Host-/Gast-Audio- oder Exit-/Shutdown-Tests möglich.
## 23.11.2026 — Geordneter In-Process-Gaststopp als Original-DOSBox-Signal

- AWE64_RequestStop als externes Windows-DLL-ABI implementiert. Setzt atomare Stop-Anforderung, geprüft am ursprünglichen DOSBox-X-GFX_Events-Dispatcher; wirft dessen schon vorhandenes Shutdown-Signal auf dem Emulator-Thread. Kein `exit()` in der DAW und keine externe Anwendung.
- Plattformunabhängige Quellpatch-Tests um Stop-Signal ergänzt. Echte Windows-DLL-Kompilierung und Worker-Join/Shutdown werden getrennt nachgewiesen.

## 23.11.2026 — Originale DOSBox-X-Windows-DLL erfolgreich gebaut; direkter MPU-401-MIDI-Pfad

- **Actions 45103209697 PASS:** Gepinnter DOSBox-X-AWE32-Originalquellcode erfolgreich als Windows-x64-DLL kompiliert, DLL-Datei vorhanden, Exporte AWE64_StartGuest, AWE64_HostRegisterIO und AWE64_HostRegisterPCM tatsächlich geprüft. Original-ROM, Creative-DOS-Binary und echte Audioausgabe waren NICHT Teil dieses Tests.
- Der bevorzugte nächste Pfad nutzt die **originale MPU-401-UART-Emulation** (Ports 330h/331h) statt den alten, eigens gepatchten Creative-DOS-Gast; Creative DEMO.EXE kann unverändert MIDI entgegennehmen und über original Creative SF2LIB an EMU8000 weitergeben.
- Quelladapter fügt AWE64_HostRegisterMidi hinzu und stellt originale virtuelle MPU-401-Eingabefunktion auch ohne physisches MIDI-Gerät unter Windows bereit. Studio-Pro-Originalklang und Timing bleiben unbewiesen.
- MIDI-Zweibyte-/Dreibyte-Nachrichten und der Host-Adapter wurden um Regressionstests erweitert; neuer CI-Build erst mit diesem Commit.
## 23.11.2026 — DOSBox-Originalprojekt-BOM-Kompatibilität

- Original-Submodul in Linux-CI ausgecheckt und Commit-Pin bestätigt; 4 Quellpatch-Unit-Tests PASS.
- Original `vs/dosbox-x.vcxproj` beginnt mit UTF-8-BOM. Der erste reale Integrations-Check lehnte dies absichtlich ab. Korrektur erhält nun BOM und CRLF bytegenau; CI-Wiederholung vor DLL-Build nötig.

## 23.11.2026 — Originale DOSBox-X-AWE32-Quellen als interne DLL vorbereiten

- `integration/patch_dosbox_runtime.py`: echte BEE0..BEEF/BEF0-I/O-Port-Handler für die Original-Creative-DOS-API, PCM-Abgriff an der tatsächlichen DOSBox-X-Mischerausgabe sowie originaler DOSBox-Haupteinstieg als DLL-Export. Der originale EMU8000-DSP bleibt unverändert.
- `integration/DosBoxHostAdapter.hpp`: RT-sichere C-Callback-Adapter für die bestehenden Audio-/MIDI-SPSC-Ringe; Unit-Test für tatsächliche Datenweitergabe und Instanztrennung.
- `.github/workflows/windows-dosbox-runtime.yml`: Windows-MSVC-Build des gepinnten DOSBox-X-Git-Submoduls als interne DLL sowie Prüfung der drei Export-Symbole. **CI-Ergebnis zum Commit ausstehend; kein aktiver DOS-Gast und kein echtes PCM nachgewiesen.**
- Keine neuen Branches, keine Veröffentlichung und keinerlei proprietäre Creative-Dateien in Git.
## 23.11.2026 — Windows-Mock-C99-Intrinsics ohne Gasttreiber isolieren
- Erstes Windows-C99-Mock-Test-Build in Actions 45102067745 fehlgeschlagen: MSVC `_inp/_outp` sind Compiler-Intrinsics und dürfen nicht als Funktionen definiert werden. Linux-Mock-Test PASS.
- Nur Mock-Test remappt diese Namen lokal auf `mock_guest_inp/outp` (C-Präprozessor); der originale Gast-Adapter und Creative-Binärtreiber bleiben völlig unverändert.
- **Linux und Windows PASS** im GitHub-Actions-Lauf [45102214476](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45102214476): Host-MIDI-Mailbox, Original-Creative-API-Mock, Quellpatch und SF2-Formatprüfungen. Das ist **keine** Klang- oder DOSBox-VST3-Abnahme.

## 23.11.2026 — Original-Creative-ABI-Dispatch absichern
- Eigenständiger C99-Test ruft den DOS-Gast-Adapter gegen Funktionsprototypen der **Original-Creative-MIDI-API** mit isolierten Mocks; NoteOn/Off, Channel/Poly Pressure, Controller, Program Change, Pitchbend und ungültige Bytes/ACK werden geprüft. **Kein echter Creative-Treiberstart und keine Klangprüfung.**
- VST3-DEV-Factory-URL aus Datenschutzgründen leer gelassen statt persönlicher GitHub-Account-URL.
- Linux-/Windows-Testlauf für Host-Mailbox und Original-DEMO-Quellpatch 45101919395 bestanden; neues Creative-C99-Mock-Gate steht aus.

## 23.11.2026 — Creative-DOS-Gastkommunikation vorbereitet (noch kein DOSBox-I/O)
- MIDI-Messages vom VST3-Host werden per **pro-Instanz** I/O-Mailbox nur an den DOSBox-Worker transportiert; der spätere DOS-Gast erhält MIDI-Status, Daten und Diagnosetimestamp und bestätigt jeden Befehl.
- `guest/AWE64_BRIDGE.H` ruft ausschließlich die ORIGINALEN Creative-DOS-MIDI-Funktionen (`awe32NoteOn/Off`, `awe32ProgramChange`, Controller, Pitchbend usw.) auf. Keine native Treiber-Rekonstruktion.
- `tools/patch_creative_demo.py` patcht ausschließlich eine private Kopie von Creative `DEMO.C`: ein Include und ein GuestPoll-Aufruf; Original-SF2-Bankladecode bleibt erhalten. Quellen und Binaries von Creative werden nicht eingecheckt.
- Tests für Mailbox-FIFO, ACK, Instanzisolation, Host-Time-Diagnostik und sichere Source-Patch-Fälle eingerichtet. **Bis zum tatsächlichen CI-Lauf nicht als PASS deklarieren.**
- Noch offen: DOSBox-X-Emulation in VST3, I/O-Handler, Original-DOS-Binary, echte Audio- und Latenztests.

## 23.11.2026 — Windows-DEV-Build und Creative-DOS-Formatprüfung erfolgreich\n\n- Windows-Actions-Lauf **45101292261: SUCCESS**: VST3-DEV-Shell, originaler GUI-Code in temporärer SDK-Kompatibilitätskopie, Frontplattenressourcen und Packaging wurden kompiliert. Kein nutzbares VST3, die Gast-Engine ist nicht integriert.\n- Synthetische Linux- und Windows-Testläufe **45101292255: SUCCESS**, insbesondere originaler Creative-DOS-SF2-Formatversions-Workaround auf einer *Kopie*.\n- Originale private Testdateien lokal geprüft: AWE-ROM 1 MiB, `SYNTHGM.SBK` Format 1.00, `Synth 512 Bank.sf2` Format 2.01; Änderung auf Arbeitskopie betrifft exakt **ein** Metadaten-Byte, Preset-/Sample-Inhalt unverändert. 77 Benutzer-Presets als `phdr` bestätigt.\n- Weiterhin **kein** DOSBox-in-process-Backend, kein Creative-Gastprogramm im VST3, keine ROM-/SF2-Klangabnahme, kein Studio-Pro-Test.\n\n## 23.11.2026 — GUI-/VST3-Build bestanden, aber Aktivierungs-Veto korrekt\n\n- Run 45100957633: Originalgrafiken kopiert, `DosBoxVst3Shell.cpp`, Factory und die P165-GUI-Buildkopie erfolgreich mit Visual Studio übersetzt und `.vst3` verlinkt.\n- Anschließend hat Steinbergs automatischer Validator das DEV-Modul **zu Recht abgelehnt**: `setActive(true)` liefert bis zur echten DOSBox-X/Creative-Laufzeit ausdrücklich Fehler.\n- Automatische Nachbau-Validierung nur für diesen unfertigen DEV-Shell-Build deaktiviert; **keine** Behauptung eines bestandenen Validators und **keine** Release-Pakete. Neuer Windows-Build noch offen.\n\n## 23.11.2026 — Original-Creative-DOS-SoundFont-Kompatibilität\n\n- P30-Orakel: Creative-DOS-SF2LIB 3.01 verweigert SF2 v2.01, lädt denselben Inhalt mit `ifil=2.00`.\n- Sichere In-Memory-Anpassung nur der Formatversions-Metadaten **in einer privaten Kopie** erstellt; nicht die Original-SF2 umschreiben und niemals Audio-/Preset-Daten verändern.\n- Prüft gültige RIFF-Listen und Format-Chunks, vermeidet Treffer in Sample-Bytes, erkennt alte SBK v1.00, doppelte bzw. ungültige Versionen und verweigert Änderungen bei Fehlern.\n- Synthetische Linux-/Windows-CI-Regressionen eingerichtet; Originaldateien werden ausschließlich privat getestet. Noch keine DOSBox-X-Laufzeitverbindung.\n\n## 23.11.2026 — Windows-VST3-DEV-Shell-Build-Korrektur

- Erster Windows-Build Run 45100511721 **FAIL**: fehlender PresetBrowserView-Header, fehlendes UString-Include und geänderte VSTGUI-Viewport-API.
- Beide Header ergänzt. Die ursprüngliche freigegebene GUI-Datei in Git bleibt **byteidentisch**; ausschließlich die in CMake erzeugte temporäre Buildkopie nutzt die zum Steinberg SDK 3.7.12 passende Viewport-API.
- Windows-Ressourcenpfade im RC-Generator berichtigt. Neuer CI-Build muss diese Änderungen erst bestätigen.

## 23.11.2026 — Windows-VST3-DEV-Shell mit originaler P165-GUI (Audioengine NICHT verbunden)\n\n- CMake-Konfiguration für Steinberg SDK 3.7.12 und originale P165-Frontplatte/Assets erstellt; Frontplattenhash wird vor dem Build geprüft.\n- P165-GUI ohne graphische Neugestaltung an neuen VST3-Entwicklungscontroller angeschlossen.\n- DEV-Shell verwendet eine eigene temporäre VST3-Klassen-ID, um bestehende Installationen/Projekte nicht zu beschädigen.\n- Activation fail-closed: Ohne tatsächlich eingebetteten DOSBox-X-AWE32-Kern wird keine nutzbare Audiolaufzeit behauptet und kein Endkunden-Paket veröffentlicht.\n- Windows-GitHub-Build eingerichtet, bei Commit noch nicht abgeschlossen; er testet nur Kompilierung, nicht Original-Creative-Klang.\n\n## 23.11.2026 — Start der in-process Audio-/MIDI-Transportimplementierung (kein Plugin)

- `AGENTS.md`: verbindliche Anleitung für Codex und spätere Chats eingefügt.
- `src/RealtimeQueues.hpp`: begrenzte SPSC-Audio-/MIDI-Ringe und getrennte per-Instanz-Zustände, callback-sicher ohne dynamische Speicherreservierung.
- `tests/RealtimeQueuesTest.cpp`: Release-Checks für FIFO, Overflow, leere Audio-Ausgabe, MIDI-Zeitstempel, getrennte Instanzen und echte Producer-/Consumer-Threads.
- Lokaler Linux-C++20-Test bestanden; Windows-/Linux-GitHub-Tests über `.github/workflows/audio-bridge-tests.yml` erfolgreich: Lauf 45099994417, beide Betriebssysteme PASS. Die Tests beweisen ausschließlich die Transportkomponente, **nicht** das fertige VST3.
- **Nicht gebaut:** DOSBox-X innerhalb VST3, Creative-DOS-Treiber, echte PCM-Ausgabe, Host-Recall, Release.

## 23.11.2026 — Neuaufbau, noch keine VST3-Version

- Neues Repository ausschließlich mit `main` angelegt.
- Originale P165/P007Z-Hardware-GUI, inklusive 73-%-Reglerhintergründen und originalem 1000×708-Frontplattenbild, byteidentisch übernommen und Git-Hashes geprüft.
- `turican0/dosbox-x-AWE32` auf `masterAWE32` Commit `f3a95b9f19a7f9863295e6481f271cf04733c635` als Submodul fixiert.
- Vergleich mit offiziellem DOSBox-X dokumentiert; kein ungetestetes Rebase vorgenommen.
- Verzeichnis der privaten ROM-/SBK-/SF2-Testdateien mit Fundorten dokumentiert, ohne die Dateien zu veröffentlichen.
- **Zentrale Versionsdokumentation und GitHub-Actions-Pflichtprüfung eingeführt.**
- **Status:** Nur Quellgerüst und GUI-Sicherung. Keine lauffähige DOSBox-X-VST3-Audiolaufzeit, kein Release, keine Klang- oder DAW-Abnahme.

**Regel für folgende Einträge:** Jede technische Änderung im selben Commit hier protokollieren und in [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md) den nachgewiesenen Funktionsstand aktualisieren. Keine unbestätigten PASS-Aussagen.
