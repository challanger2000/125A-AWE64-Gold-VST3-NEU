# Änderungsverlauf

## 23.11.2026 — Windows-VST3-DEV-Shell-Build-Korrektur

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
