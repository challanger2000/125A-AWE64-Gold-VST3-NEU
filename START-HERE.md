# START HERE – 125A AWE64 Gold VST3

**Verbindliche aktuelle Version und echte Prüfergebnisse:** [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md).  
**Anweisungen für Codex und spätere Chats:** [AGENTS.md](AGENTS.md).  
**Chronologischer Änderungsverlauf:** [CHANGELOG.md](CHANGELOG.md).  
**Private ROM-/Creative-/SF2-Fundorte:** [docs/TEST-ASSET-LOCATION.md](docs/TEST-ASSET-LOCATION.md).

## Festgelegte Architektur (gemeinsame Entscheidung)

Ein einziges Windows-x64-VST3 mit internem gepinntem **DOSBox-X AWE32 + originalem Creative-DOS-SF2LIB/DEMO.EXE**. Originale P165-GUI mit nachträglich um 73 % abgedunkelten Reglergruppen. **Kein Win95/86Box-PC**, keine getrennt zu startende DOSBox-EXE, kein eigener nativer Ersatz-SF2-Player.

**Submodul:** `third_party/dosbox-x-AWE32` auf `f3a95b9f19a7f9863295e6481f271cf04733c635` gepinnt. **Einziger Branch:** `main`.

## Neue echte Windows-VST3-Klang-Negativkontrolle (Stand 23.11.2026)

Zur positiven synthetischen MIDI-zu-PCM-Prüfung gibt es jetzt eine **echte Windows-VST3-Negativkontrolle**: Das identische öffentliche DOS-Testprogramm darf im privaten Creative-only-Modus **keinen hörbaren PC-Speaker- oder OPL-Ton** mehr erzeugen. Der native Steinberg-Host muss `nonzero=0` am echten VST3-PCM-Ausgang bestätigen; andere Fehler zählen nicht als Erfolg. Code und Workflow eingereicht, **der neue Windows-Integrationstest muss noch PASS melden**. Original-Creative-ROM-/SF2-Klang steht weiter aus, `0.0.0-dev`.

## Neue Original-Creative-VST3-Klangprüfung: Alternativtonquellen im Test deaktiviert

Im **privaten** Original-Creative-VST3-Hosttest werden jetzt auch im tatsächlichen VST3-Gast PC-Speaker und OPL abgeschaltet (`AWE64_ORIGINAL_CREATIVE_ONLY=1`); der Testhost verlangt außerdem `SDL_AUDIODRIVER=dummy`. Öffentliche MIDI-/Audio-Synthetic-Gates bleiben unverändert. Quelländerung und neue Regression sind implementiert, **neuer CI-Lauf noch zu bestätigen**; echter Besitzer-Creative-ROM/SF2-Klangtest weiterhin nicht ausgeführt. `0.0.0-dev`, nur `main`.

## Aktuell neu bestätigter Windows-Release-Build (23.11.2026)

**[Windows 45205416871](../../actions/runs/45205416871) SUCCESS:** Neu erweiterter Windows-VST3-Testhost mit privatem Creative-`--original`-Modus tatsächlich kompiliert, weiterhin echter synthetischer MIDI-zu-PCM-VST3-Rundweg bestanden (979 nicht-stumme Stereo-Frames, 7 NoteOn). [Linux/Windows 45205416646](../../actions/runs/45205416646) PASS. **Original-ROM/Creative-`DEMO.EXE`/echte `SYNTHGM.SF2` wurden dabei nicht ausgeführt.** Privater Originalklang weiterhin nicht abgenommen; `0.0.0-dev`.

## Neuester geprüfter Stand (23.11.2026)

**[Linux-/Windows-Prüflauf 45215251617](../../actions/runs/45215251617) vollständig PASS:** neue private PowerShell-Builder-Quellregression, tatsächliche Windows-PowerShell-Parserprüfung der beiden Owner-only-Skripte und sämtliche datenfreien Audio-Bridge-Tests. Versions-/Changelog-Konsistenz ebenfalls PASS [45215251656](../../actions/runs/45215251656). **Die originalen Creative-ROM/SF2/DEMO-Klangtests sind damit nicht ausgeführt.** Nur `main`, `0.0.0-dev`.

## Teststand: SHA256-Contract-Assertion korrigiert

Der erste synthetische Builder-Test ([45214831987](../../actions/runs/45214831987)) hatte eine falsche Textassertion (`SHA-256` gegenüber der tatsächlichen `-Algorithm SHA256`-Syntax). Die Assertion ist korrigiert. **Der Besitzer-Windows-Original-Creative-Klangtest selbst bleibt unausgeführt**, `0.0.0-dev`.

