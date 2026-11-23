# Änderungsverlauf

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
