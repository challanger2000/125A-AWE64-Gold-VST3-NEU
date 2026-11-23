# Änderungsverlauf

## 23.11.2026 — Echte Windows-VST3-Negativkontrolle: POSITIV und NEGATIV bestanden

- Der bestehende echte Windows-VST3-MIDI→DOSBox-X→Mixer→PCM-Positivtest wird um eine **direkt folgende echte Negativkontrolle** ergänzt. Bei unveränderter synthetischer NoteOn-DOS-`DEMO.EXE` und Null-ROM muss der explizite private `AWE64_ORIGINAL_CREATIVE_ONLY=1`-Modus **PC-Speaker und OPL ausschalten** und das reale VST3-`process()` genau **0 nicht-stumme Frames** liefern.
- Neues `tests/WindowsVst3CreativeOnlyNegative.py` ist ein zeitlich begrenzter echter Windows-VST3-Testhost-Kindprozess. Ein Fehler gilt **nur dann als erwartetes Negativergebnis**, wenn der native Testhost tatsächlich durchgelaufen ist und das ausdrücklich nachgewiesene Null-Audio-Ergebnis meldet. Absturz, Startfehler oder zufällig fehlendes Testprogramm führen ausdrücklich zu FAIL. Kein Original-EMU8000-Klang-PASS daraus ableiten.
- **[Windows-Actions 45260688380](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45260688380): SUCCESS.** Echter VST3-Positivtest mit 63.488 Stereo-Frames, 1.011 nicht-stummen, Float32-Peak 0,304871 und 8 NoteOn; unmittelbar danach das gleiche synthetische DOS-Programm im privaten Creative-only-Modus mit **nachweislich exakt 0 nicht-stummen Frames**, erwartetem SILENCE-Fehler und Marker `REAL_VST3_PRIVATE_CREATIVE_ONLY_SILENCES_PC_SPEAKER_PASS`. Vollständige originale DOSBox-DLL-/EMU8000-Register-/MIDI-/Audio-Integrationskette weiter PASS. [Linux/Windows 45260688317](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45260688317) Regressionen SUCCESS; [Version 45260688373](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45260688373) SUCCESS. Die reine Tonquelle bleibt synthetische DOS-PC-Speaker-Hardware; **Original-Creative-SF2-Klang nicht getestet**, `0.0.0-dev`, kein Release.


## 23.11.2026 — Tatsächlichen Original-Creative-VST3-Klangtest gegen PC-Speaker-/OPL-Falschtöne abgesichert

- **Bisherige Lücke beseitigt:** PC-Speaker und OPL waren nur beim separaten ursprünglichen DOSBox-DLL-Soundcheck stumm; der `--original`-VST3-Hostcheck konnte alternative DOSBox-Klangquellen noch nicht ausschließen. Im VST3-DOSBox-Boot werden jetzt **nur im privaten `AWE64_ORIGINAL_CREATIVE_ONLY=1`-Abnahmemodus** beide deaktiviert; öffentlicher MIDI→PC-Speaker→VST3-Test und normaler Produktstart bleiben im Klangpfad unverändert.
- Der private native Steinberg-Host lehnt Originaltestbetrieb ohne den expliziten Creative-only-Modus und `SDL_AUDIODRIVER=dummy` ab. Der Besitzer-PowerShell-Starter setzt und restauriert beide Umgebungsvariablen zusammen mit dem Original-EMU8K-Trace-Protokoll.
- Zusätzlicher Linux-/Windows-Unit-Contract für die unterschiedlichen Klangquellen und den unmöglich gemachten stillen Rückfall. Echte `DEMO.EXE`/`SYNTHGM.SF2`/`awe32.raw`-Wiedergabe noch nicht privat ausgeführt, daher weiterhin `0.0.0-dev`, kein Release. Neue CI-Ergebnisse noch ausstehend.


## 23.11.2026 — Original-Creative-Privat-Build: Linux-/Windows-Syntax- und Quellvertrag vollständig bestanden

- [GitHub-Actions 45215251617](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45215251617) **SUCCESS** (Ubuntu + Windows): Vollständige bestehende Audio-/MIDI-C++-Suite, neue datenfreie Private-Builder-Sicherheitsregression und echte Windows-PowerShell-**AST-Syntaxprüfung** für `Build-And-Test-Private-AWE64.ps1` sowie `Run-Private-Creative-AWE-Test.ps1` erfolgreich.
- [Versionslauf 45215251656](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45215251656) **SUCCESS**. Den vorherigen rein statischen SHA256-Schreibweisenfehler korrigiert, keine Produkt-DSP-/Original-EMU8000-/P165-GUI-Änderungen.
- **Noch kein Original-Creative-Klang-PASS:** Diese öffentliche CI führt keine owner-only-ROM-/Creative-Originaldateien aus. Ein echter Klangnachweis erfordert den lokalen privaten Windows-Gesamtlauf. `0.0.0-dev`, nur `main`, kein Release.


## 23.11.2026 — Falsche SHA-256-Textassertion im neuen Privatskript-Test behoben

- Der [initiale Linux-/Windows-Test 45214831987](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45214831987) schlug **im neuen Static-Test** fehl, weil die Prüfung eine wörtliche Bindestrichschreibweise `SHA-256` forderte, während die echte Windows-PowerShell-API die gültige Syntax `Get-FileHash -Algorithm SHA256` verwendet. Die Regression fordert nun die tatsächliche SHA256-Operation inklusive `-LiteralPath`, ohne einen Produktions- oder EMU8000-Codepfad zu verändern.
- Der private Besitzer-Originalklang-Lauf wurde dadurch **nicht** ausgeführt. Neuer Windows-PowerShell-Parser- und Linux-/Windows-Regressions-PASS bis zum Abschluss offen. `0.0.0-dev`, nur `main`.


## 23.11.2026 — Private reproduzierbare Windows-Gesamtprüfung ab Quellcode

- `tools/Build-And-Test-Private-AWE64.ps1` automatisiert auf dem **Besitzer-Windows-Host** den vollständigen bisherigen Windows-CI-Build: gepinnte Original-DOSBox-X-AWE32-SDL2-DLL (`f3a95b9`), SDK 3.7.12, Original-P165-Windows-VST3 samt nativer Steinberg-Audiotest-EXE, Zusammenführung im **einen** VST3-Bundle. Anschließend alle drei authentischen Creative-`DEMO.EXE`-/`SYNTHGM.SF2`-/Wave-ROM-Hashprüfungen und der echte private EMU8000-Port-/MIDI-/PCM-Test.
- Der private Builder klont das gepinnte Original-Submodul lokal nach **`PRIVATE/working-upstream/`**; ABI-Patches und Native-Build sind ausschließlich in diesem gitignorierten Arbeitscheckout erlaubt, sodass Original-Submodul und einziger Branch `main` **nicht** beschmutzt oder zurückgesetzt werden. Keine neuen Repos, Zweigstellen, externen DAW-Hosts oder Veröffentlichungsdateien.
- Schnell laufende datenfreie Linux-/Windows-Quellregression und Windows-`PowerShell Parser.ParseFile`-Syntaxprüfung für beide privaten Skripte ergänzt. **Auch bei bestandenem Syntax-/CI-Gate gilt: Original-Creative-Klang im echten VST3 wird erst durch privaten Besitzer-Windows-Lauf bewiesen; noch nicht durchgeführt.** `0.0.0-dev`, Release NO-GO.