## Reproduzierbare Original-Creative-Klangabnahme lokal vorbereitet (noch KEIN PASS)

`tools/Build-And-Test-Private-AWE64.ps1` führt auf dem Besitzer-Windows-PC den gesamten gepinnten DOSBox-X-/VST3-Build und danach das echte, private Creative-DOS-Klanggate aus. Es hält **das originale Submodul sauber**, kompiliert in `PRIVATE/working-upstream` und gibt keine Originaldateien weiter. Benötigt originale ROM-Datei, Windows x64 mit Visual Studio 2022/CMake/Python/Git und Zugriff auf den öffentlichen Steinberg-SDK-/Creative-Quellcode. **In dieser Sitzung nur die Quell-/Syntaxprüfungen, keine echte Originaldatei-Audioausführung.**

## Privater Creative-Klangtest: OPL/PC-Speaker ausgeschlossen (Stand 23.11.2026)

Im **privaten Original-Creative-DOSBox-DLL-Test** sind PC-Speaker und OPL ausgeschaltet; nur echte EMU8000-I/O-Portschreibzugriffe (mindestens 16 auf 2 Ports) zählen. Der synthetische öffentliche VST3-Audiotransport wird dadurch **nicht** verändert. Die echte Creative-`DEMO.EXE`/`SYNTHGM.SF2`/Besitzer-ROM-Klangabnahme **steht weiter aus**.

## Privater Original-Creative-VST3-Klangtest jetzt ausführbar vorbereitet (noch kein PASS)

Der reale VST3-Host besitzt jetzt einen separaten privaten `--original`-Modus für das **echte** Creative-`DEMO.EXE`, authentische `SYNTHGM.SF2` und `awe32.raw`. Der Besitzer-Windows-Launcher `tools/Run-Private-Creative-AWE-Test.ps1` fordert SHA-verifizierte Originaldaten, echten MIDI-NoteOn/NoteOff, originale EMU8000-Registerwrites und hörbare Stereo-PCM-Frames **aus dem tatsächlichen VST3**. Öffentliche CI bleibt vollständig datenfrei. **Dieser Original-Creative-Test wurde noch nicht durchgeführt und ist NICHT abgenommen.** Alle vorherigen synthetischen VST3-Transporttests bleiben davon getrennt.

## Neuester integrierter Windows-MIDI-und-PCM-VST3-PASS (23.11.2026)

**[Windows-Actions 45201802002](../../actions/runs/45201802002): vollständiger Erfolg.** Im echten VST3-Instrument laufen jetzt **7 MIDI-NoteOn-Events durch Steinberg-`IEventList` → Original-DOSBox-X-MPU401 (331h/330h) → synthetisches 16-Bit-DOS-PC-Speaker-Programm → originalen DOSBox-X-Mixer → echten VST3-`process()`-Stereoausgang**. Gemessen **59.904 Float32-Stereoframes, 982 nicht-stumm, Spitzenpegel 0,304871**, Stille vor NoteOn geprüft. Linux-/Windows-Regressionen ebenfalls bestanden. **Der Original-Creative-DOS-SF2/ROM-Klang ist damit weiterhin NICHT nachgewiesen**, erfordert getrennten privaten Test. Kein Release, Entwicklungskennung `0.0.0-dev`. Verbindlich: [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md).

## Aktueller erster VST3-`process()`-PCM-End-to-End-PASS (23.11.2026)

**[Windows 45197768158](../../actions/runs/45197768158): SUCCESS.** Der echte Windows-x64-VST3-Host hat das gebaute Plugin über die Steinberg-Factory aktiviert und seinen Audio-Callback ausgeführt: **80.384 Float32-Stereoframes, 568 nicht-stumme Frames, Peak 0,304871**. Quelle war die emulierte **DOS-PC-Speaker-Hardware**, nicht der EMU8000. Originale ROM-, `DEMO.EXE`- und `SYNTHGM.SF2`-Dateien sind hier **bewusst nicht vorhanden**; ein vollständiges Creative-AWE64-Instrument ist noch nicht freigegeben. `0.0.0-dev`, `main`, Original-P165-GUI unverändert. Weitere Details in [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md).

## Zusätzlicher tatsächlich bestandener Original-DOS-Audio-Test (23.11.2026)

- **[Windows 45189311093](../../actions/runs/45189311093) PASS:** echtes PC-Speaker-PIT-Tonprogramm innerhalb des originalen DOSBox-X-Gasts, **68.608 originale Mixer-PCM-Stereoframes**, **17.633 nicht-stumm**, PCM16-Peak **9.990** und ausschließlich internem SDL2-Dummy-Audiogerät. Original-EMU8000-Registerwrites und MPU401 weiter PASS; Host/Runtime-Windows-VST3-Bundle gebaut.
- **[Linux/Windows 45189310952](../../actions/runs/45189310952) PASS:** Originalgerät-spezifische synthetische Tests. Ein PC-Speaker ist **nicht** der EMU8000 und beweist **keinen** echten Creative-SF2-Sound im VST3. Release bleibt NO-GO.

