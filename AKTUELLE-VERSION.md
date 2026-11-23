# Aktuelle Version — verbindlicher Stand

**Stand:** 23.11.2026  
**Produkt:** 125A AWE64 Gold VST3 (DOSBox-X-Neustart)  
**Entwicklungskennung:** `0.0.0-dev` — **noch keine funktionierende VST3-Version, kein Release**  
**Einziger Entwicklungsbranch:** [`main`](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/tree/main)  
**Neuester Quellcode-Commit:** [aktuelles `main`-HEAD](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/commits/main) (immer den obersten Commit prüfen; statische SHA wäre nach jedem Commit veraltet).

## Aktueller nachgewiesener Funktionsstand

| Bereich | Aktueller Stand | Referenz |
| --- | --- | --- |
| GUI | **Gesichert, nicht mit neuer Runtime verbunden** | P165/P007Z, Quellcommit `988b04aa2b6a02884d17fc792244919e7f9384d4`; 73 % Regler-Abdunklung, originale Grafiken |
| Originalfrontplatte | **Gesichert, Git-Hash geprüft** | Git-Blob `7e57ef8e0c90d84fa4c951bea7815b0233762ca7` |
| DOSBox-X/AWE32 | **Als Quell-Submodul eingebunden, noch nicht im VST3 ausgeführt** | `turican0/dosbox-x-AWE32`, `masterAWE32`, `f3a95b9f19a7f9863295e6481f271cf04733c635` |
| Creative-DOS-Treiber | **Integrationsnachweis im neuen Plugin ausstehend** | Originale Creative-DOS-Software ist Ziel, keine rekonstruierte Ersatztreiberlogik |
| ROM, SYNTHGM.SBK, SF2 | **Private Testdaten in ChatGPT-Dateibibliothek auffindbar** | [TEST-ASSET-LOCATION.md](docs/TEST-ASSET-LOCATION.md); keine öffentlichen Binärdaten |
| Echtzeit-Audio-/MIDI-Transport | **SPSC-Queue-Quellcode implementiert; lokaler Linux-C++20-Test PASS** (Thread, Overflow, Underrun, Mehrfachinstanz), Windows-GitHub-CI noch ausstehend | `src/RealtimeQueues.hpp`, `tests/RealtimeQueuesTest.cpp` |\n| Windows-x64-VST3 | **Noch nicht gebaut** | Kein freigegebener Installer, kein bestandener Hosttest |
| MIDI/Audioklang/Projekt-Recall | **Noch nicht nachgewiesen** | Tests erst nach Runtime-Einbindung |
| Fender Studio Pro | **Nicht geprüft** | Praxis-Abnahme erst am Ende |

## Unveränderliche Versionsregeln

1. **Diese Datei ist die zentrale Versions- und Statusreferenz.** Sie wird im **selben Commit** wie jede technische Änderung an `src/`, `gui/`, dem DOSBox-X-Submodul oder dem VST3-Buildsystem aktualisiert. Ein GitHub-Actions-Check prüft, ob sie bei entsprechenden Änderungen mitgeändert wurde.
2. **`CHANGELOG.md`** hält die tatsächlichen technischen Änderungen und Nachweise chronologisch fest; jeweils exakt mit Datum und geändertem Modul. Keine allgemeinen Erfolgsmeldungen ohne belegtes Testergebnis.
3. Ein **erfolgreicher Quellcommit ist nicht automatisch eine funktionierende VST3-Version**. Build, Installer, Steinberg-Validator, originale Creative-Klangerzeugung und abschließende Studio-Pro-Abnahme sind getrennte Prüfgates.
4. Eine **veröffentlichte Versionsnummer** (Tag, Release, ZIP-Dateiname, Installer und Plugin-internes Versionsfeld) darf erst nach erfolgreichem Abnahme-Gate gesetzt werden und muss überall übereinstimmen. Bis dahin bleibt `0.0.0-dev`.
5. Es gibt **nur `main`**, keine Phasen- oder Forschungsbranches. Neue Entwicklungsstände ersetzen nicht stillschweigend einen geprüften Stand; sie erhalten einen nachvollziehbaren Commit und den Status **geprüft / ungeprüft / gescheitert**.
6. Vor weiterer Arbeit immer zuerst **diese Datei, `START-HERE.md` und den aktuellen `main`-HEAD** prüfen. Bei erneuter Prüfung der Upstream-Version den zuletzt bestätigten DOSBox-X-AWE32-Commit ebenfalls aktualisieren.

## Letzte dokumentierte Änderung

**23.11.2026:** Erste Audio-/MIDI-Transportkomponente (pro Instanz getrennte, begrenzte SPSC-Ringe) implementiert; lokaler C++20-Test inklusive tatsächlicher Thread-Kommunikation bestanden; GitHub-Windows-Test zum Commit ausstehend. **Noch keine integrierte DOSBox-Laufzeit, Creative-DOS-Software oder VST3-Audiofunktion.**
