# START HERE — verbindlicher Projektstand

**Datum:** 2026-11-23  
**Ziel:** Ein *einziges* Windows-x64-VST3 mit intern eingebettetem DOSBox-X-AWE32 und **originaler Creative-DOS-Software**.  
**Branch:** ausschließlich `main`. **Keine parallelen Engines oder Entwicklungszweige.**

## Was wirklich vorhanden ist

1. **GUI:** Originale komplette P165/P007Z-Hardware-GUI als exakt übernommene Quelldateien in `gui/`; nachträgliche Abdunklung der Reglerflächen auf 73 %, Alpha 186 (Quellcommit `988b04aa2b6a02884d17fc792244919e7f9384d4`). Die unveränderte P165-Frontplatte wird anhand des verifizierten Git-Blob-Hashes `7e57ef8e0c90d84fa4c951bea7815b0233762ca7` eingebracht.
2. **DOSBox-X:** Fertiger öffentlicher AWE32-Fork als Git-Submodul unter `third_party/dosbox-x-AWE32`, fixiert auf `f3a95b9f19a7f9863295e6481f271cf04733c635` (`masterAWE32`).
3. **Original-Creative-DOS-Laufzeit:** Historisch nachgewiesene Ausführung von Creative DOS SF2LIB/DEMO in DOSBox-X mit dem AWE-ROM. **Noch nicht im neuen VST3 verbunden.**
4. **VST3-Integration:** Noch **nicht implementiert/abgenommen**. Keine alten 86Box-/Win95- oder P009-Engines übernommen. Auch die bestehende GUI muss an die neue Runtime verdrahtet werden.

## Einziger nächster technischer Pfad

- Git-Submodul checkout und reproduzierbarer Windows-x64-VST3-Build mit internem DOSBox-X-Kern.
- Echte originale Creative-DOS-Laufzeit im Plugin starten und Midi-/SF2-Kommandos durch diese Laufzeit bis zu den EMU8000-Registern nachweisen.
- Host-MIDI samplegenau, Audio 44,1/48/96 kHz, 28 MiB Sample-RAM, realer Klang, SF2-/ROM-Wechsel, 16 Kanäle und State-/Projekt-Recall prüfen.
- Alle GUI-Elemente nachweisbar mit echtem Klang / echter Registeränderung verdrahten. Keine funktionslosen Bedienelemente als fertig deklarieren.
- 47/47 Steinberg-Validierung ist nur *ein* Gate, kein Ersatz für klangliche und Host-Tests.
- Abschließend Praxistest in Fender Studio Pro; vorher kein Release.

## Projektregeln

**Nur `main`**, keine weiteren Entwicklungszweige. **Nur ein Audio-Kern und eine GUI.** Keine proprietären ROMs, Creative-Treiberdateien, privaten SF2/SBK-Samples oder Win95-Images in Git. Lizenzpflichten vor Veröffentlichung prüfen. Reproduzierbare Nachweise statt Erfolgsmeldungen aufgrund bloßer CI-Builds.

Source-of-truth: diese Datei wird für **tatsächlich** abgeschlossene Meilensteine fortgeschrieben, keine P001–P999-Parallellisten.