## Neue echte Hardware-Abnahme (23.11.2026)

- **[Windows-Prüflauf 45186047184](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45186047184) vollständig erfolgreich:** Original-DOSBox-X startet jetzt die echte AWE32/EMU8000-Hardware; 9.688 tatsächliche EMU8000-Portschreibvorgänge durch DOSBox-`AWEUTIL /S` nachgewiesen. Originaler MPU-401, separater SDL2-Dummy-Audioweg, kontrollierter Stop und Windows-VST3-Bundle-Build ebenfalls PASS. In öffentlicher CI nur synthetisches Nullen-ROM, kein Original-Creative-Klang.
- **Kritischer Fix:** `-silent` war im echten VST3-Start aktiv und hatte nachweislich den originalen Sound-Blaster-/AWE32-Kern deaktiviert (`SBT_NONE`). Entfernt; `-nogui -nomenu` bleiben. Linux- und Windows-Negativregressionen bestanden.
- **Noch offen:** Tatsächlich hörbares `DEMO.EXE /SBK:SYNTHGM.SF2` mit privatem authentischen `awe32.raw` in **demselben VST3**. Kein freigegebenes Instrument; `0.0.0-dev`.

## Tatsächlich gebaut

- Der **Original-DOSBox-X-AWE32-Quellcode** ist als interne Windows-x64-DLL gebaut. Ein originaler DOSBox-DOS-Befehl wurde im Windows-Testprozess ausgeführt und der Emulator verließ den Prozess nicht.
- Die **ursprüngliche P165-GUI**, der Windows-VST3-Host und die originale DOSBox-X-DLL wurden bereits zusammen in einem einzigen VST3-Bundle kompiliert.
- Original-MPU401-MIDI-Anbindung, PCM-Bridge, verstecktes DOSBox-SDL-Fenster und internes audiofreies SDL-Backend sind implementiert; Windows-Integrations- und I/O-Tests wurden tatsächlich bestanden ([vollständige Windows-Integration 45175109078](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45175109078)); **der echte Creative-DOS-/ROM-/SF2-Klangtest ist weiterhin offen**.
- **Noch KEIN fertiges Instrument:** Creative-DOS-Programm mit den echten privaten Originaldaten innerhalb des VST3, echter AWE-Sound, State/Host/Studio-Pro-Abnahme stehen weiterhin aus.

## Stand der neuen VST3-Hostprüfung

- Der echte Windows-x64-VST3-Bundle-Build und das separate Steinberg-VST3-Host-`process()`-Testprogramm **kompilieren**. Im aktuellen Lauf 45195522027 wurde der tatsächliche Hostaufruf jedoch durch den schreibgeschützten PowerShell-Variablennamen `$Host` verhindert. Das neue Skript nutzt `$hostTestExe`; eine Negativregression schützt davor. Das erneute Windows-Testergebnis muss noch tatsächlich abgewartet werden.
- Originales ROM, `DEMO.EXE` und `SYNTHGM.SF2` lassen sich mit `tools/Run-Private-Creative-AWE-Test.ps1` ausschließlich lokal auf dem Besitzer-PC verifizieren und in ein bereits gebautes Entwicklungsbundle übernehmen. **Kein privater Originalklangtest wurde in diesem Chat bereits ausgeführt.**

## Fortsetzung – ausschließlich dieser Pfad

1. Neuesten Actions-Lauf zur originalen DOSBox-X-In-Process-Laufzeit und den echten DOS-UART-MIDI-Tests prüfen und etwaige Fehler direkt korrigieren.
2. Private Original-ROM-, `DEMO.EXE`- und `SYNTHGM.SF2`-Dateien lokal bereitstellen und originalen Creative-Klang **im VST3**, nicht bloß im DOS-Shell-Smoke-Test, nachweisen.
3. SF2-Auswahl/GUI-Parameter und projektfesten Recall zuverlässig implementieren; Clock/PCM/Latenz/Audio-Sends und sämtliche Abnahmetests durchführen. Studio-Pro-Praxistest zuletzt.
4. Erst danach freigeben; Versionsnummer, Installer, Git-Tag und Download müssen identisch sein.

**Keine neuen Branches, keine Wiederaufnahme der alten Win95/86Box/P009-Wege, keine Zwischenberichte und kein Paket mit falscher Fertigmeldung.**
