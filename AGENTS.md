# AGENTS.md — verbindliche Anweisung für Codex und folgende Chats

**Vor JEDER Änderung:** Lies `AKTUELLE-VERSION.md`, `START-HERE.md`, `CHANGELOG.md`, `docs/PROVENANCE.md` und `docs/TEST-ASSET-LOCATION.md`. Prüfe dann `main` HEAD, den Git-Status und tatsächlich vorhandene Testnachweise. Ältere Chat-Aussagen sind kein Ersatz für Quellcode und reale Build-/Klangtests.

## Einzige festgelegte Architektur
- **Ein einziges Windows-x64-VST3-Instrument**, das die **DOSBox-X-AWE32-Emulation intern** betreibt. Kein separat gestartetes DOSBox-Programm.
- Fixierte Grundlage `turican0/dosbox-x-AWE32`, `masterAWE32`, Commit `f3a95b9f19a7f9863295e6481f271cf04733c635` als Git-Submodul `third_party/dosbox-x-AWE32`. Upstream-Upgrades nur bei belegtem Bedarf und bestandenen Regressionen.
- **Originale Creative-DOS-Software** zur Ansteuerung der emulierten EMU8000-Hardware verwenden; keinen nachgebauten Creative-Treiber oder nativen SF2-Player als stillschweigenden Ersatz.
- **MPU401-Startmodus: `intelligent` (nicht `uart`)**. Die Original-Creative-DOS-Demo schaltet selbst mit Befehl 0x3F in UART und erwartet 0xFE ACK; bei vorab eingestelltem UART ignoriert die Original-DOSBox-X-Emulation den Befehl. Diese Einstellung ist für Original-Creative-Kompatibilität zwingend.
- **Primäre MIDI-Architektur ab 23.11.2026:** DAS ORIGINALE Creative-DOS-DEMO.EXE liest UART-Port 330h/331h. Deshalb MIDI vom VST3 in die **originale MPU-401-Eingangsqueue der gepinnten DOSBox-X** einspeisen (AWE64_HostRegisterMidi). Den älteren BEE0/BEF0-Patch eines DOS-Gastprogramms nur als nicht gleichzeitig aktives, experimentelles Fallback behandeln. Nicht beide auf denselben SPSC-MIDI-Consumer schalten.
- Keine 86Box-PC-VM, kein Windows-95-Gast, keine native P009-AWE32Emu-Ersatz-Engine. Der im DOSBox-X-Fork enthaltene 86Box-abgeleitete **EMU8000-Chipcode** gehört ausdrücklich zur erlaubten Grundlage.
- **Nachgewiesener Hardwarestand ab 23.11.2026:** [Actions 45186047184](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45186047184) **SUCCESS**: echte Original-AWE32-`AWEUTIL /S`-Startsequenz hat **9.688 EMU8000-Registerschreibzugriffe** im unveränderten DOSBox-X-Kern erzeugt; originaler MPU-401-MIDI-Test und realer gemeinsamer Windows-VST3-Bundle-Build erfolgreich. **Nur synthetisches Null-ROM in CI – keine originale Creative-DOS-SF2-Klangausgabe nachgewiesen.** Die Ursache des früher fehlenden EMU8000-Starts war `-silent`: der Originalcode erzwingt dafür `SBT_NONE`. `-silent` darf NIE in das VST3-Produktstartargument zurückkehren; nur `-nogui -nomenu` dürfen das Fenster unterdrücken.
- **Nachgewiesene NICHT-stumme PCM-Mixer-Ausgabe ab 23.11.2026:** [Windows 45189311093](../../actions/runs/45189311093) vollständig PASS. Originaler DOSBox-X-Emulator spielt über 16-Bit-DOS-Programm mit echten PIT-/PC-Speaker-Ports, **68.608 Stereo-PCM-Frames**, davon **17.633 !=0**, Peak PCM16 **9.990** im originalen Host-PCM-Callback. Internes Dummy-Audio; Windows-VST3-Bundle mit originaler DLL erfolgreich gebaut; [Linux/Windows 45189310952](../../actions/runs/45189310952) PASS. **Die Schallquelle war NICHT der EMU8000**, die ROM-Datei war absichtlich ein synthetisches Null-ROM. Echte Creative-/Original-ROM-/SF2-Klangabnahme und DAW-Host-Test weiterhin offen. Kein Release.
- **Original-Creative-Klang privat vorbereiten:** `tools/Run-Private-Creative-AWE-Test.ps1` (alle Original-Dateien bleiben auf dem Windows-PC, ROM wird vom Besitzer angegeben, Creative-Archiv nur lokal aus öffentlicher Quelle heruntergeladen, alle Daten vor Verwendung gehasht, vorhandene Owner-Dateien nicht überschrieben). Test ist **noch nicht bestanden**, öffentliche Actions enthält weiterhin keinerlei Originalbytes. VST3-Host-Audio mit synthetischer DOS-EXE ist **nicht** mit echtem Creative-Klang gleichzusetzen.
- **Aktueller Originalkern:** `integration/patch_dosbox_runtime.py` erzeugt nur im Windows-CI-Arbeitsverzeichnis Quellpatches (Original-DOSBox-X-MPU401/EMU8000/Mixer/SDL2), das Submodul bleibt fest gepinnt. Die DOSBox-X-DLL wird im VST3-Bundle neben dem Host geladen. Der originale Creative-DOS-Guest muss für echte Klangtests privat bereitstehen. SDL-Fenster und echter Windows-Audioausgang dürfen im VST3 **nicht** erscheinen; nur P165-GUI und Audio über DAW.
- **WICHTIG: Original VS-Projekt MUSS als `Release SDL2|x64` gebaut werden**, nicht als `Release|x64` (dies wäre SDL1 und ignoriert Hidden-Window- sowie Audio-Dummy-Funktionen). Die interne DOSBox-X-Audioausgabe darf keine separate Windows-Soundhardware öffnen.
- **Originale P165/P007Z-Hardware-GUI** in `gui/` erhalten: 1000×708-Frontplatte, originale Hardware-Reglergrafiken, nachträglich abgedunkelte Reglergruppen `CColor(7,11,16,186)` (73 %). Kein neues Design.