## 23.11.2026 — Original-Creative-Klangtest gegen PC-Speaker-/OPL-Falschtreffer abgesichert

- Im privaten echten Creative-DOSBox-DLL-Klangtest werden jetzt **PC-Speaker und OPL explizit deaktiviert**, während `sblaster sbtype=sbawe` und der Original-EMU8000-Kern weiter aktiv sind. So ist hörbares DOSBox-Mixer-PCM nicht mehr durch andere emulierte Hardwaretöne erklärbar. Keine Änderung an Produktstart, öffentlich-synthetischem PC-Speaker-Test oder originalem Chipcode.
- `EMU8K_TRACE` zählt nur noch echte Original-EMU8000-I/O-Writes auf 620h/622h/A20h/A22h/E20h/E22h, fordert **mindestens 16 Writes auf wenigstens 2 Ports** und weist gewöhnliche MIDI-/OPL-/sonstige Register sowie Reads zurück.
- Neue datenfreie Python-Regressionsfälle für die Stummschaltungen und Trace-Negativkontrollen. **Keine private Originaldaten-Audioabnahme behauptet; erst ausgeführter Besitzer-Windows-Test zählt**, `0.0.0-dev`.


## 23.11.2026 — Windows-VST3-Build und neuer privater Original-Creative-Testmodus: CI bestanden

- **[45205416871](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45205416871): Windows-Original-DOSBox-X-DLL und tatsächliche Steinberg-VST3-Audio-Host-EXE mit neuem owner-only `--original`-Modus **erfolgreich kompiliert**. Öffentlicher synthetischer NoteOn→MPU401→DOS-Hardware→Original-Mixer→VST3-PCM-Test weiterhin PASS (60.928 Float32-Stereoframes, 979 nicht-stumme, Peak 0,304871, 7 NoteOn). Original-EMU8000-`AWEUTIL`, MPU und Windows-CI-Stop ebenfalls PASS.
- **[45205416646](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45205416646)**: Linux/Windows-Datenfrei-Regressionen PASS inklusive Verhinderung von Originaldatei-Nutzung in öffentlicher CI; [45205416611](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45205416611) Versionsprüfung PASS.
- Privaten PS-Prüfablauf präzisiert: `PRIVATE_CREATIVE_VST3_EMU8000_PCM_PASS` wird **erst nach sämtlichen drei Bedingungen** (originale hashes, tatsächliches VST3-PCM mit EMU8000-Portwrites und unabhängiger Original-Creative-DOSBox-DLL-Klangprüfung) ausgegeben. Kein Status vor Abschluss vorzeitig als erfolgreich markieren.
- **Nicht verwechselt werden:** Öffentlicher PC-Speaker-Ton im echten VST3 ist noch nicht der Original-Creative-SF2-Klang. Private Besitzer-ROM-/Creative-DOS-Original-Klangabnahme auf Windows steht aus, `0.0.0-dev` und NO-GO für Release.


## 23.11.2026 — Private Original-Creative-Klangprüfung durch echten VST3-Host integriert

- Separater **`--original 15..120`-Modus** in `WindowsVst3HostPcmSmoke.cpp`: authentisches Owner-ROM, echte Creative-`DEMO.EXE` und `SYNTHGM.SF2` im **tatsächlich ausgeführten VST3** statt des öffentlichen synthetischen PC-Speaker-Gasts. VST3-`IEventList` sendet C4 Note-On/Note-Off über originale DOSBox-X-MPU-401 an unveränderte Creative-DOS-SF2LIB; reale VST3-`process()`-Audioausgabe mit Stille-Negativkontrolle und niedrigerem Original-Preset-Mindestpeak `0,002`.
- Private PowerShell-Abnahme führt nun zuerst den echten VST3-Host-Prozess isoliert mit Timeout aus und verlangt echte nicht-stumme PCM, danach tatsächliche Original-EMU8000-Registerwrites an mindestens zwei Original-I/O-Ports. Anschließend unabhängig der bestehende originale DOSBox-X-DLL-/Creative-Klangtest. Das private Originalmaterial wird per festen SHA-/Git-Blob-Hashes validiert und **niemals überschrieben oder in GitHub Actions hochgeladen**.
- Plattformsichere **datenfreie** Regression sichert `--original`-Modus, NoteOff, privaten Trace-/Hash-Nachweis und die strikte Nichtnutzung des Originalmodus in öffentlicher CI. Original-P165-GUI, Original-DOSBox-X-AWE-Kern und Produktklangcode bleiben unangetastet. **Noch kein privater Klang-PASS**; erst echte Windows-Ausführung zählt. `0.0.0-dev`, Release-NO-GO.


## 23.11.2026 — Workflow-Name nach bestandener echter VST3-Integration präzisiert

- Der nachweislich bestandene Original-DOSBox-X-/VST3-`process()`-Test [45201802002](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45201802002) wird nicht länger unter dem widersprüchlichen Workflow-Titel `NO VST3 PLAYBACK` angezeigt. Neuer Titel `DOSBox-X original + VST3 MIDI/PCM end-to-end (synthetic DOS)` benennt den **wirklich** geprüften bi-direktionalen MIDI-/PCM-Pfad und zugleich dessen synthetischen DOS-PC-Speaker-Gast.
- Keine Änderung am VST3-Produkt, EMU8000-/DOSBox-X-Chipkern, P165-GUI, Testdaten oder Originaldateien. Immer noch `0.0.0-dev`; privater Creative-Originalklang und DAW-Abnahme nicht erfolgt.


## 23.11.2026 — Echte bidirektionale VST3-MIDI↔Original-DOSBox-I/O-PCM-Abnahme: Windows-PASS

