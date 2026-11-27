# Herkunft und überprüfbare Quellstände

## Die festgelegte Laufzeit
- `turican0/dosbox-x-AWE32`, Branch `masterAWE32`.
- Lokaler Kandidat mit Git-Commit `81d0fde73917b57a1716e610dec2605c36def555`. Die Besitzer-Klangreferenz `609ab17` verwendet `f3a95b9f19a7f9863295e6481f271cf04733c635`.
- Der Original-Commit ändert nur EMU8000-C-Code und Header. Der produktseitige Patcher erhält die strikte Datei-ROM-Initialisierung und gibt den optionalen Modulations-Attack-Cache mit dem Chip frei.
- Als Git-Submodul gepinnt; keine 86Box-VM, kein Windows-95-Gastsystem und keine native P009-Engine.
- Der Upstream enthält **EMU8000-Chipcode aus 86Box/AWE32Emu**. Das ist ein Teil der DOSBox-X-AWE32-Hardwareemulation, **kein** eingebetteter 86Box-PC.

## Die unveränderte P165/P007Z-GUI
- Grafische Endfassung 18.11.2026, Git-Commit `988b04aa2b6a02884d17fc792244919e7f9384d4`.
- Quellpfad im früheren Repository: `experiments/p007-inprocess-86box/vst3/p165/`.
- Die Quelldateien in `gui/` wurden **blob-identisch** übertragen. Abdunklung: `CColor(7,11,16,186)` statt `CColor(7,11,16,153)`.
- Die drei Knopf-/Tasten-Atlanten und die VSTGUI-`uidesc` sind ebenfalls original.
- Originale Bildfrontplatte aus dem früheren `125A-AWE-Synthesizer`-Repository, Stand `8e8315c2ecab9f4a8bdbd415c1b7af5a8485a3c1`, Original-Git-Blob `7e57ef8e0c90d84fa4c951bea7815b0233762ca7`.
- Die GUI-Sicherung ist **keine** Bestätigung einer funktionierenden DOSBox-X-VST3-Kopplung.

## Lizenz und private Inhalte

Der DOSBox-X-Fork enthält GPL-lizenzierte Komponenten. Rechtslage des gesamten Pakets vor Veröffentlichung prüfen. Die ursprünglichen Creative-DOS-Treiber und ROM-/SoundFont-Bankdaten gehören **nicht** in das öffentliche Repository; eine Nutzungs-/Verbreitungserlaubnis ergibt sich nicht aus dem Besitz der Binärdateien.

## Nicht übernommen

- P007/86Box-VM mit Windows-95-Gastbetriebssystem.
- P008/P009 mit nachgebauter Creative-Treiberlogik.
- Alte, nicht getestete Rendering-Engines, private ROMs, SF2-Samples, Windows-Images.

## Neutrale installierte Dateinamen (27.11.2026)

`Engine.dll` ist die intern gebaute DOSBox-X-Laufzeit mit unveränderter Herkunft; `ROM/Driver.exe` ist der byteidentische originale Creative `DOS/DEMO/DEMO.EXE` (Git-Blob e10b8705c6a46444a998c4f6aa74a1cda2081506). Die tatsächliche ROM-Bank ist `ROM/SYNTHGM.SF2` (SHA-256 78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f), nicht SBK. Historische Namen in Dokumenten/Upstream sind Herkunftsnachweise, keine Produktnamen. Keine Rechteklärung durch Umbenennung; keine Veröffentlichung proprietärer Bytes.