## Einfache Entwicklung, verbindlicher Versionsstand
- **Nur `main`**. Keine zusätzlichen Feature-, Forschungs-, Fix- oder Phasenbranches.
- Bei **jeder** Änderung an Code, GUI, Buildsystem oder Submodul **im selben Commit** `AKTUELLE-VERSION.md` und `CHANGELOG.md` aktualisieren. Versionsworkflow darf nicht umgangen werden.
- `AKTUELLE-VERSION.md` nennt die **neueste tatsächlich nachgewiesene Version** und den Prüfstand. `CHANGELOG.md` führt Änderungen und Prüfergebnisse chronologisch. Commit-Head im Zweifel direkt aus Git ermitteln.
- Keine fingierten erfolgreichen Tests, keine Version für bloß erfolgreiche Kompilierung als fertiges Plugin ausgeben. `implementiert`, `kompiliert`, `getestet`, `abgenommen` sind unterschiedliche Zustände.
- Zum 23.11.2026 ist `0.0.0-dev` nur die Entwicklungskennung. **DOSBox-X-DLL/VST3-Bundle und ein DOS-Shell-Boot sind gebaut und getestet, aber Original-Creative-DOS-SF2/EMU8000-Audio in diesem VST3 noch nicht nachgewiesen.**
- Konsistenz von Plugin-Metadaten, Installer, Release-ZIP, Git-Tag, Dokumentation und Plugin-Anzeige vor einer tatsächlichen Releasefreigabe herstellen.

## Private Originaldaten
- `awe32.raw` (ROM), `SYNTHGM.SBK` und zahlreiche SF2 befinden sich in der **privaten ChatGPT-Dateibibliothek**. Exakte Dateinamen/Größen in `docs/TEST-ASSET-LOCATION.md`. **Vor erneuter Uploadbitte zuerst dort suchen.**
- Codex, GitHub Actions und externe Hosts haben **nicht automatisch Zugriff** auf diese private Bibliothek. Eine lokale Testumgebung benötigt gesonderten **privaten** Zugriff.
- Wichtig: die originale **Creative-SF2LIB-`SYNTHGM.SF2` (42.726 Bytes)** aus `DOS/DEMO` ist **nicht** identisch zur vorhandenen `SYNTHGM.SBK` (34.832 Bytes); `DEMO.EXE` erwartet tatsächlich **`/SBK:`**, nicht das irreführend dokumentierte `/SF2:`.
- **Niemals** proprietäre Creative-ROMs, originale Creative-Treiberprogramme, private SF2/SBK, Windows-Images oder private Testdaten in das öffentliche GitHub, öffentliches CI, Artefakte oder Logs schreiben. `PRIVATE/` ist gitignoriert.
- GPL und Rechte an Creative-Software vor jeder Veröffentlichung klären.

## Abnahmebedingungen
1. Originalen Creative-DOS-Laufzeitpfad im echten **in-process** DOSBox-X-VST3 nachweisen; MIDI bis zu den EMU8000-Registern und echter PCM-Ausgabe messen.
2. Original-ROM, `SYNTHGM.SBK` und private SF2 nachweislich korrekt laden; originale Klangcharakteristik und ADSR-/Filter-/Effekt-Register vergleichen.
3. 16 MIDI-Kanäle; Bank-/Program Change, Sustain, Controller, Pitchbend/RPN, Sample-RAM 28 MiB, Sample-Offset-Timing und 0-%-Send-Verhalten testen.
4. 44,1/48/96 kHz, Mehrfachinstanzen, RT-Sicherheit ohne Heap-/I/O im Audio-Callback, Projekt-Save/Recall und Automation verifizieren.
5. **Jedes GUI-Bedienelement muss echte Audio-/Registerwirkung haben**, bevor „fertig“ behauptet wird.
6. Steinberg-Validator plus echte Windows-VST3-Hosttests prüfen. Validator-PASS allein beweist **keinen** Originalklang.
7. **Studio-Pro-Praxistest zuletzt**, nach den automatisierten technischen Prüfungen. Kein voreiliger Release.

## Arbeits- und Kommunikationsregeln
- Ziel ist **ein einziges verlässlich funktionierendes Plugin**. Keine neuen Pluginideen, parallelen Architekturen oder P001–P999-Branch-Flut.
- Der Projektinhaber **will keine Zwischenberichte**. Erst konkrete abgeschlossene Ergebnisse bzw. ein endgültig festgestelltes Hindernis mitteilen. Kein behauptetes Hintergrundarbeiten und keine künstlichen Zeitversprechen.
- Nach einem neuen Chat **nicht bei Null anfangen**: aktuelle `main`-Version, `AGENTS.md`, `START-HERE.md`, Changelog, originale GUI und dokumentierte private Testdateien zuerst prüfen.