- **[Windows-Integrationslauf 45201802002](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45201802002) vollständig SUCCESS**: echtes Windows-VST3 über `GetPluginFactory`/`IAudioProcessor::process()` mit `IEventList` betrieben, **7 Steinberg-VST3-NoteOn-Events** durch den Original-DOSBox-X-MPU401-UART (Port 330h/331h mit 3Fh/FEh) an ein eigenständiges DOS-MZ-Programm. Diese schaltet nur bei passender Note die original emulierte PIT-/PC-Speaker-Hardware frei; vorher fordert der Host Stille.
- Im selben Pluginprozess nachweislich **59.904 Stereo-Float32-Frames**, davon **982 nicht-stumme**, Peak **0,304871** am wirklichen VST3-Ausgang; originale Mixer-PCM, STOP, Release, kein NaN/Clip **PASS**. Ebenso Original-EMU8000-Registertest (**9.688 reale Writes**) und originale MPU-/Mixer-/Windows-x64-DLL-Tests **PASS**.
- [Linux-/Windows-Datenfrei-Release-Prüfung 45201802160](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45201802160) **SUCCESS**, [Changelog-Konsistenz 45201801967](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45201801967) **SUCCESS**. Original-P165-GUI und DOSBox-X-Chipkern bleiben unangetastet, weiterhin nur `main`.
- Originale Creative-DOS-`DEMO.EXE`, echtes Besitzer-`awe32.raw` und `SYNTHGM.SF2` waren aus Privacy-/Lizenzgründen **nicht** in der öffentlichen CI. Der bisherige MIDI-gesteuerte PC-Speaker-Ton ist **kein** AWE64-GM-SoundFont-Nachweis. Kein Fender-Studio-Pro-Test und kein Produkt-Release; `0.0.0-dev`.


## 23.11.2026 — Echte VST3-Host-Audioausgabe aus Original-DOSBox: vollständiger Windows-PASS

- **[Windows-Actions 45197768158](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45197768158) SUCCESS.** Reales Windows-`AWE64GoldDOSBoxDev.vst3` über tatsächliche Steinberg-`GetPluginFactory`/`IComponent`/`IAudioProcessor` aktiviert, `process()` bei **44,1 kHz / Float32** ausgeführt und echte nicht-stumme PCM durch das VST3 empfangen. **80.384 Stereo-Frames**, **568 nicht-stumme Frames**, **Peak 0,304871**; Deaktivierung, Beenden und Freigabe ebenfalls PASS.
- Echtes unverändertes Original-DOSBox-X-AWE32-Audiogerät, originaler Mixer und VST3-PCM-Bridge; Tonquelle ausschließlich eigenständig erzeugtes DOS-PC-Speaker-Programm. Keine andere Host-Ton-Engine, keine privaten Creative-ROM-/SF2-Bytes im öffentlichen CI-Workflow. Testbezeichnung `SYNTHETIC_DOS_FIXTURE_REAL_VST3_HOST_PCM_PASS` bedeutet **nicht** Original-Creative-Klang.
- Im selben Windows-Lauf originale EMU8000-AWEUTIL-Registertests (**9.688 reale Registerwrites**), 16-Bit-DOS-MPU-401, PCM-Transport, Worker-Stop und vollständige VST3-Kompilierung **PASS**. [Release-Regressionen 45197768094](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45197768094) auf Linux und Windows PASS, [Dokumentationsprüfung 45197768039](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45197768039) PASS.
- **NO-GO für Auslieferung:** Original-Creative-DOS-`DEMO.EXE`, echtes Owner-`awe32.raw`, authentische `SYNTHGM.SF2` und Original-AWE-MIDI-Klang im VST3 bisher **nicht nachgewiesen**; Fender Studio Pro erst nach abschließenden weiteren Prüfungen. `0.0.0-dev`, P165-GUI unverändert; nur `main`.


## 23.11.2026 — Schützendes VST3-Host-Gate: PowerShell-`$Host`-Variable ersetzt und DLL-Buildcache eingerichtet

- [Windows 45195522027](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45195522027): originale DOSBox-X-AWE32-Windows-DLL und eigentlicher VST3-Host samt eigenem Hosttest-C++ **erfolgreich kompiliert**; originale EMU8000-I/O-, MPU-UART- und Nicht-Stille-PC-Speaker-Tests PASS. Das neue VST3-`process()`-Audio-Gate **FAIL ohne Testdurchführung**, da `$host` in PowerShell schreibgeschütztes `$Host` referenziert.
- **Quellfix:** Workflow verwendet `$hostTestExe` statt `$host`. Neue plattformübergreifende Regex-Negativregression unterbindet diese PowerShell-Fehlerklasse.
- **CI-Beschleunigung:** GitHub Actions speichert die **aus Original-GPL-DOSBox-X-Quellen compilierte, IP-freie DLL** unter einem Hash des exakten Quellpatchskripts und des gepinnten Originalcommits. Cache-Speicherung erfolgt sofort nach erfolgreicher DLL-Kompilierung, auch wenn ein nachgelagerter Hosttest fehlschlägt. Niemals privates AWE-ROM, Creative-DOS-Binary oder SF2 im öffentlichen Cache.
- Neuer Lauf noch ausstehend; Original-Creative-Sound und echte VST3-Host-PCM noch nicht nachgewiesen. `0.0.0-dev`.


## 23.11.2026 — Compilerbefunde beim neuen Steinberg-VST3-Hosttest behoben; Originaldaten privat bereitstellbar

- [Windows-Integrationslauf 45193354491](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45193354491) **FAIL**: original DOSBox-X/EMU8000/MPU/PC-Speaker-PCM weiterhin **PASS**, aber der neu hinzugefügte VST3-Testhost kompiliert nicht wegen irrtümlichem `Steinberg::TChar` (korrekt `Steinberg::Vst::TChar`) und Windows-`min/max`-Makrokollision. Beide Quellfehler im Testhost korrigiert, Original-Emu-Kern und Produkt-DSP bleiben unverändert.
- Portables [Release-Gate 45193355496](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45193355496) **PASS auf Linux/Windows** und Version [45193355170](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45193355170) PASS. Neuer vollständiger Windows-VST3-Hosttest bis zum Ergebnis **offen**.
- `tools/Run-Private-Creative-AWE-Test.ps1` integriert: nur lokal auf Besitzer-Windows, ursprüngliches ROM/SF2LIB/`DEMO.EXE` durch SHA-256 und Git-Blob absichern, gegebenenfalls aus öffentlichem Creative-Archiv **herunterladen**, niemals hochladen oder bestehendes Eigentümermaterial überschreiben. Führt das separate private Original-DOS-/EMU8000-Klang-Gate aus; **noch kein privater PASS**.
- Eine Windows-CI-**Syntax**prüfung des PowerShell-Skripts hinzugefügt; keine privaten ROM-/Creative-Dateien in CI.


## 23.11.2026 — Echten VST3-IPluginFactory/IAudioProcessor-Audio-End-to-End-Test integriert (Ergebnis offen)

