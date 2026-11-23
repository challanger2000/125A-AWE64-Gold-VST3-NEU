# 125A AWE64 Gold VST3 — Neustart

**Projektstatus:** Original-DOSBox-X-In-Process-DLL und Windows-VST3-Bundle mit originaler P165-GUI erfolgreich gebaut; **kein abgenommenes, musikalisch funktionsfähiges Instrument und kein Release.** Siehe [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md).

## Festgelegte Architektur

- **Ein einziges Windows-x64-VST3-Instrument**, das seine Laufzeitkomponenten intern betreibt; keine separat zu startende DOSBox.
- **DOSBox-X-AWE32** von `turican0/dosbox-x-AWE32`, unverändert auf **`masterAWE32` Commit `f3a95b9f19a7f9863295e6481f271cf04733c635`** fixiert.
- **Originale Creative-DOS-Treiber-/SF2LIB-Laufzeit**, sobald ihre korrekte Funktion und die Nutzungsrechte nachgewiesen sind. Keine Eigenimplementierung als stiller Ersatz.
- **Authentischer EMU8000**, echtes AWE-ROM und SF2/SBK über die emulierte Hardware; Hardwareverhalten messbar testen.
- **Die bestehende P165/P007Z-GUI behalten**, insbesondere das originale 1000×708-Frontpanel und die späteren **73 % mattschwarzen Reglergruppen (Alpha 186)**.

## Projektordnung

Es gibt nur **`main`** als Entwicklungsbranch. Keine neuen Forschungs-, Phasen- oder Testbranches. Änderungen kommen nachvollziehbar als kleine Commits auf `main`. `START-HERE.md` beschreibt den einzigen aktuellen Zustand.

```text
gui/                     Originale P165/P007Z-Oberfläche und Grafiken
third_party/dosbox-x-AWE32  Fixierter Upstream-DOSBox-X-AWE32-Fork (Git-Submodul)
docs/                    Herkunft, technische Grenzen, Abnahmebedingungen
src/                     Windows-VST3-Host und originaler DOSBox-X-DLL-Loader (noch nicht klanggeprüft)
integration/             Gepinnter DOSBox-X-Quellpatch, originale MPU-401-MIDI- und PCM-Schnittstellen
```

**Die gesicherte GUI und die originale DOSBox-X-AWE32-DLL sind tatsächlich im selben Windows-VST3-Bundle kompiliert. Vollständige Original-Creative-DOS-/ROM-/SF2-Klangabnahme steht noch aus.** Der bisherige 86Box-Win95-P007-Code und die native P009/AWE32Emu-Anwendung werden **nicht** als Runtime übernommen.

## Entwicklungs- und Freigaberegeln

1. Nur erprobte, benötigte Komponenten übernehmen; weder eine zweite Engine noch eine zweite Oberfläche parallel entwickeln.
2. Original-ROM, Creative-Treiber und private SoundFonts niemals in das öffentliche Git-Repository übernehmen.
3. Git-Historie, Binärartefakte und Versionsnummern nur mit realen Prüfergebnissen als erfolgreich kennzeichnen.
4. Erst Original-DOS-Runtime und DOSBox-in-VST3 nachweisen, danach MIDI, SF2/SBK, echte PCM-Ausgabe, Projekt-Recall und Echtzeitverhalten prüfen.
5. **Studio-Pro-Praxistest erst am Ende**, nach den automatisierten Tests.
6. GPL- und Creative-Rechte vor einer Weitergabe klären. DOSBox-X/EMU8000-Quellcode unterliegt GPL-Pflichten.

**Immer zuerst den aktuellen Stand prüfen:** [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md), [CHANGELOG.md](CHANGELOG.md), [START-HERE.md](START-HERE.md), [Quellen und Herkunft](docs/PROVENANCE.md).