- `WindowsVst3HostPcmSmoke.cpp`: tatsächliche Windows-VST3-Factory instanziieren, Instrument aktivieren, 44,1-kHz-Float32-Ausgabe über `process()` empfangen, nicht-stumme Audioframes quantifizieren, Host deaktivieren und sauber terminieren. Kein separater DAW-Emulator und keine vom Testhost generierten Audiosamples.
- Für öffentliche CI nur **selbst generiertes und frei verwendbares DOS-MZ-Testprogramm** als klar markierte `DEMO.EXE`-Fixture, 1-MiB-Null-ROM und nichtwertige SF2-Textdatei. Die echte DOSBox-X-AWE32-DLL und das echte Windows-VST3 arbeiten dabei unverändert. Nicht mit echter Creative-`DEMO.EXE`/Original-SF2 verwechseln.
- Schutz vor Überschreiben vorhandener privater Originaldateien; harte Windows-Prozess-Timeout-Grenze. Windows-Workflow baut zusätzlich die separate Host-Test-EXE, startet realen VST3-Prozesspfad und meldet bei Stille/Crash/Timeout ausdrücklich FAIL; Linux-/Windows-Syntax-/Fixture-Regression ergänzt. Neue CI-Ergebnisse vor tatsächlichem Abschluss **offen**, kein Klang-Release.


## 23.11.2026 — Echte nicht-stumme Original-DOSBox-X-Mixer-Audioausgabe + Windows-VST3-Bundle: PASS

- **[Windows-Lauf 45189311093](../../actions/runs/45189311093) vollständig SUCCESS**. Realer originaler DOSBox-X-SDL2-Windows-Emulator führt ein unabhängiges 52-Byte-16-Bit-DOS-Programm aus, das **echte PC-Speaker-/PIT-Hardwareports** steuert. Der original DOSBox-X-Audiomixer hat dabei **68.608 Stereo-PCM-Frames** geliefert, darunter **17.633 nicht-stumme Frames**, Peak PCM16 **9.990**; nur internes SDL2-Dummy-Audiogerät, DOS-Gast-Exit 0.
- Im selben Run auch **9.688 echte EMU8000-Registerwrites** durch `AWEUTIL /S`, originaler MPU-401-16-Bit-MIDI-Test, Gaststart/Stop und Windows-x64-VST3-Bundle-Build mit originaler DOSBox-X-DLL **PASS**.
- **[Linux- und Windows-Regressionslauf 45189310952](../../actions/runs/45189310952): SUCCESS**, [Versionsprüfung 45189310913](../../actions/runs/45189310913): SUCCESS.
- **Beweisgrenze:** Klangquelle dieser öffentlichen, komplett datenfreien Prüfung war **DOS-PC-Speaker**, nicht Creative-EMU8000. Kein echter Creative-`DEMO.EXE`-/`SYNTHGM.SF2`-/ROM-Klang im VST3 getestet; noch keine echte VST3-Host-/Studio-Pro-Abnahme. Weiterhin `0.0.0-dev`, kein Release.


## 23.11.2026 — 9.688 echte EMU8000-Registerwrites / Windows-VST3-Bundle PASS

- Vollständiger [Windows-Actions-Lauf 45186047184](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45186047184) **SUCCESS**: originaler DOSBox-X-AWE32-Chip als in-process-SDL2-DLL, echter DOS-Gast, Hardwarestart per `AWEUTIL /S`, **9.688 originale EMU8000-Registerschreibzugriffe**, originale MPU-401-UART-Bytes, clean shutdown und Windows-VST3-Host samt Original-P165-GUI in einem Bundle gebaut.
- Nach Entfernen des Sound-Blaster-tötenden DOSBox-X-Parameters `-silent` **echter Hardware-Register-PASS statt vorherigem fehlenden Trace**. Das CI-ROM war nur ein nicht-proprietärer 1-MiB-Nullen-Dummy und lieferte **keinen Creative-Klang**. Private ROM-/Creative-/.SF2-Dateien sind nicht hochgeladen worden.
- [Linux + Windows-Release-Regression 45186047103](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45186047103) SUCCESS; [Version 45186047049](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45186047049) SUCCESS.
- **Weiter NO-GO:** Private Creative-`DEMO.EXE`-`SYNTHGM.SF2`-ROM-Einbindung und hörbare Original-PCM im VST3 noch nicht belegt. Keine Veröffentlichung, `0.0.0-dev`.


## 23.11.2026 — Kritischen `-silent`-Klangkiller aus Original-DOSBox-Start entfernt

- **Grundursache:** Gepinnter originaler DOSBox-X-Quellcode `src/hardware/sblaster.cpp` erzwingt bei `control->opt_silent` die Emulation `SBT_NONE`. Dadurch verhinderten sowohl VST3-Produktstart als auch beide Original-EMU8000-Prüfpfade aktiv die Sound-Blaster-/AWE32-Hardware.
- Originalhardware-Windows-Gate [45184081242](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45184081242) **FAIL**: keine `EMU8K_TRACE`-Datei. Diesen Fehlschlag nicht als Erfolgsnachweis werten.
- In `src/DosBoxRuntimeHost.hpp`, `tests/WindowsAweHardwareSmoke.py` und `tests/WindowsPrivateCreativeGuestSmoke.py` genau `-silent` entfernt. `-nogui -nomenu` verbleiben; originale Synthese, P165-GUI und Creative DOS unberührt.
- Datenfreie Windows-/Linux-Testfälle verhindern Rückfall; Hardware- und echter Original-Creative-Klang-PASS erst nach beendeten End-to-End-Läufen deklarieren. Noch kein Release, `0.0.0-dev`.


## 23.11.2026 — CI-Pfadregression an Windows-Pathlib angepasst

- Fehlgeschlagenen Windows-Harness-Test im Job 45183835257 ausgewertet: `Path('C:/PROBE/awe32.raw')` erzeugt auf Windows Backslash-Trennzeichen, der Test erwartete aber hart codierte Forward Slashes. Test benutzt jetzt `str(Path(...))` für das tatsächliche Gastargument, ohne Emulator-/Soundcode anzufassen.
- Neue Windows-Datenfrei- und echte Original-DOSBox-X-Hardware-CI prüfen; bis dahin kein Hardware-PASS.


## 23.11.2026 — AWEUTIL-Porttrace-Regression korrigiert

- Python-Regex `\\d+` und synthetische Trennzeilen auf den tatsächlichen Original-DOSBox-X-EMU8000-Trace abgestimmt; doppelte Escape-Ebene beim Quellenbau entfernt. Portwerte und echte originale EMU8000-Engine bleiben unverändert.
- Neue Linux-/Windows-Prüfungen müssen auf die beendeten CI-Jobs warten; noch kein Klang- oder Originaldaten-Nachweis.


## 23.11.2026 — Originale EMU8000-Hardware-Register im DOSBox-X-Windows-DLL-Gast prüfen

- Zusätzlich zu DOS-Shell und MPU-401 jetzt originalen AWE32-Hardwarepfad als `sblaster sbtype=sbawe` prüfen: Original-DOSBox-X-Builtin `AWEUTIL /S` soll echte EMU8000-Schreibfolgen auf 620h/A20h/E20h erzeugen, vom **originalen** `EMU8K_TRACE` aufgezeichnet. Keine eigene Chip-Engine, keine geänderten Originalregisterpfade.
- Nur absichtlich leeres 1-MiB-ROM im temporären Windows-Runner, niemals privates Owner-ROM/Creative/SF2, und keine Ausgabe privater Artefakte. Mindestens 16 EMU8000-Registerwrites an mehreren Ports, tatsächlicher DOS-Gast-Rücksprung; ansonsten FAIL. Timeout 75 Sekunden.
- Datenfreie Linux/Windows-Python-Regressionsprüfungen für Gastargumente und die originale Trace-Zeilenanalyse ergänzt. **Neu eingeführter Original-DOSBox-X-EMU8000-Windows-Test nicht als PASS ausgeben, bevor der integrierte Lauf beendet ist.** Kein Original-Creative-/VST3-Audioerfolg und kein Release.


## 23.11.2026 — Original-DOSBox-X-Windows-Integration und synthetische Privattest-Regressionen: CI bestätigt

- [Windows-Integration 45175109078](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45175109078) **SUCCESS**: unveränderter AWE32-Kern als in-process-SDL2-DLL, MPU-401 bis in DOS-COM-Programm, echtes Mixer-PCM, Stop/Join und VST3-Bundle-Build auf Windows 2022 bestanden.
- [Transport- und Privat-Klangtest-Harness 45177530826](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45177530826) **SUCCESS auf Linux und Windows**: synthetische Unit-Tests auch für Original-MIDI-Bytes und Hash-/EMU-Trace-Gates. **Keine** originale Creative-DOS-Software, ROMs oder SF2 in öffentlicher CI.
- `AKTUELLE-VERSION.md` von historischen, inzwischen überholten „Windows-Nachweis ausstehend“-Formulierungen bereinigt. Keine Änderung am Original-P165-GUI oder Synthesizer-Code, keine klangliche Freigabe. Original-Creative-DEMO im neuen VST3 und echter Ton bleiben weiterhin ausstehend.


## 23.11.2026 — Private Creative-Prüfung mit öffentlichen, datenfreien Regressionen verbunden

- Linux-/Windows-`audio-bridge-tests.yml` führt den privaten Creative-Klangtest **nicht** aus, sondern prüft mit Fake-Dateien SHA/Git-Blob-Verifikation, Original-DOSBox-Konfiguration, MPU-MIDI-Bytes und trace-write-Erkennung. Kein Original-ROM, Creative-Binary oder privates Audio in CI.
- PCM-Auswertung startet erst, nachdem eine echte NoteOn-Nachricht vom Original-MPU-401-Callback abgerufen wurde; zuvor auftretende DOSBox-Initialisierungsframes können damit nicht als NoteOn-Audio gelten.
- Klang- und DAW-Abnahme weiterhin offen, `0.0.0-dev`.


## 23.11.2026 — Binäre Testbytes und Trace-Parser im privaten Klang-Gate korrigiert

- Escape-Sequenzen für originale MIDI-Statusbytes, Git-Blob-Hashheader und echten EMU8000-Zeitstempelregulärausdruck in der Windows-Gateprüfung berichtigt; portable synthetische Regressionen verifizieren das Format. Keine Originaldateien in CI.
- Original-Creative-Klang-PASS weiterhin ausstehend; Plugin bleibt nicht freigegeben.


## 23.11.2026 — Private Original-Creative-AWE-Klang-Abnahme als ausführbare Windows-Gateprüfung

- `WindowsPrivateCreativeGuestSmoke.py` führt den originalen DOSBox-X-AWE32-Kern **aus dem bereits gebauten einzelnen VST3-Bundle** im selben Windows-Prozess wie den isolierten Testhost aus. Es benötigt nach Hash bestätigte Original-Dateien nur in `Contents/Resources/PRIVATE` und verwendet den identischen Gaststart mit Creative `DEMO.EXE /SBK:SYNTHGM.SF2`.
- **Realitäts-Gates:** Original-MPU-401-CC0/Program/NoteOn, ursprünglicher PCM-Stereo-Callback mit messbarem Note-On-Peak, echte Original-EMU8000-Portschreibtraces, ausschließlicher SDL2-Dummy-Ausgang und erfolgreicher Stop/Join. Keine generierten Audiosignale und keine freien ROM-Dummies im Klangtest.
- Absturz-/Timeout-Isolation via privatem Windows-Unterprozess. Öffentliche Linux/Windows-CI prüft ausschließlich Syntax und synthetische Konfiguration/Fake-Daten ohne originale Creative-Dateien. **Privater Klangtest aktuell noch nicht tatsächlich bestanden**, kein VST3-DAW-Test, keine Releasefreigabe.


## 23.11.2026 — Release-Sicherheitsprüfung des Bankwahltests (kein Audio-/Creative-Fix)

- Regression von `assert` auf einen in `Release` aktiven `CHECK`-Abbruch umgestellt. Vorher war die Laufzeitschleife durch `NDEBUG` abgeschaltet, obwohl die Compiletime-Randwertprüfungen funktionierten.
- Jetzt alle 16 MIDI-Kanäle und jeweils 128 Bank- und Programwerte prüfen (4096 Kombinationen), ohne proprietäre Originaldaten und ohne Änderungen am VST3-Audio-/EMU8000-Code.
- Linux-/Windows-Release-CI-Ergebnis erst nach tatsächlichem Workflowabschluss deklarieren. Entwicklungskennung unverändert `0.0.0-dev`.


## 23.11.2026 — Creative-DOS-Bankwahl auf den originalen CC0-Pfad korrigiert

- Original `DOS/DEMO/DEMO.C` unabhängig kontrolliert: Die Creative-SF2LIB-Demo setzt beim manuellen Bankwechsel `awe32Controller(15, 0, k)`. Die bisherige VST3-Bankübertragung setzte **CC0 fälschlich immer auf 0** und sendete den gewünschten Wert über CC32; der DOS-Treiber konnte deshalb bei Bankwechseln weiter Bank 0 verwenden.
- VST3 sendet jetzt den gewünschten Bankwert direkt in **CC0** und danach die Program-Change-Nachricht. Keine Änderung an der originalen DOSBox-X-EMU8000-Engine, Creative-DOS-Demo oder originalen P165-GUI.
- `src/CreativeMidiBankSelect.hpp` und unabhängiger C++20-Regressionstest hinzugefügt, Linux-/Windows-CMake-Testworkflow für Änderungen am Header aktiviert. Isolierter lokaler GCC20-Helper-Test PASS; echte CI-/VST3-Integration **bis zum GitHub-Actions-Ergebnis offen**.
- Kein neuer Branch, keine proprietären Binärdaten und keine Freigabe. `0.0.0-dev` bleibt unverändert.


## 23.11.2026 — Private Test-Bundle-Paketierung Linux/Windows geprüft

- **GitHub Actions [45110560473](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45110560473): SUCCESS** – originale Windows- und Linux-Transporttests einschließlich der neuen privaten VST3-Paketierungsregression mit ausschließlich erzeugten **Fake-Originaldateien** durchgelaufen. Keine echten Creative-ROMs, Original-`DEMO.EXE`, SoundFonts oder herunterladbaren Lizenzdateien im GitHub-Runner.
- Der private Assemblierer prüft Hashes und exakt drei Originaldateinamen (ROM, DOS-Guest, ROM-GM-`SYNTHGM.SF2`), bewahrt P165-Artwork und schreibt ausschließlich in gitignoriertes `PRIVATE/`. Keine Aussage über echten Creative-DOS-AWE-Sound im finalen Plugin.

## 23.11.2026 — Private Ein-Bundle-Paketierung mit Originaldatei-Integritätsprüfung vorbereitet

- `tools/prepare_private_vst3.py` sichert die geforderte **Ein-VST3-Installation**: originaler Windows-VST3-Host, originale DOSBox-X-AWE32-DLL, unveränderte P165-Frontplatte und die später lokal benötigten Creative-Dateien zusammen in einem einzigen privaten `.vst3`-Bundle.
- Verwendet ausschließlich lokal vorhandene, bereits legitim bezogene Ownerdateien und kopiert unter das im Git ignorierte `PRIVATE/`. Akzeptiert nur Original-ROM mit geprüftem SHA256/1MiB, Creative `DEMO.EXE` mit dokumentiertem Original-Git-Blob, `SYNTHGM.SF2` mit SHA256/42.726B und die byteidentische P165-Originalfrontplatte. SBK wird ausdrücklich **nicht** mit SF2 verwechselt.
- Vor dem Kopieren alle Eingaben geprüft; Verweigerung bei Falschdateien, Symlinks, unsicherem Zielort oder bestehendem Output. Keine Originaldateien in Git, GitHub-Runner, CI-Artefakte oder Logs.
- Neue portable Python-Unit-Tests mit **ausschließlich synthetischen Testbytes** in Linux/Windows-Testlauf eingebunden; vor echtem CI-Ergebnis kein PASS. Ein so gepacktes DEV-Bundle ist **kein** originalklanggeprüftes Plugin und kein Release.

## 23.11.2026 — Original-DOSBox-X-SDL2, 16-Bit-MIDI-Hardware, Stereo-PCM und vollständiges VST3-Bundle: Windows-PASS

- **GitHub Actions [45109673741](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45109673741) – gesamter integrierter Windows-Lauf SUCCESS.** Originaler gepinnter DOSBox-X-AWE32-Fork als **Windows x64 SDL2-DLL** gebaut, verlinkt und zur Laufzeit innerhalb des Windows-Testprozesses geladen. Originales DOSBox-`main()` führte echte DOS-Kommandos aus und kehrte mit Exitcode 0 zum Host zurück.
- **In-Process-Audioisolierung PASS:** wirklicher interner `SDL`-Audiotreiber `dummy` bestätigt, keine zweite Windows-Hardwareausgabe. Ein **echter DOSBox-X-Mixer-PCM-Callback** lieferte **1024 Stereo-PCM-Frames** an die Windows-Hostfunktion, kein generierter Fake-Klang. Das emulierte DOSBox-Workerprogramm wurde per `AWE64_RequestStop` beendet und der Host-Thread ordnungsgemäß gejoint.
- **Unabhängiger Original-MPU-401-Hardwarepfad PASS:** Ein selbständiges 16-Bit-`MPUPROBE.COM` schaltete den Original-MPU über Port 331h in UART, las exakt `90 3C 64` aus Port 330h (vom echten Hostcallback über den gepinnten Original-MPU401-Emulator eingespeist) und schrieb die erhaltenen Bytes in eine DOS-Datei `MIDI.BIN`. Der Windows-Prozesstest verglich die drei echten empfangenen Bytes erfolgreich.
- **Windows-VST3-Bundle erfolgreich:** P165-Original-GUI und Windows-x64-Steinberg-VST3-Host mit dem kompilierten originalen DOSBox-X-AWE32-Kern als **interne DLL innerhalb desselben `.vst3`-Bundles** gebaut. Es wird keine externe DOSBox-EXE benötigt.
- **KEINE** künstliche Klangabnahme: CI arbeitet ausschließlich mit proprietärdatenfreien DOS-Programmen; **Original-Creative-DEMO.EXE + echtes ROM + SYNTHGM.SF2 und hörbarer/vergleichbarer AWE-Sound im echten Windows-VST3 wurden nicht nachgewiesen.** Version bleibt `0.0.0-dev`, keine Release-Freigabe.
- Die älteren Windows-SDL1/SDL2/CRT-, CMake-Bracket- und MPU-401-Startmodus-Fehler sind bei diesem Windows-Erfolgsnachweis nicht mehr vorhanden.

## 23.11.2026 — SDL_STATIC_LIB-Originalquellpatch gegen Doppelanwendung absichern

- CI 45109467871, Linux-/Quellcheck **FAIL**: Die Funktion `patch_sdl2_static()` ließ sich ein zweites Mal auf dieselbe SDL.c anwenden, weil der ursprüngliche `#if`-Anchor trotz eingefügtem Define weiterhin vorhanden war.
- Eindeutigen Guard gegen vorhandenes `#define SDL_STATIC_LIB 1` ergänzt, der wiederholtes Patchen verweigert und damit die eigentliche Ursache der Fehlermeldung beseitigt. C++-Quellstand und Original-EMU8000 bleiben unangetastet.
- Erneuter originärer SDL2-Windows-Build weiterhin erforderlich. Auch dieser Bug war ein Source-Patcher-Unit-Test, keine Behauptung von AWE-Audio.

## 23.11.2026 — Doppelte SDL2-DLL-Einstiegsfunktion aus statischer Originallibrary entfernen

- Windows-`Release SDL2|x64` Run **45108821043** zeigt wichtige Verbesserung: Die zuvor ca. 280 fehlenden CRT-Symbole wurden durch explizite `libvcruntime/libucrt/libcmt`-Abhängigkeiten beseitigt. Verbleibende **eine** Link-Sperre: `sdl2.lib(SDL.obj)` definiert selbst `_DllMainCRTStartup`, obwohl SDL2 im Originalprojekt als **StaticLibrary** gelinkt wird und das übergeordnete DOSBox-VST3 bereits den echten CRT-DLL-Startpunkt besitzt.
- Das Original-`vs/sdl2/src/SDL.c` verwendet `!defined(SDL_STATIC_LIB)` als Schalter für genau diese zusätzliche DllMain-Definition. In der **temporären** DOSBox-Working-Copy nun `#define SDL_STATIC_LIB 1` direkt vor dieser Originalbedingung gesetzt. Ursprüngliche SDL2-Audio-Callbacks und Hardwareemulation bleiben unverändert; Original-Git-Submodul bleibt fixiert.
- Eindeutiger statischer Regressionstest verhindert das versehentliche Einführen eines zweiten CRT-DLL-Entrypoints. Vollständiger Windows-Build und Gast-/PCM-Tests zu diesem Commit noch offen.

## 23.11.2026 — Realen Original-DOSBox-X-PCM-Callback und CI-Build-Isolation prüfen

- Der Windows-Guest-Smoke registriert `AWE64_HostRegisterPCM` über C-ABI als echte Python-Hostfunktion. Er verlangt einen vom originalen DOSBox-X-Stereo-Mixer gelieferten PCM-Block, während der Original-DOSBox-Guest startet und kein zusätzliches Windows-Audiohardwaregerät öffnet. Keine künstliche Klangerzeugung; ohne Creative-ROM darf das PCM-Signal hier noch still sein.
- `windows-dosbox-runtime.yml`: `concurrency.cancel-in-progress` auf `main` verhindert, dass ältere DOSBox-X-/VST3-Kompilierungen weiterhin gleichzeitig kostspielig laufen, wenn eine neue geprüfte Änderung auf `main` eingecheckt wird.
- **Keine Behauptung eines Audio-Sound-PASS vor tatsächlichem CI-Ergebnis** und weiterhin keine Original-Creative-DOS-/ROM-Klangabnahme.

## 23.11.2026 — Original-Creative-MPU401-Initialisierung korrigiert (kritischer 0x3F-ACK)

- Originales gepinntes `src/hardware/mpu401.cpp` direkt überprüft: `MPU401_WriteCommand` ignoriert im **bereits aktiven UART-Modus** jeden Befehl außer 0xFF (Zeile 135). Die ursprüngliche Host-Konfiguration `midi mpu401=uart` verhinderte dadurch den ACK auf 0x3F und konnte `DEMO.EXE` mit `ERROR: MPU401 initialization failed` scheitern lassen.
- Die originale Creative DOS-Demo sendet nach ihrem Reset ausdrücklich `MPU_ENTER_UART = 0x3F` und wartet auf `MPU_ACK_OK=0xFE`. Korrekte Vorgabe **`midi mpu401=intelligent`**: emulierter Chip startet im Intelligent-Modus, Creative schaltet selbst zur echten UART-Betriebsart, originaler Hardware-ACK bleibt erhalten.
- Diesen Pfad sowohl beim **VST3-Guest-Start** als auch beim eigenständigen echten DOS-COM-MPU-Hardwaretest korrigiert. Keine Creative-Code-Änderung und keine native Treiber-Emulation. Neuer CI-Nachweis erforderlich.

## 23.11.2026 — SDL2-DLL-CRT-Linker und P165-CMake-Syntax korrigiert

- CI 45108405024 (SDL2-internes DOSBox-X als DLL) scheiterte mit fehlenden VCRUNTIME-/UCRT-Symbolen beim DLL-Link; die ursprüngliche SDL2-Konfiguration stammt von einem EXE-Projekt. Beim DLL-Build werden die benötigten **statischen** CRT-Bibliotheken `libvcruntime.lib`, `libucrt.lib`, `libcmt.lib` nun explizit in die SDL2-Release-Linkerabhängigkeiten aufgenommen. Originaler Chip-/MIDI-/Audio-Code bleibt unverändert. Neuer Windows-Nachweis ausstehend.
- CI 45108564142 scheiterte schon beim portablen CMake-Parser: falsch terminierte CMake-Bracket-Literals der temporären P165-Dialogersetzung sowie escaped Variablen; beide korrigiert. P165-GUI-Originaldatei und Grafikdaten bleiben byteidentisch.
- Keine Aussage über funktionierenden Creative-Gast oder Klang vor einem tatsächlichen vollständigen Windows-End-to-End-Test.

## 23.11.2026 — Falsche Win95-/86Box-Dialoge im alten P165-GUI-Build korrigiert

- Die alte P165-GUI verwendet beim ROM/SF2-Klick noch den Win95-/86Box-P007-Wortlaut, der zur festgelegten **DOSBox-X + Creative-DOS**-Architektur falsch ist.
- Ausschließlich der CMake-generierte temporäre GUI-Quellcode erhält korrigierte Dialogtexte. Original-GUI, Frontplattenbild und 73-%-Reglerhintergründe werden **byteidentisch erhalten**.
- ROM-Text nennt den privaten WAV-ROM-Pfad; SF2-Text behauptet **nicht**, dass der noch fehlende Live-SF2-Wechsel funktioniert. Kein Layout- oder Engine-Umbau.
- Erst der echte Windows-VST3-CMake-Lauf entscheidet über PASS/FAIL.

## 23.11.2026 — SDL2-Gerätediagnose nach tatsächlicher Gerätedefinition platzieren

- Erster echter `Release SDL2|x64`-Build (Actions 45108048294) scheiterte auf der Compilerfehlerstelle `mixer.cpp:86`: das Testexport `AWE64_IsInternalAudioOnly` stand **vor** der Deklaration `SDL2_AudioDevice`.
- Quellpatch verankert die Diagnosefunktion jetzt explizit **direkt nach der ursprünglichen SDL2_AudioDevice-Deklaration** im originalen `#ifdef C_SDL2`-Block. Keine DSP-/EMU8000-/SoundFont-Änderung; statischer Regressionstest für korrekte Code-Reihenfolge.
- Erneuter kompletter Windows-SDL2-DLL-/MIDI-/Audio- und VST3-Build muss erst tatsächlich bestehen.

## 23.11.2026 — Kritischen SDL1/SDL2-Buildfehler korrigiert (Recheck erforderlich)

- Fehlerdiagnose aus Windows-Actions 45107364211: Das bisherige originale `Release|x64`-Projekt baut **SDL1**, nicht SDL2. Dadurch wurden die ursprünglichen Hidden-Window-/SDL2-Dummy-Audio-Integrationen **nicht** wirksam, und die echte Audio-Isolierungsprüfung schlug fehl.
- Der Original-DOSBox-X-Fork besitzt eine separate `Release SDL2|x64`-Konfiguration. Diese wird jetzt für den internen Emulator zwingend gepatcht, mit `C_SDL2` kontrolliert und vom Windows-CI ausgeführt; die bisherige SDL1-Konfiguration bleibt im Original-Submodul unangetastet.
- Geänderter tatsächlicher DLL-Pfad: `bin/x64/Release SDL2/AWE64DOSBoxRuntime.dll`. Windows-Exports, der echte DOS-Guest, Host-MIDI-Probe, Mixer-Dummy und das VST3-Bundle werden **neu** geprüft, bevor ein PASS erklärt wird.

## 23.11.2026 — Verbindlichen Quell-/Prüfstand für Codex und spätere Chats konsolidiert

- `AKTUELLE-VERSION.md` komplett neu strukturiert: **gebaut/real getestet**, **nur implementiert** und **NO-GO bis Original-AWE-Sound** strikt getrennt. Exakte gepinnte Versionen, Referenz-CI-Läufe und private Datenfundorte stehen an einer Stelle.
- `START-HERE.md`, `README.md`, `AGENTS.md` auf denselben tatsächlich nachgewiesenen Stand gebracht; keine veraltete Behauptung mehr, die originale DOSBox-X-DLL sei noch überhaupt nicht als VST3-Bundle gebaut worden.
- Entwicklungskennung bleibt `0.0.0-dev`. **Ungeprüfte Audiofunktionen und der ausstehende Creative-DOS-Gaststart sind keine produktive Freigabe.**

## 23.11.2026 — Echter MPU-401-Hardwarepfad in 16-bit-DOS-Guest (unabhängiger Integrationstest)

- Eigenes winziges DOS-`MPUPROBE.COM` ohne Creative-Dateien (Assembler in `tests/MakeMpuCom.py`): spricht tatsächliche emulierte MPU-401-UART-Ports 331h/330h an, wartet auf ACK, liest drei MIDI-Bytes und schreibt die gelesenen Bytes mit originalen DOS-INT21-Funktionen in `MIDI.BIN`.
- `tests/WindowsMpuGuestSmoke.py` startet **die originale echte DOSBox-X-DLL im selben Windows-Prozess** und speist MIDI `90 3C 64` über `AWE64_HostRegisterMidi` ein. PASS nur, wenn der 16-bit-Gast dieselben drei Bytes wirklich in der DOS-Datei ablegt, exakt ein Callback konsumiert wurde und der Emulator kontrolliert zurückkehrt.
- Verifiziert bei Erfolg die Original-MPU-401-Transportstrecke bis zum DOS-Gast; keine proprietäre Creative-Datei und noch **kein echter EMU8000-/SF2-/AWE-PCM-Klangnachweis**. Teststatus zum Commit offen.

## 23.11.2026 — Ausschließlich DAW-Audioausgabe: interner SDL-Audiotreiber ohne Hardwareausgang

- `mixer.cpp` im gepinnten **internen** DOSBox-X-Windows-DLL-Build auf SDL2-`dummy`-Audiotreiber umgestellt. Die Original-Mixer-Callback-Synthese bleibt identisch und liefert PCM an den VST3-Ring; die Zusatz-Windows-Audiohardware bleibt geschlossen. Kein globales `SDL_AUDIODRIVER`-Environment im DAW-Prozess.
- Win32-Export `AWE64_IsInternalAudioOnly()` prüft, dass der originale interne SDL2-Treiber tatsächlich `dummy` ist **und** ein Audio-Callback-Gerät existiert. Windows-Lebensdauer-Test prüft dies ohne gesetztes `SDL_AUDIODRIVER`.
- Bei fehlgeschlagener SDL-`dummy`-Initialisierung kein Fallback auf eine physische Soundkarte; der Mixer bleibt im No-Sound-Modus. Windows-Compiler-/Laufzeittest zum Commit ausstehend.
- Weiterhin keine Original-Creative-DOS-Audio- und Studio-Pro-Abnahme.

## 23.11.2026 — Eigenes DOSBox-Fenster im DAW-Plugin unterdrücken

- Korrigierter tatsächlicher Original-SDL2-Codepfad: im Build der internen DOSBox-X-AWE32-DLL wird `SDL_WINDOW_SHOWN` durch `SDL_WINDOW_HIDDEN` ersetzt. Das einzige sichtbare Bedienfeld bleibt die erhaltene originale P165-VST3-GUI.
- Zusätzlich originale DOSBox-Argumente `-nogui` und `-nomenu`. Kein globaler SDL-Videotreiber-Environment-Hack in Fender Studio Pro.
- Source-Patch-Einzigkeitskontrolle und Tests aktualisiert. **Noch kein bestätigter Studio-Pro-Praxistest, keine Freigabe.**

## 23.11.2026 — Erste tatsächlich zusammengebaute interne DOSBox-X-VST3-Struktur nachgewiesen

- **GitHub Actions 45105475614 erfolgreich (Commit ccdbe3ac)**: originales gepinntes DOSBox-X-AWE32 als Windows-x64-DLL gebaut, alle Exporte geprüft und DLL in einem Windows-Prozess real geladen. Originales DOSBox-X-`main()` ausgeführt, DOS-`exit` erfolgreich mit Rückkehrcode 0 beendet.
- Im **selben** Windows-Lauf wurde die erhaltene originale P165-Frontplatte als VST3 kompiliert. Host-DLL `AWE64GoldDOSBoxDev.vst3` und originale `AWE64DOSBoxRuntime.dll` liegen gemeinsam innerhalb `AWE64GoldDOSBoxDev.vst3/Contents/x86_64-win/`. **Das ist eine VST3-Bundle-Struktur, keine externe DOSBox-EXE.**
- Der zusätzliche Original-Creative-DOS-Programmstart, Besitzer-ROM, private `SYNTHGM.SF2`, tatsächliche EMU8000-Register-/PCM-Ausgabe und Studio-Pro-Test sind weiterhin **nicht** geprüft.
- Windows-CI gestrafft: `windows-dosbox-runtime.yml` bleibt die einzige automatische integrierte Windows-Kompilierung bei Code-/GUI-Änderungen; die reine VST3-DEV-Shell kann bei Bedarf manuell gestartet werden. Verhindert doppelte Windows-Kompilierung in jedem Commit.

## 23.11.2026 — DOSBox-X-main() läuft und kehrt zurück; zusätzlich echten Worker-Shutdown prüfen

- Windows-Actions 45105475614: Original-DOSBox-X-Windows-DLL kompiliert; tatsächlicher In-Process-DOS-Guest-Boot und der DOS-Befehl `exit` **in einem Python-Windows-Prozess PASS**. Dies beweist **nicht** Creative-DOS-AWE-Audio.
- Smoke-Test meldete nach dem Funktionsrücksprung bisher missverständlich `DLL_UNLOADED`, obwohl `WinDLL` erst beim Verlassen des Funktionsscopes freigegeben wird. Meldung präzisiert auf `CALL_RETURNED`, keine unbelegte DLL-Unload-Behauptung.
- Zusätzlicher Windows-Prozess-Smoke-Test startet die echte DLL über einen Hintergrundthread (wie später die VST3), fordert über `AWE64_RequestStop` das Ende an und verlangt einen erfolgreichen `join()` nach 18 Sekunden, ohne den DAW-Hauptthread zu blockieren. **Noch kein PASS**, bevor CI das tatsächlich belegt.

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
