# Änderungsverlauf

## 26.11.2026 — Gezielte Nachweise erfolgreich; vollständige Originalregression wieder verpflichtend

- Exakter Produktquellstand `dfa777702ec79254974fa20a878a86c8ae65044a`: alle sieben Workflows SUCCESS. [Native DLL/Validator](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45893794898): sechs EMU8000-Fehlerprovokationen (Open, ROM, Read, Empty, RAM, Chip) PASS; insgesamt 26/26 ROM/Entry-Fälle einschließlich Positivkontrolle, 47/47 Steinberg-Validator, synthetische Dual-Instanz und P165-Editor/MOD RESET PASS. Keine allgemeine In-process-Absturzgarantie.
- [SF2 State](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45893794905): frischer Recall mit veränderter Sampleidentität abgelehnt, identische Datei akzeptiert, Zustand erhalten. [Adversarial](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45893794917): akzeptierte Bankidentität, Programm-/Notenreihenfolge, invalid block recovery, Handover ohne alten Stop unter Audiosperre und portable Negativkontrollen PASS. Keine sample-exakte Gastzeit oder lückenlose Übergabe behauptet.
- [Native Runtime](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45893794911): drei Kaltstarts, Pause, Automation, Bankwechsel und Rollback einschließlich asynchronem Bootfehler PASS, keine Prozessfehler. [96-kHz-Original-Deadline](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45893794916): 3/3 PASS, 0 Callback-Überschreitungen; p95 MIDI->PCM 17,2708 / 17,6458 / 16,2917 ms, maximale Callback-Walltime 0,2888 / 0,1602 / 0,0871 ms. ETW-Positivkontrollen nachgewiesen; keine Besitzer-PC-CPU-Messung.
- Die gezielte Hauptprüfung hatte die vollständige Original-Archivregression explizit NOT RUN. Temporären Push-Scope `[audit-targeted]` aus dem Hauptworkflow entfernt, so dass die vollständige Originalregression wieder zwingend läuft; keine Änderung an Plugin-, GUI-, DSP- oder Emulationsquellen und keine abgesenkten Gates. Getrennte gezielte Workflows bleiben erhalten. Vollständiger Nachweis dieses Produktquellstands, Besitzer-Assets, Studio Pro und übrige Auditgrenzen weiterhin offen; NO-GO bis zur Bewertung.

## 26.11.2026 — Codex-Nachaudit: SF2-Inhaltsidentität und alter Gast außerhalb Audiosperre (Kandidat)

- V6-Projektzustand speichert SHA-256 der tatsächlich akzeptierten SF2-Gastbytes und Pfad sowie alle bisherigen Parameter. V6-Recall verlangt identische Inhalte vor Zustandsänderung; fehlende oder veränderte Dateien werden abgelehnt. V1–V5 bleiben als Legacy lesbar, ohne rückwirkende Inhaltsidentität. Kein eingebettetes SoundFont-Abbild, keine automatische Ersatzbank. Assetfreier Validator-Fallback bleibt separat.
- Native Gegenprobe: gleicher Presetkatalog mit anderem Sampleinhalt, frische Instanz, alter State abgelehnt, Zustand unverändert, identische Originaldatei erneut akzeptiert. State-Oracles unterscheiden V2-Eingabe und V6-Ausgabe.
- Bankwechsel: bisherige Runtime unter Lifecycle-Sperre aus dem aktiven Besitzer verschoben, neue Runtime und Recall-Parameter publiziert, alter Gast erst nach Freigabe der Audiosperre gestoppt. Gegenprobe mit 600 ms verzögertem altem Gast-Stop, neuen NoteOns und Master-Automation. Kurze Publikationssperre, Primingphase und alte gehaltene Stimmen separat zu bewerten; kein Versprechen einer nahtlosen oder samplegenauen Übergabe.
- Alle fünf Codex-Befunde als Korrekturkandidaten zusammengeführt. Originaler EMU8000-Klangpfad und P165-GUI unverändert. Neue native Tests noch NOT RUN. Der vollständige unabhängige Bericht ist auf GitHub nicht vorhanden; Grundlage sind die vom Besitzer übermittelten fünf Befunde.
- Expliziter Push-Marker `[audit-targeted]`: gezielte native/portable Prüfungen einschließlich Validator, ohne vollständigen Original-Archiv-Sweep; normale Pushes und manueller Workflow behalten vollständige Regression. Kein Release, weiterhin NO-GO bis tatsächliche Nachweise und erneute Bewertung.

## 26.11.2026 — Codex-Nachaudit: EMU8000-Initialisierungsfehler (Kandidat)

- Gepinnter snd_emu8k.c 2935–2964: fehlender ROM-Stream und Short-Read rufen fatal() -> E_Exit -> exit auf; ROM/Empty/RAM-calloc ungeprüft. emu8000.cpp:268/277 weitere E_Exit-Pfade. AWE32ROM_Locate kann bei fehlender expliziter Datei auf Host-Suchpfade/Download-Prompt ausweichen. SBLASTER 4305 init im Original-VM-Power-on.
- Eingebettete C-Initialisierung prüft ROM-Open, exakt 1 MiB fread, alle drei calloc, räumt Teilressourcen vor IO-Handler-Registrierung ab. C++-Device prüft Chip-calloc und Init-Erfolg, veröffentlicht keinen ungültigen Chip, expliziter ROM-Pfad ohne Locator-Fallback; new(nothrow) behandelt Objektallokation. Fehler latcht bestehenden kooperativen Stop; nach abgeschlossener VM-Initialisierung wird der etablierte Run-Machine-Cleanup benutzt; Wrapper liefert -12567 erst nach Rückkehr aus main. Keine generelle SEH-/E_Exit-Umschreibung, keine Klangalgorithmusänderung, Standalone unverändert.
- Sechs gezielte neue native Provokationen (Open, ROM-calloc, tatsächlicher Short-Read, Empty-calloc, RAM-calloc, Chip-calloc). Kein EMU-Trace aus fehlgeschlagenem Start, Rückgabecode/normaler Cleanup/FreeLibrary, frische DLL im selben Prozess muss echte AWEUTIL-Schreibvorgänge nachweisen. Synthetische OOM-Injektion ist keine vollständige System-OOM-Absicherung. Arbiträre weitere Exceptions und Prozesscrashs bleiben offen. Tests NOT RUN.
- Zusammen mit Programm-Ordering und ETW-Gegenprobe auf demselben main-Stand. SF2-Recall und Bankwechsel im nachfolgenden gemeinsamen Kandidaten enthalten; native Nachweise offen. Keine Freigabe.

## 26.11.2026 — Unabhängiger Codex-Nachaudit: Programm-Automation und ETW-Gegenprobe (Kandidat)

- Unabhängige Meldung auf `0630e30`: NO-GO (EMU8000-Init-Fatal, fehlende SF2-Inhaltsidentität im Recall, blockierende alte Gastabschaltung beim Bankwechsel, Programm-/Notenoffset-Reihenfolge, ETW-Nullzeit). Vollständiger Codex-Bericht nur in dessen Checkout, auf GitHub 404; Auftrag und Befunde aus Besitzer-Nachricht übernommen, nicht als vollständiger Bericht ausgegeben.
- Originalregression `3a53c1d` [45883848769](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45883848769): 47/47, 128/128 Hüllkurven, Original PCM, Dual, 20/20 Fehlerfälle; Gesamt FAIL wegen einer 96-kHz-Callback-Deadline (2,2166 ms/1,33333 ms). Gezielter Nachlauf [45889779835](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45889779835) 3/3 PASS, 0 Queue-Verluste, max callback 0,1437/0,0937/0,2432 ms. Historischer Ausreißer nicht reproduziert. Keine Retro-PASS-Wertung.
- Legacy-Programm-Automation: gültige Programmpunkte werden mit realem Offset in bestehendem MidiBlockBatch als Setup einsortiert; vorheriger Controllerwert dient als Initial-Setup, keine Vorwegnahme des letzten Programms; nach tatsächlichem akzeptiertem Programmpaket lastProgram_ aktualisiert. Andere Blockregler unverändert. Native Gegenprobe tatsächliche FIFO-Bytefolge PNQRNNZ mit zwei Punkten im selben Block, Note vor späterem Programm und Setup bei gleichem Offset. Noch keine Sample-exakte Gastzeit im Legacy-Pfad behauptet.
- ETW-Diagnose: nur nachgewiesene positive Scheduled-Ticks des tatsächlichen Callback-Threads werden als Verhältnis ausgegeben; fehlende Spur, Nullzeit, ungültiges Fenster bleiben -1/NOT VERIFIED. Portable Tests mit fremder/fehlender Spur und gültigem Intervall. Bestehende strikte Walltime-Deadline unverändert.
- Korrekturen und neue Tests NOT RUN bis tatsächliche Logs. EMU8000-Init-Fatal, SF2-Recall und aktiver Bankwechsel bleiben separat offen. Kein GO.

## 26.11.2026 — Abschlussregression nach A09/A22-Sicherheitskorrekturen (ausstehend)

- `f513c0db`: [45879596716](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45879596716) SUCCESS, 20/20 gezielte native DLL-Fälle, config-isolation unverändert=1, SDL und Host-Policy PASS; Bridge, Adversarial und Version SUCCESS. Vollständige Originalregression damals NOT RUN.
- `831dd244`: [45881429683](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45881429683) SUCCESS, 24/24 Kaltstarts (12 historische, 12 aktuelle Produktrevision; beide Launcher jeweils 6/6); erste Note in jedem Versuch hörbar, process errors 0. Historische Ursache nicht reproduziert, NOT VERIFIED. Details in docs/AUDIT-COLD-START-INVESTIGATION.md.
- Besitzerauftrag: vollständige Original-/VST3-Regression der inzwischen gebündelten Sicherheitskorrekturen. Temporären Push-Shortcut [fatal-only] aus dem Hauptworkflow entfernt; damit laufen alle bisherigen Gates wieder verpflichtend. Separater manueller Fatal-Workflow bleibt für gezielte Prüfungen erhalten. Keine Änderung an Plugin-, GUI-, DSP- oder Emulationsquellen. Neue Regression NOT RUN bis tatsächliche Logs vorliegen; keine Releasefreigabe. Historische Kaltstartursache, vollständige In-process-Absturzabsicherung, Besitzer-Assets/Studio Pro und andere offene Auditrisiken bleiben gesondert zu bewerten.

## 26.11.2026 — A09/A22: Isolation der Host-Konfigurationsdateien (Kandidat)

- `7bf11818`: [Gezielter nativer Fatal-Audit 45877513159](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45877513159) SUCCESS 19/19, gemessene Host-Policy (16384,0,510) vor und nach Boot identisch, SDL-Fehlerreturn -12566 PASS. Adversarial [45877513162](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45877513162) SUCCESS, synthetischer Kaltstart peak 0,304871, process errors 0. Bridge und Version SUCCESS. Vollständige Originalregression NOT RUN.
- Konkreter weiterer A09/A22-Quellbefund: gepinnter sdlmain.cpp 8570–8839 liest/erzeugt vorab User- und CWD-Konfiguration, wertet daraus Arbeitsordner aus, kann chdir/Dialogs ausführen; 8840–9131 lädt/speichert/migriert erneut. -noconfig setzt nur opt_noconfig, verhindert diese beiden Phasen nicht. Setup.cpp ParseConfigFile enthält keine entsprechende Sperre. ENVIRON_LINKED kann zudem DOSBOX_-Variablen nach explizitem -set anwenden. Das ist ein belegter Störeinfluss, nicht bewiesene Ursache des historischen sporadischen Fehlers.
- Eingebetteter Pfad überspringt beide Standalone-Konfigurationsblöcke vollständig, setzt opt_defaultconf/opt_promptfolder und usecfgdir explizit, behält die originale -set-Schleife und kompilierten Defaults bei; ParseEnv deaktiviert. Standalone unverändert. Kein Eingriff in die originalen EMU8000-Algorithmen. Weitere OS-/Ressourcenpfade nicht pauschal als isoliert behauptet.
- Native config-isolation Gegenprobe: disposable dosbox-x.conf mit erzwungenem anderem CWD, nosound und autoexec exit. Original AWEUTIL muss trotzdem mit >=16 EMU8000-Writes durchlaufen, CWD und Konfigurationsbytes unverändert, kein Config-loaded-Marker. Neue Tests NOT RUN. Gezielter Fatal-Test, danach vollständige Original-/VST3-Regression notwendig. Allgemeine In-process-Abstürze und historische Kaltstartursache weiterhin offen.

## 26.11.2026 — A22: Hostprozess- und Desktop-Policy (Kandidat)

- `84940f6b`: [Gezielter nativer Fatal-Audit 45875728663](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45875728663) SUCCESS: 18/18 Fälle, SDL-Injektion rc=-12566, Cleanup-Return, DLL freigegeben und frischer Original-AWEUTIL-Start im selben Prozess PASS. Fehlerliste leer. Bridge, Adversarial und Version SUCCESS. Vollständige Original-/VST3-Regression beim [fatal-only]-Lauf bewusst NOT RUN; letzte vollständige Evidenz `f0646faa`.
- Gepinnter sdlmain.cpp: SetPriority (3540ff, Aufruf bei Startup 3787) setzt Prozesspriorität der DAW, Windows_DPI_Awareness_Init (7854ff) setzt DPI des gesamten Prozesses, sticky_keys (2808ff) schreibt Accessibility-Einstellungen, DOSBox_ShowConsole (7326ff) kann Standardstreams umleiten; ImmDisableIME(-1) (9309) betrifft den ganzen Prozess. Diese Standalone-Eingriffe sind im eingebetteten, unsichtbaren Runtime-Pfad nicht erforderlich und jetzt unter AWE64_VST3_RUNTIME deaktiviert. Standalone-Verhalten bleibt erhalten. Keine Umstellung der emulierten Takt-/MIDI-/DSP-Logik.
- Native host-policy-Gegenprobe: eigener Windows-Child mit expliziter BELOW_NORMAL-Priorität, echte GetPriorityClass/GetProcessDpiAwareness/SPI_GETSTICKYKEYS vor/nach vollständigem Original-AWEUTIL-Boot, unveränderte Werte und >=16 Registerwrites erforderlich. DPI/Sticky-Vergleich erkennt nur beobachtete Endzustandsänderungen, nicht alle transienten Eingriffe; Console/IME bislang statische Gates. Neue Tests noch NOT RUN. Kein genereller E_Exit-/SEH-/Crashschutz; sporadische Kaltstartursache weiterhin ungeklärt. Gezielter Fatal-Lauf, vollständige Abschlussregression danach erforderlich.

## 26.11.2026 — A22: gezielte SDL-Fehlerdiagnose (Kandidat)

- `5a62cdb9`: [Original-Lauf 45873551856](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45873551856) FAIL im neuen sdl-init-Fall. DLL-Build, upstream patch und 17 weitere ROM/Argument-Fehlerfälle PASS; spätere vollständige Original-/VST3-Regression NOT RUN. Bridge, portable/native adversarial und Version SUCCESS. Fehlende Child-Details lassen die genaue Ursache des sdl-init-FAIL nicht bestimmen.
- Upstream `sdlmain.cpp` DOSBOX_parse_argv (7536ff) setzt für -test opt_console=true und opt_nolog=true; DOSBox_ShowConsole leitet stdin/stdout/stderr um. Diese Diagnose war ungeeignet für eingebettete Fehlerprovokation. Jetzt explizites -awe64-audit-sdl-init nur im eingebetteten Entry-Wrapper verarbeitet und vor main entfernt; Scope-gebundene thread-local Injektion, kein -test und keine Prozessumgebung. Echte SDL-Initialisierung und gleicher Fehler-/Cleanup-Return. Zusätzlich Flush/Phasenmarker, Child-Exitcode (hex), Faulthandler und Fehlerlog. Keine Behauptung, dass dies die einzige Ursache war.
- Auf Besitzerwunsch gezielter Workflow-Scope: explizites [fatal-only] im Push-Commit aktiviert nur Original-DLL-Build, bestehende frühe native Gates und Fatal-Audit; spätere vollständige VST3-/Original-/Performance-/128er-Regression bewusst NOT RUN (sichtbarer Workflow-Name und Summary). Normale Pushes und workflow_dispatch bleiben unverändert vollständige Regression. Neuer Nachtest ausstehend; keine Freigabe und kein allgemeiner Absturzschutz-Claim.

## 26.11.2026 — A22: kontrollierter SDL-Initialisierungsfehler (Kandidat)

- `f0646faa`: sieben Workflows SUCCESS, [Boot-Timeout 45867304992](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45867304992) tatsächlich 30017 ms, timeout/closed/late_pcm/silent/no_midi=1, UART=1, initial PCM=0. Vier weitere Kaltstarts PASS. [Original 45867304458](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45867304458) 47/47, Original-Dual, 128/128 Cheezoids, p95 19,4558/18,9167/17,2604 ms, keine gemeldeten Drops/Underruns in Original-Performancefenstern. Frühere sporadische Ursache offen.
- A22: gepinnter Upstream sdlmain.cpp bei SDL_Init-Fehler E_Exit (support.cpp: MessageBox, SDL_Quit, exit(1)). An dieser einzelnen frühen Grenze, vor SDL_Prepare, GUI_StartUp, VM_PowerOn und Gerätelauf, kontrollierter Fehlerreturn -12566 mit Log-/optionalem Duktape-/SDL-Cleanup; Config wird durch normalen Scope-Return zerstört, control nach Return geleert, erfolgreicher COM-Init des Workerthreads per RAII ausgeglichen. Standalone-Branch unverändert. Kein globaler E_Exit-Fang oder SEH-Recovery.
- Native Gegenprobe erweitert: explizites -test + Diagnosevariable erzwingt die exakte Fehlerbehandlung nach erfolgreicher SDL_Init, kein Hardwaretrace, Rückgabecode -12566, Host überlebt; DLL freigeben, frische isolierte DLL im gleichen Host laden und Original-AWEUTIL-Positivkontrolle. Diese künstliche Fehlerprovokation beweist nicht sämtliche natürlichen SDL-Partial-Init-Fehler. Produkt-VST3 verwendet nie -test. Neue Tests noch NOT RUN; Originalkern-Neubau und Klangregression erforderlich. Weitere E_Exit-Pfade und beliebige in-process Abstürze offen.

## 26.11.2026 — A09/A22: Startüberwachung und Ursachen-Evidenz (Kandidat)

- `8c4612d7`: [A10 Snapshot 45860597792](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45860597792) PASS isolated=1 exact=1 cleaned=1; [Original 45860597790](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45860597790) SUCCESS 47/47, 128/128 Cheezoids, p95 19,161/18,9583/18 ms. Vier weitere Kaltstarts PASS. Bridge 45860597878 FAIL in Linux/Windows nur wegen statischem veraltetem Default-SF2-Pfad-Assert; auf neues D:-Staging plus tatsächlichen Schreibaufruf angepasst. Neuer Nachtest ausstehend.
- A09: Bei dauerhaft ausbleibender UART/PCM-Initialisierung meldete der Legacy-Echtzeitpfad unbegrenzt Erfolg mit Stille. Jetzt Startdeadline ab erstem nichtleerem Ready-Poll, 30 Sekunden wie existierender Candidate-/Offline-Boot; Ready+PCM zuerst prüfen, kein Warten im Echtzeitpfad, Pause/Reprime setzt pending Deadline zurück. Nach beobachtetem Timeout latched fault bis zur Deaktivierung, Ablehnung von MIDI/PCM, GUI-Status für Starttimeout. Keine automatische Wiederholung, keine Rückdatierung der MIDI-Zeit. Noch kein Nachweis der Ursache des früheren sporadischen Fehlers. Bei Shutdown neue Diagnose UART jemals beobachtet, maximale PCM-Reserve vor erstem Prime, Priming, Timeout, tatsächliche NoteOn-Pulls; kein RT-Logging.
- Deterministische ABI-Probe: UART ready aber dauerhaft kein PCM, reguläre 30-s-Deadline (kein Test-Override), verspätete PCM-Lieferung nach Timeout darf den Host nicht wieder freischalten. Bestehende Startup-Positivprobe bleibt. Ausführung NOT RUN. Kein Schutz gegen beliebigen in-process Fatalabbruch (E_Exit/SEH); keine Änderung Originalkern oder Grafik. Kein FULL AUDIT.

## 26.11.2026 — A10: unveränderte Standard-Soundbank pro Aktivierung (Kandidat)

- Exakter `af912386`: alle sieben Workflows SUCCESS. [Deterministischer A22-Test 45854291438](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45854291438): stale_pcm=1 not_ready=1 not_wait=1 not_midi=1 silent=1 exit=97. [Original 45854291377](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45854291377): 47/47 Validator, Original-Dual, 128/128 Cheezoids, p95 19,9546/18,8958/17,2708 ms, keine gemeldeten Drops/Underruns in Original-Performancefenstern. Vier erneute synthetische Kaltstarts PASS; einzelne synthetische Start-/Runtime-Unterläufe weiterhin gemessen. Frühere Kaltstartursache offen.
- A10: Default-SF2 zuvor aus installierter C:-Quelle durch den DOS-Gast geöffnet, während der Browser diese separat las. Jetzt vor Aktivierung begrenzter Snapshot (max. 32 MiB wie externer SF2), dieselben Originalbytes für Katalog und D:-Staging, keinerlei Versionsanpassung der Defaultbank. Bei Default-Wechsel mit vorbereitetem Katalog muss sichtbares Presetinventar übereinstimmen. Quelle unverändert, Staging wird beim Stop entfernt. Kein vollständiger Default-SF2-Struktur-/Originalhash-/Recall-Nachweis; bestehender synthetischer Fallback bleibt. 
- Native ABI-Gegenprobe: Defaultbank X, tatsächliches Gastargument /SBK:D:\\SYNTHGM.SF2, Quellfixture nach begin überschreiben, erst danach liest Worker die unveränderte Stagingdatei; Staging-Cleanup und Quelländerung prüfen. Windows-Ausführung NOT RUN. Originalkern/GUI nicht geändert; Originalklangregression erforderlich. A22 Prozessfatalpfade offen.

## 26.11.2026 — A22: veraltete Bereitschaft nach tatsächlichem Worker-Ende (Kandidat)

- [Adversarial 45826817908](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45826817908) SUCCESS: Kaltstart PASS (0,304871 / 0,213013 Peak), synthetischer Offline 289959 nicht-stumme Frames / 0 Prozessfehler, Bankidentität PASS, MIDI vNNV, 3/3 Invalid-Block und Wiederanlauf PASS; leere Fehlerliste. Portabler Oracle-Job, Transport und Version SUCCESS. Der frühere sporadische Kaltstart-FAIL bleibt ursächlich ungeklärt.
- A22: RuntimeHost durfte nach tatsächlichem Worker-Ende noch eine alte UART-Ready-Markierung bzw. gepuffertes PCM als erfolgreich melden und MIDI in die verwaiste Queue schreiben. Guard auf finished vor Bereitschaft, Audio-Warteprüfung, Demand-/MIDI-Übernahme; nach bereits beobachtetem Ende readAudio=0 und stumme Puffer. Legacy-Callback prüft zusätzlich nach Startwartepfad und am Blockende; Timed-Pfad prüft nach Audioübernahme. Keine Garantie atomarer Synchronisation mit einem zeitgleich eintretenden Worker-Ende, keine allgemeine Prozess-/SEH-/E_Exit-Isolation. Kein behaupteter Fix des sporadischen Kaltstarts.
- Neue deterministische native ABI-Gegenprobe: Testworker setzt Ready, erzeugt 1024 nicht-stumme PCM-Frames, bleibt bis zum expliziten Signal aktiv, beendet sich dann mit Code 97 ohne Ready/PCM zu löschen. Live-Positivkontrolle, danach stale reserve vorhanden, beide Ready-Abfragen, beide Wartearten, MIDI und PCM müssen fehlschlagen bzw. stumm sein. Windows-Ausführung NOT RUN; Originalkern und GUI unverändert. Kein FULL AUDIT.

## 26.11.2026 — A09: Kaltstart-Gegenbefund und unabhängige Testausführung

- Exakter Produktstand `ebbc1489`: [Original-Windows 45817583838](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45817583838) SUCCESS, 47/47 Validator, Original-Creative-Dual und 128/128 Cheezoids, p95 19,7506 / 17,6042 / 18,2708 ms, keine gemeldeten Drops/Underruns in den Original-Performancefenstern. SF2-State, Transport, Version ebenfalls SUCCESS. [Runtime 45817583796](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45817583796): drei von drei Kaltstarts mit erster Note PASS, gemessene Startfenster-Unterläufe 2560/2560/3072 Frames; kein genereller Null-Unterlauf-Claim.
- [Adversarial 45817583829](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45817583829) FAIL: identische Revision, synthetischer Kaltstart early/late PCM-Peak 0/0 bei process_errors=0. Ursache noch unbestimmt; weder als bloßer Testfehler noch als behoben bewerten. Bisher brach die Workflow-Stufe bei diesem ersten Fehler ab; offline, bankreplace, eventorder und neuer invalidblock wurden nicht ausgeführt, daher NOT RUN auf dieser Revision.
- Jetzt einzelne native Prozesse mit begrenzten Timeouts und separaten Resultat-/Markerprüfungen. Auch bei einem Kaltstart-FAIL werden alle unabhängigen Fälle ausgeführt; FAIL bleibt bestehen, kein Retry-to-green. Bei zwei stummen Startnoten zusätzliche begrenzte 30-s-Diagnose und späte Note, ursprünglicher Early-Deadline-FAIL bleibt unverändert. Normaler Cleanup und vorhandene Queue-Statistik auch im fehlgeschlagenen Kaltstart. Keine Änderung an Produktcode/GUI/Klang. Neuer Lauf NOT RUN, A09 und A22 offen.

## 26.11.2026 — A09: vollständige Eingabeprüfung vor Legacy-Blockübernahme (Kandidat)

- A12 Windows [45812591813](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45812591813) SUCCESS: vier native echte HWND-Ablehnungsfälle (begin/perform × zwei Editor-Instanzen), unveränderter State, Wiederanlauf, 47/47 Validator, Creative-Dual und 128/128 Cheezoids; p95 17,619/18,6042/17,6354 ms, keine gemeldeten Drops/Underruns. Transport und Version SUCCESS. Keine vollständige GUI-Abnahme.
- A09: im Legacy-process wurden bisher Master/Voice/Programmwerte veröffentlicht, bevor alle Punkte und MIDI-Ereignisse validiert waren; fehlerhafte reguläre Queue-Offets/NaN wurden teilweise ignoriert, gültige MIDI-Präfixe konnten bereits gesendet werden. Jetzt vollständige feste, begrenzte Blockvorbereitung (4096 Gesamteingaben), jede Queue einmal lesen, alle Punkte auf finite Werte, Offsetgrenzen und aufsteigende Reihenfolge prüfen; MIDI und Blockwerte erst nach erfolgreicher Strukturprüfung übernehmen. Fehlerhafte Blöcke liefern Stille + kResultFalse und vorhandene MIDI-Recovery. Bestehendes Clamping, Bank-0-/unbekannte reguläre-ID-Verhalten und blockweise Nicht-Master-Automation bleiben. Kein Claim samplegenauer Legacy-Gastzeit oder atomarer Rollback bei späterem Queue-Overflow.
- Native Gegenprobe mit echter VST3 und synthetischem ABI-Recorder: gültiger Master/Voice/CC-Präfix plus falscher Voice-Offset, NaN oder falscher Note-Offset; State bytegleich, Ausgänge stumm, kein NoteOn/CC7 des verworfenen Blocks, danach positive Wiederholung. Windows-Ausführung NOT RUN. Originalkern, Creative und GUI unverändert. Allgemeiner in-process Fatalpfad, weitere QA und Studio Pro offen.

## 26.11.2026 — A12: GUI-Host-Ablehnung und Rollback (Kandidat)

- A19 [45811475200](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45811475200) Linux/Windows SUCCESS: tatsächliche neue Git-Fixture-Regression und Windows-PowerShell-Syntax bestanden; Version 45811475148 SUCCESS. Privater Besitzer-Build nicht ausgeführt.
- A12: SDK v3.7.12 EditController beginEdit/performEdit/endEdit leiten Host-Rückgabewerte weiter; SingleComponent-Parameteränderung erfolgt lokal. Bisher wurden begin/perform-Ablehnungen ignoriert. Generierter P165-Editor prüft beginEdit, stellt abgelehnte performEdit-Werte lokal zurück, sendet kein end ohne akzeptiertes begin, synchronisiert abgelehnte sichtbare Regler und Browserauswahl. Ein abgelehntes end nach akzeptiertem perform kann ein bereits akzeptiertes Hostereignis nicht zurücknehmen. Kein neues Artwork und keine Änderung der freigegebenen ursprünglichen PresetBrowserView.cpp.
- Echte HWND-MOD-RESET-Regression erweitert: auf jeder Editor-Instanz positive Klicks, Host-Ablehnung begin/perform mit unveränderten 12 Parametern und byteidentischem Component-State, korrekte Callbackzahlen und anschließender positiver Wiederanlauf. Native Ausführung noch NOT RUN; keine vollständige Regler-/Browser-/DPI-/Automation-Abnahme. Allgemeiner in-process Fatalpfad weiterhin offen. Kein FULL AUDIT.

## 26.11.2026 — A19: privater Build ohne alte gepatchte Arbeitskopie (Kandidat)

- Vorheriger A22-Kandidat `4ed394c2`: [Original-Windows 45805621014](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45805621014) SUCCESS; alle 17 ROM-/Argument-Fälle PASS einschließlich neun neuer Ablehnungen mit anschließendem AWEUTIL-Wiederanlauf; Validator 47/47, Original-Creative-Dual, RAM-Stereo/Release, exakte Cheezoids-Revision 128/128 PASS, p95 18,9116 / 18,25 / 18,9688 ms bei 44,1/48/96 kHz, keine gemeldeten MIDI-/Audio-Drops oder Underruns. [Native Gegenproben 45805620986](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45805620986), Transport und Version SUCCESS. Allgemeiner in-process Fatalpfad bleibt offen.
- A19: früher wurde die gepatchte DOSBox-Arbeitskopie bei identischem Upstream-HEAD und bloßem PCM-Marker weitergenutzt. Jetzt pro privatem Build ein einzigartiger lokaler Checkout aus gepinnten Git-Objekten, aktueller Patch muss wirklich angewendet werden, kein Wiederverwenden alter Build-Verzeichnisse. Original-Checkout und alte private Arbeitskopien bleiben unangetastet. SHA256 des Patchers, resultierender Git-Diff, kopierte DLL und VST3-Binärdatei werden im privaten Run dokumentiert; kopierte DLL muss frisch gebautem Original entsprechen, beide Binärhashes vor/nach privater Prüfung identisch. Produkt-HEAD und Worktree-Status dokumentiert; keine vollständige Binärreproduzierbarkeit behauptet.
- Neue echte Git-Fixture-Regression mit zwei verschiedenen Patch-Revisionen, alter manipuliertem Checkout, unverändertem Besitzer-Edit, falschem SHA und wirkungslosem/fehlschlagendem Patch. In bestehende Linux-/Windows-Unit-CI integriert. Kein erneuter teurer Originalkern-Build für diese Skriptkorrektur. Ausführung dieser neuen Tests und privater Windows-Owner-Build noch NOT RUN. Paketpfad künftig `PRIVATE/working-upstream-<unique>/build-vst3/...`, wird ausgegeben. Alte Arbeitskopien nicht automatisch löschen. Keine Änderung am Plugin, Klang, GUI oder Treiber; keine Freigabe.

## 26.11.2026 — A22: abgesicherter Standalone-Kommandozeilen-Eintritt (Kandidat)

- Pinned DOSBox-X `f3a95b9`: `src/gui/sdlmain.cpp` 7119–7269/8406–8412 enthält eigenständige Konfigurations-/Editor-/Reset-Pfade mit direktem `exit()`; `src/misc/support.cpp` 489–514 enthält den allgemeinen E_Exit-Pfad mit Dialog, SDL_Quit und exit(1). `src/dosbox.cpp` 1288 und Mixer-Initialisierung sind weitere Fehlerquellen. Der bekannte ROM-Preflight bleibt unverändert.
- Der interne DLL-Eintritt akzeptiert nur die vom Produkt und den direkten DLL-Diagnosen verwendeten Schalter und Settings; `-noconfig` verpflichtend, Options-/Argumentstruktur vor originalem main() prüfen. Unbekannte oder Standalone-Argumente return -12565, kein DOSBox-Start. ROM-Ablehnung bleibt -12564. DOS-`-c`-Nutzlast bleibt für die vorhandenen internen Gäste unverändert. Dies ist kein Gast-Sandbox- oder allgemeiner Sicherheitsboundary.
- Lokaler isolierter C++17-Test des Argument-Prädikats mit g++ -Wall -Wextra -Werror PASS (erlaubte Produkt-/SB16-Argumente und abgewiesene Standalone-/Konfigurations-/Syntaxfälle). Vollständiger nativer MSVC/DOSBox-Build nicht lokal ausgeführt. Vorhandene echte native DLL-Prüfung um neun negative Fälle erweitert; jeder muss denselben DLL-Handle anschließend mit gültigem ROM und echten AWEUTIL-Portwrites nutzen können. Keine Lockerung des vorhandenen ROM-Gates. Native Ergebnisse ausstehend.
- Allgemeiner E_Exit in Initialisierung, Run-Loop, anderen Threads, abort/SEH, Speicherbeschädigung und vollständiger DAW-Prozessschutz bleiben **OPEN/NO-GO**. Kein pauschales Exception-Swallowing, keine Prozessisolation behauptet; ein separater Prozess würde die explizite Nutzerarchitektur ändern. Original-EMU8000, Creative-Treiber und P165-GUI unverändert. Kein FULL AUDIT.
 
## 26.11.2026 — Auditkorrekturen, Paket 1 (noch nicht nachgetestet)

- Referenz und nachgewiesene Defekte: docs/ADVERSARIAL-AUDIT-0ff0811.md. Keine neue Engine, kein Creative-/EMU8000-Quellpatch, kein GUI-Redesign. Stand bleibt 0.0.0-dev, keine Freigabe.
- A06-I: akzeptierte SF2-Bytes bei Reaktivierung wiederverwenden; kein unbemerktes erneutes Lesen und Programmwechsel. Explizites Neuladen/setState darf neue Daten akzeptieren. Serialisiertes Format weiterhin pfadbasiert; neue Hostinstanz nach Dateiaustausch noch separat zu prüfen.
- A01-O: feste MidiBlockBatch auch im Legacy-Pfad, chronologische Reihenfolge nach Host-Offset, bei Gleichstand Controller vor Note. Kein Claim samplegenauer Legacy-Gastzeit; blockweise übrige Parameter und PDC bleiben offen.
- A07-O: nur kOffline darf begrenzt auf echte Gastbereitschaft und PCM warten; bei fehlenden Frames expliziter Fehler statt erfolgreicher Stille. Legacy-Offline bleibt an die Gast-Wallclock gebunden, nicht schneller/deterministisch samplegenau; Demand-Clock-Forschungsweg unverändert. Realtime ohne neue Warteoperationen.
- A13: vollständiges 10-Byte-Overlay und absolute Sends als vorhandene GuestCommand-Pakete auf dem Gast-CPU-Thread zwischen MIDI-Paketen übernehmen; kein separater Host-Thread-Write der beiden Hälften während Wiedergabe. Native Register-/PCM-Regression noch erforderlich.
- A26: Stereo-/Linked-Sample-Referenz muss reziprok, typkompatibel und nicht selbstreferenziell sein. A15/16/20/23: tatsächliche Zahlen/Rate/Deadline/CPU-Sustain-Evidenz erforderlich, kompletter 128er Vergleich mit nichtstummem Referenz- und Ist-Signal, exakte MIDI-Zeitziele und stereo 441-Hz-AC-Oracle, exakt 47 Validator-PASS. Gitlink-Trigger ergänzt. Ursprüngliche sieben Negativkontrollen bleiben aktiv, native drei Gegenproben erneut ausgelöst. Neue Ergebnisse NOT RUN.
- A24/A25: Original-RAM-Spektraltest fordert dominante Sollfrequenz in beiden Kanälen und abgeklungene letzte Release-Hälfte (RAM-Fixture -2400 Timecents); Original-Dual-Host prüft beide Richtungen mit je frischem Prozess, gleicher Initialisierungsreihenfolge, vertauschter erster Note und Teardown. Keine bloße Pegel- oder doppelte PASS-Ausgabe; native Ausführung noch offen. 
- Weiter offen: allgemeiner in-process E_Exit/abort/SEH-Schutz A22, GUI-Host-Rejections A12, native Nachweise zu den neuen A24/A25-Orakeln, Private-Build-Provenienz A19, restliche vollständige MIDI-/Host-/Besitzer-Matrix und unabhängiger Nachaudit. Keine FULL-AUDIT-Freigabe.

## 26.11.2026 — Stabilisierung 7: frühe MIDI-Pakete erst bei primed PCM zustellen

- `51545219`: [Original-Gesamtlauf 45727299084](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45727299084) PASS, Validator 47/47, Cheezoids alle 128 Vergleich PASS, p95 19,161 / 18,9375 / 17,3021 ms (44,1/48/96 kHz), 0 Drops/Underruns; 2822 / 3440 / 3175 Kandidatenframes bewusst vor Handover unterdrückt. [Native ABI 45727299133](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45727299133) Kandidaten-Queue 0/4 PASS. Unit-, SF2- und Versionstests PASS.
- [Native Runtime-Faults 45727299032](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45727299032) fünf Modi PASS, früher NoteOn FAIL (Peak 0, späterer NoteOn 0,304901; erstes process() erfolgreich, keine MIDI-Drops, 1536 Audio-Underrun-Frames). Exakter Verlustort nicht nachgewiesen. Aktuelle Host-Logik erlaubt UART-Entnahme bereits während PCM-Priming; daraus folgt eine reale Startup-Race-Möglichkeit. Keine Behauptung, dass der beobachtete Ausfall eindeutig dadurch verursacht wurde.
- A09: unabhängiges Legacy-MIDI-Startgate im Adapter, geschlossen bis zum ersten nichtleeren Host-Block mit UART-Ready und bestehender PCM-Priming-Reserve. Frühe Note/Bank/Program/CC bleiben in der vorhandenen FIFO, keine Wiederholung oder Neuerzeugung. Synthetischer DOS-Gast erhält ebenfalls echten PCM-Vorlauf; ABI-only ready(0) bleibt als UART-Abfrage verfügbar, öffnet aber nicht die MIDI-Zustellung. Clocked/Demand-Pfad unverändert; keine Änderung an normalem PCM-Zielpuffer, originalem Chip, Treiber oder GUI. Separater NoteOn-Pull-Zähler zur Eingrenzung (Adapter-Entnahme ist keine unabhängige Guest-Read-/PCM-Bestätigung).
- Gegenprobe: portable FIFO-Reihenfolge, null vor Gate, exakt ein NoteOn nach Gate; native echte RuntimeHost-ABI mit absichtlich verzögerter PCM-Erzeugung und wiederholten frühen UART-Polls. Drei unabhängige kalte native VST3-Startproben (alle müssen bestehen, keine Retry-zu-PASS-Regel). Der vorhandene echte Originalklang-/Cheezoids-/20-ms-Gate bleibt unverändert. Neue Nachweise offen, FULL AUDIT und Freigabe weiterhin offen.

## 26.11.2026 — Stabilisierung 6: Kandidaten-PCM erst beim Host-Handover

- `9acaf1af`: [native Runtime-Faults 45724029235](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45724029235) alle sechs Modi PASS, insbesondere asynchroner Bankstartfehler (26 Callback-Blöcke während Fehler, Peak 0,247864, keine Prozessfehler, alter State/PCM erhalten) und Bus-Indizes. Unit-/ABI-/Bank-/Versionstests PASS. [Original-Gesamtlauf 45724029192](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45724029192) FAIL: p95 75,034 / 87,2708 / 85,2812 ms bei 44,1/48/96 kHz, mittlere native Queue-Tiefe 3123,82 / 3663,51 / 3546,97 Frames, jeweils 0 Drops/Underruns, keine Callback-Deadline-Überschreitungen. Validator 47/47 und ROM-Eintritt 8/8 bleiben PASS. Cheezoids-VST3-Sweep Exit 1 (konkrete Hostdiagnose bisher vom Python-Wrapper verschluckt); nicht als gelöst behauptet.
- Ursache der Latenzregression: das in Stabilisierung 5 vorgebootete Ersatz-Guest liefert nach UART-Ready bereits PCM, bevor der Host erstmals dessen Queue liest. Keine Änderung an Mixer, ROM, Creative-DSP, Samplerate oder Klangparametern. Separates pro-Adapter Candidate-Capture-Gate unterdrückt diese PCM-Frames bis zum ersten nichtleeren Host-Callback nach Veröffentlichung; alter Guest bleibt unabhängig, Host-Pause-Gate und experimenteller Demand-Clock behalten ihre Semantik. Keine beliebige laufende Musik verwerfen, keine Verringerung der normalen Priming-Reserve und keine Lockerung des 20-ms-Ziels. Pre-Handover-Frames separat messbar.
- Portable Gegenprobe 40.000 Kandidatenframes vor Handover, alte Engine liefert weiter, Host-Pause bleibt aktiv, anschließend frische Stereo-PCM; Demand-Clock Positivkontrolle. Native ABI-Gegenprobe injiziert denselben Burst und prüft echte RuntimeHost-Queue vor/nach Freigabe (0/4 Frames und exakte Samplewerte). Original-Latenz-/Cheezoids-Gate unverändert.
- Kandidaten-Startdeadline 30 s entsprechend dem bestehenden unabhängigen Original-Bank-Sweep; große SF2 dürfen länger laden. Früh beendeter Guest wird weiterhin sofort abgewiesen. Diagnose der konkreten Cheezoids-Fehlerursache künftig über tatsächliche Host-stdout/stderr-Tails; die Ursache des bisherigen Sweep-Exit 1 ist nicht bestätigt. Sämtliche neuen Prüfungen noch offen. Keine Freigabe und kein FULL AUDIT.

## 26.11.2026 — Stabilisierung 5: asynchroner Bankstart und native Bus-Grenzen

- Referenzstand `0ba53e5`: [vollständiger Original-Gesamtlauf 45715001899](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45715001899) PASS, Job 139568744292. Original-DLL-ROM-Eintritt 8/8, Validator 47/47, GUI, Original-Creative-DOS-PCM und Dual-Instanzen PASS; Portwrites 12903, Original-ROM verifiziert. p95 MIDI-to-PCM 17,7778 / 18,6042 / 17,2812 ms (44,1/48/96 kHz), je 0 Audio-/MIDI-Drops und 0 Underruns. Keine Änderung des Latenzziels. [Unit-Tests](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45715001842) und Version PASS.
- A06: aktiver Bankwechsel behält die laufende Instanz auch während Kandidat-Staging und asynchronem Gaststart. Nicht-RT-Transitionsmutex serialisiert Bank-/Activate-/Setup-Aufrufe; Runtime-Mutex wird während Kandidatenstart und maximal 5-s-UART-Wartefenster freigegeben, damit die alte Engine weiterverarbeitet. Erst UART-Bereitschaft erlaubt Ersatz; frühes Gastende/Timeout führt zum Abbruch der Kandidatenengine, ohne bisherige Bank/State/Engine zu zerstören. UART-Gate ist keine Bestätigung beliebiger SF2-Klanginhalte. Erstaktivierung bleibt asynchron. Umschalt-/Shutdown-Phase selbst kann weiterhin kurze Stille erzeugen. Neue native Bootfehler-Gegenprobe nutzt explizit synthetische ABI-DLL, die nach 300 ms ohne UART mit Fehlercode endet, während der tatsächliche bisherige Original-DOSBox-Guest per VST3 weiter PCM liefert. Reale Bankwechsel-Positivkontrolle und synchrone Rollback-Probe bleiben erhalten.
- Native VST3-Bus-ABI: ungültige negative/zu große Indizes und falsche Medien-/Richtungswerte vor geerbten SDK-Vektorzugriffen abweisen. Native Host-Gegenproben für vorhandene und nicht vorhandene Busse, Stereo-Vertrag unverändert. Kein GUI-/EMU8000-/Originaltreiber- oder DOSBox-Quellpatch-Wechsel. Neue Ergebnisse offen; beliebiger nativer Fatalpfad/Prozessabbruch weiterhin nicht allgemein isoliert. FULL AUDIT / Freigabe weiterhin offen.

## 25.11.2026 — Stabilisierung 4: Original-DLL-ROM-Eintritt fail closed

- Native [ABI-Prüfung 45714113649](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45714113649) auf `998cc4b4`: ROM-Snapshot PASS, isoliert, Löschen/Schreiben verweigert, vollständiges Lesen nach Entfernen der Quelldatei, Cleanup und fehlender ROM-Neustart abgewiesen. CC7 und Shutdown weiterhin PASS. [SF2](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45714113150), [Runtime-Faults](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45714113471), [Transport](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45714113609) und Version PASS. Original-Gesamtlauf 45714113107 bei Dokumentation noch aktiv.
- A22 / bekannter ROM-Startpfad: zusätzliche explizite ROM-Validierung direkt am exportierten originalen DOSBox-DLL-Eintritt vor main(), exakte 1 MiB, vorhandener regulärer Disk-Handle, Windows-Sperre gegen Schreiben/Löschen bis Rückkehr; fehlender Pfad bei explizitem sbawe, leerer/mehrfacher ROM-Parameter und ungültige ROM-Datei werden synchron mit -12564 abgewiesen. Kein Einstieg in standalone ROM-Such-/Download-/Empty-ROM-Fallback. SB16-Diagnostik ohne ROM bleibt möglich. Originalchip/DSP/Creative-Treiber/GUI unverändert. Kein genereller Schutz vor beliebigem E_Exit, abort, OS-Ausnahme oder Prozessabbruch behauptet.
- Neue echte Windows-DLL-Nachprobe: gültiger synthetischer ROM, sieben ungültige Fälle (fehlend, zu kurz, zu lang, leer, doppelt, Parameter fehlt, Verzeichnis), jeweils nach Ablehnung direkt derselbe DLL-Handle mit gültigem ROM und echten AWEUTIL-Registerwrites als Wiederanlaufkontrolle. In den vorhandenen Hauptworkflow integriert; früherer separater Diagnoseworkflow nur manuell, um redundante Kernbuilds zu vermeiden. Native Ausführung noch offen. Keine Freigabe.

## 25.11.2026 — Stabilisierung 3: ROM-Preflight und Original-ROM-Lebensdauer

- Ergebnis auf `24c53699`: [SF2](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45706126975), [native Runtime-Faults](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45706127035), [native ABI](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45706127136), [Transport](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45706127000) und Version PASS. Pause 0 Overflow-Drops, 51.200 bewusst nicht erfasste Frames; Stop bei 6-s-Shim-Stall 2.508 ms, Quarantäne/Reaper PASS.
- [Original-Gate 45706127057](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45706127057) Job 139528591816: Validator 47/47, GUI/MOD RESET PASS, Original-Transport-Latenzgate FAIL. Drei Messungen p95 20,1134 / 19,5833 / 17,9271 ms, jeweils 0 Audio-/MIDI-Drops und 0 Underruns. Überschreitung des 20-ms-Ziels nur 0,1134 ms; keine Pufferreduktion auf Verdacht und keine stille Gate-Lockerung. Priorität Betriebssicherheit.
- A10/A22-R: vor Original-Gaststart eine eigene unveränderliche ROM-Kopie in Instanz-Staging erstellen, exakte Größe/narrow-path-Zugänglichkeit prüfen, für originalen Creative-Gast zusätzlich exakten SHA256 der verbindlichen privaten Paketierung prüfen. Windows-Lesehandle verweigert Schreiben/Löschen bis zum tatsächlichen Worker-Ende, auch bei Quarantäne. Keine ROM-DSP-Modifikation. Neue native Shim-Gegenprobe: Quellen-ROM nach Aktivierung löschen, Gast liest weiterhin Snapshot, Schreib-/Löschversuche gegen Snapshot abgelehnt, Cleanup und erneuter Start ohne ROM. Ergebnisse noch offen. Keine Behauptung, dass damit alle DOSBox-Fatalpfade abgesichert sind.

## 25.11.2026 — Stabilisierung 1: ausgeführter Nachtest und Bus-Vertrag

- Commit `88273211`: [22/22 auf Linux und Windows](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45705048430); [Paketierung 3/3](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45705048428); [SF2 Loopgrenzen + Same-Path-Recall PASS](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45705048568).
- [Nativer VST3-Fehlertest](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45705048493): frühe Note hörbar (Peak 0,304901), Master-Hälfte exakt 0, zweite 0,247589; synchron fehlgeschlagener Bankwechsel erhält State und laufendes Audio (Peak 0,248596). Alle drei PASS. Pause weiterhin FAIL (19.968 Overflow-Frames) auf 88273211.
- [Native ABI](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45705048544): CC7-Recovery PASS; blockierender Shutdown 5.999 ms FAIL. Stabilisierung 2 adressiert beide Restfälle.
- [Vollständiger nativer Lauf](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45705048512): echter DOSBox-DLL/MPU-/PCM-/Dual-Instanz-Transport PASS, Validator jedoch 44/47 statt 47/47. Die geerbte SDK-Methode akzeptierte Mono und änderte den Bus dauerhaft; neue strikte Stereo-Prüfung im process() wies danach Mono-Buffer zurück, auch die nachfolgenden Automationstests. SDK-Quellstand geprüft; diese Ursache wird durch explizite Stereo-only-Busverhandlung und realen Host-Negativtest adressiert. Keine Validator- oder Automationstest-Abschwächung. Neuer Nachweis offen.
- Shutdown-Kandidat nutzt 1,5 s Bootreserve innerhalb des 2,5-s-Wait-Budgets, um die normale UART-Initialisierung zu ermöglichen. Keine Freigabe, kein Originalklang- oder vollständiger Auditnachweis; A22 Fatalausgang bleibt offen.

## 25.11.2026 — Stabilisierung 2: Legacy-Processing-Pause ohne FIFO-Überlauf und Alt-PCM

- A04: bei Processing-Suspend stoppt die Erfassung von PCM aus dem asynchronen Gast, absichtlich nicht angeforderte Frames separat gezählt. Auf Resume published Audioqueue in O(1) als Consumer leeren, SRC/Priming resetten, epoch-markierte PCM-Einträge verhindern Ausgabe verspäteter alter Producer-Frames. Shared CaptureGate bleibt auch bei quarantänisiertem Core gültig. Kein Lock/Heap/Wait in setProcessing; bisherige Original-Gast-Zeitbasis läuft im Legacy-Modus bewusst weiter (keine Behauptung eines virtuellen Freeze). Experimenteller Demand-Pfad bleibt unverändert.
- Neue portable Pause-/Resume-Tests inklusive absichtlich verspäteter Callback-Generation und 40.000 suspendierter Frames; vorhandener nativer Pausentest muss 0 Overflow-Drops plus explizite Suspend-Zähler nachweisen. Native/portable Ergebnisse offen.

## 25.11.2026 — Stabilisierung 2: Begrenztes Shutdown bei nichtkooperativem Worker

- A05: reale Windows-Threadbeendigung mit WaitForSingleObject statt unbeschränktem Join auf dem Hostthread; 2,5-s-Wait-Budget (mit 1,5-s-Bootreserve innerhalb des Budgets). Bei Timeout gesamte RuntimeCore-Lebensdauer an Hintergrund-Reaper übertragen, um Callback-Kontext, DLL, Thread und Staging nicht vor Ende zu zerstören. Nur im Fehlerfall VST3-Modulabbildung bis Prozessende pinnen; keine neuen Aktivierungen bis Quarantäne geräumt. Kein TerminateThread, kein Unload eines laufenden DLL-Threads. Bei OS-Pin-Fehler sicherer blockierender Fallback, bei Reaper-Startfehler bewusst Quarantäne bis Hostprozessende. Keine allgemeine Crash-Isolation (A22 bleibt offen).
- Erweiterter nativer Shim-Test: kooperativer Stop, 6-s-Stall mit tatsächlichem Timeout/Quarantäne, abgelehntem Neu-Start und anschließendem Reaping. Windows-Nachweis ausstehend. Original-DOSBox/EMU8000 und GUI unverändert.

## 25.11.2026 — Stabilisierung 1 ergänzt: Host-Master, frühe MIDI-Eingaben, CC-Recovery

- A01: Master-Gain-Punkte im Defaultpfad linear im Host-Samplebereich anwenden, mit fester 4096-Punkte-Kapazität, Flush- und Grenztests. Nicht gleichbedeutend mit samplegenauer MPU/Overlay-Zeitplanung.
- A09: MIDI-Setup und frühe Notes bereits in den FIFO einreihen, die vorhandene MPU-UART-Gate-Logik hält die Zustellung bis zum Guest-Handshake zurück; Fehler bei Queue-Ablehnung/ungültigen Noten nicht als Erfolg verschlucken. Finished-Ausgänge nullen, Stereo-/Format-/NaN-Validierung und SilenceFlags im Defaultpfad. Native Validator-/Host-Nachprüfung ausstehend.
- A02: Legacy-Recovery protokolliert abgelehnte persistente CC1/7/10/11/91/93, Channel Pressure und Pitchbend pro Kanal und sendet sie vor dem nächsten angenommenen Ereignis erneut. Sustain/Notes werden nie reaktiviert. Clocked-batch-Recovery und simultane Controllerreset-Semantik separat offen.
- Kein Original-DSP-/GUI-Wechsel. A04 echte Pause, A05 nichtkooperatives Shutdown und A22 Fatalpfade bleiben gesonderte Architektur-/Lifecycle-Aufgaben; keine unsichere Thread-Abtrennung/erzwungenes DLL-Unload.

## 25.11.2026 — Stabilisierung 1: Bank-Identität, RAM-SF2-Loopgrenzen, private Paketierung

- A03: RAM-Sample-Header mit außerhalb des Samples liegenden oder verkehrten Loop-Grenzen abweisen; neue drei synthetische Regressionen. ROM-Header/Creative-Ausnahmen unverändert; zusätzliche RAM-Instrumentzonen-Grenzprüfung mit signierten feinen/groben Offsets, Globalzonenvererbung und aktiven Loopgrenzen. Presetgenerator-/umfassende Legacy-Matrix und Originalbank-Nachprüfung weiterhin offen.
- A06: externe Bank anhand der tatsächlich validierten/stagierten Bytes statt nur Pfad identifizieren; Presetkatalog und Gaststaging lesen denselben Snapshot. Ersatzengine vor Zerstörung der alten synchron erstellen; bei synchron fehlgeschlagener Erzeugung alte Engine und State erhalten. Asynchrone Gastboot-Freigabe/Stop-Timeout noch nicht gelöst.
- A18/A21: kanonischer Zielpfad, nicht umgeleitete PRIVATE-Wurzel, Prüfungen vor Commit der Paketierung; verschachtelte PRIVATE-/Build-Verzeichnisse und exakte Originaldateinamen ignorieren. Kein Schutzversprechen gegen absichtlich parallel umbenannte Verzeichnisse.
- Relevante Audit-/Transport-Trigger ergänzt. Zusammenhängender Fixcommit, keine Änderung an GUI-Layout oder EMU8000/DOSBox-Kern. Nativer Nachtest offen; NO-GO bis Regression und Restbefunde geschlossen.

## 25.11.2026 — Native deterministische Laufzeit-ABI-Fehlerinjektion

- A05/A02: kurzer nativer Windows-Build des **unveränderten produktiven DosBoxRuntimeHost.hpp** und tatsächlicher Queue/Adapter mit expliziter synthetischer ABI-DLL. Kooperativer Stop als Kontrolle, 6-s nichtkooperativer Worker, erzwungener CC7-Verlust nach gefüllter Queue und unabhängiger MIDI-Empfangslog. Kein echtes DOSBox-/VST3-/Klang-PASS; gezielte native Host-/Transportprüfung. Kein Produktcode geändert. [Windows-ABI-Audit 45703606784](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45703606784), Job 139519757621 auf 30c62801, Build PASS, alle drei Fälle ausgeführt: kooperativer Stop 16 ms, nichtkooperativer Worker blockiert stop() 6005 ms (A05 FAIL im kontrollierten nativen Host-Fault-Test, 3-s-Auditgrenze); CC7=0 ausgeliefert, CC7=127 bei gefüllter Queue abgelehnt, neue Note nach Recovery tatsächlich zugestellt ohne CC7-Wiederherstellung (A02 FAIL). Reale native RuntimeHost-Implementierung mit kontrollierter Test-DLL, nicht der Originalgast, keine echte PCM-Aussage.

## 25.11.2026 — Native Original-DLL-Fatalpfadprüfung

- A22: gezielte isolierte Original-DOSBox-DLL-Prüfung mit synthetischem 1-MiB-ROM als Positivkontrolle und absichtlich fehlendem ROM plus blockierter temporärer ROM-Fallback-Datei. `-test` verhindert ROM-Downloadfrage; nur ein eigenes E_Exit-Dialogfenster des isolierten Kindprozesses wird quittiert, tatsächlicher Prozess-Exit/Timeout und Originaldiagnose werden getrennt geprüft. Keine Änderung am Originalkern/Produkt und keine privaten Daten. Direkte DLL-Fehlerinjektion, nicht behauptet, dass jede normale VST3-Aktivierung den Fehlerfall auslöst. [Erster Lauf 45703219472](../../actions/runs/45703219472): Testmodus-Positivkontrolle exit 0 ohne Rückkehrmarker/Trace, missing-ROM E_Exit-Dialog und 35-s-Timeout; spezifischer Fatalpfad und Prozessabbruch NOT VERIFIED. Prüfaufbau korrigiert: bewährter normaler AWEUTIL-Positivlauf ohne -test, -test nur für Downloadunterdrückung im Missing-ROM-Kind; exakter modaler Dialogtext per WM_GETTEXT und gezielter IDOK/BM_CLICK. Produkt und bestehende Klanggates unverändert. [Zweiter nativer Versuch 45703486677](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45703486677), Job 139519344518 auf 332d9ed3: normaler Positivlauf kehrt mit 0 zurück, 9.688 echte EMU8000-Registerwrites. Missing-ROM-Fehlerkind zeigt tatsächlich E_Exit-Dialog und kehrt 35 s lang nicht zurück; Test-Watchdog beendet es. Exakter Dialogtext nicht erfasst (nur OK), automatische Quittierung erfolglos; spezifischer beabsichtigter Fatalgrund und natürlicher Prozess-Exit weiterhin NOT VERIFIED. Direkte Original-DLL-Modal-/Timeout-Beobachtung, kein realer DAW-Absturz behauptet. Kein Originaldownload.

## 25.11.2026 — Native Windows-Paketgrenzen adversarial prüfen

- Separate kurze datenfreie Windows-Gegenprobe A18/A21: echter Windows-Verzeichnisjunction, `..` und direkter Zielpfad mit derselben produktiven Paketierungsfunktion, Hashs ausschließlich für selbst erzeugte Fakedaten; unabhängige Git-ignore-Proben in einem temporären Git-Repository für die private Buildstruktur. Kein Besitzerinhalt, kein tatsächlicher Git-Add/Upload, kein Produktbuild. [Windows-Paketprüfung 45702913863](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45702913863), Job 139517374512 auf 02dbd2f4, abgeschlossen FAIL mit drei erwarteten Vertragsfehlern: reguläres Staging und direkte Außenpfad-Ablehnung PASS; `..` und echter Windows-Junction werden als Ziel außerhalb PRIVATE akzeptiert; `DEMO.EXE` im privaten Build-Bundle nicht gitignored. ROM und SF2 dort unter Windows ignoriert, root PRIVATE alle drei PASS. Nur Fake-Dateien; kein Leak, kein tatsächlicher Git-Add/Push.

## 25.11.2026 — Native adversariale Laufzeitfehlerprüfungen

- Audit-only, Produktquellen und Originalkern unverändert. Zusätzlicher separater Windows-Workflow mit echter VST3/DOSBox-Laufzeit, synthetischem MIDI/PC-Speaker-Gast und eigenem nativen Host: frühe Note, Processing-Pause mit Queue-Zählern, Master-Automation innerhalb eines Blocks, aktiver SF2-Wechsel bei absichtlich fehlender installierter Runtime-DLL. Positive PCM-Kontrolle, separate Kindprozesse/Timeouts, kein Originalklangnachweis aus PC-Speaker.
- [Windows-Lauf 45702696527](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45702696527), Job 139516622535 auf 6f932056, Build PASS, fünf isolierte native Fälle ausgeführt. A09 frühe Note Peak 0 bei späterer positiver Note 0,304901, null Process-Fehler; A04 Pause 1,2 s: 19.968 verworfene Audioframes, volle 32.768-Frame-Queue; A01 Master-Soll erste Blockhälfte 0, Ist 0,249115, zweite 0,248444; A06 fehlende Runtime-DLL während Bankwechsel: setState Fehler, alte Statebytes unverändert, bisherige Audioausgabe Peak 0; intakter aktiver Wechsel mit Peak 0,304901 PASS. Keine ungültigen/unbeschriebenen Ausgabesamples in diesen Fällen. Vier native Vertragsfehler, keine Infrastrukturfehler; Transport 45702696490 und Version 45702696462 SUCCESS. Synthetic-PC-Speaker, kein Original-Creative-Klangtest. PARTIAL / NO-GO, kein Release, GUI/EMU8000 unverändert.

## 25.11.2026 — Adversarialer Audit: nativer SF2-State-Negativtest ergänzt

- Produktquellen, P165-GUI und DOSBox-X-Pin unverändert gegenüber `16d139b5d6602355e97c70d00ce6558a5ba8615f`. Neue separate datenfreie native Auditprüfung für A03 (ungültige Sample-Loop-Grenze) und A06 (erneute Auswahl/Recall einer gültigen, am selben Pfad ausgetauschten Bank); Positivkontrolle mit unterschiedlichen Bankpfaden und Negativkontrolle mit fehlender Bank/ungültigem Programm. Tatsächliche IComponent- und IEditController-Schnittstellen des gebauten VST3, kein Gaststart und kein Originalklang-PASS. Der isolierte Windows-Workflow nutzt ausschließlich generierte SF2 und startet einmal beim Audit-Testcommit; keine private Datei, kein Release und keine Änderung an bestehenden Produktgates.
- [Windows-Audit 45701077711](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45701077711), Job 139511005412 auf d9ea6d05: Build und reale VST3-Initialisierung erfolgreich, gültige unterschiedliche Bankpfade und negative fehlende/ungültige Bank-/Programm-Kontrollen PASS. Zwei echte Vertragsfehler: `AUDIT_A03_LOOP_BOUNDS FAIL result=0` (ungültiger Loop-Endpunkt 0xffffffff akzeptiert), `AUDIT_A06_SAME_PATH_RECALL FAIL result=1 currentProgram=0` (ausgetauschte gültige Bank am selben Pfad mit Programm 17 nicht zurückgerufen). `AUDIT_NATIVE_BANK_STATE_EXECUTED failures=2`, Exit 1, keine Infrastrukturstörung. Transport 45701077726 und Version 45701077685 SUCCESS. Inaktive State-Prüfung, kein Gaststart; aktive Rollback-/PCM- und andere native Fehlerfälle bleiben offen. Produktcode unverändert, PARTIAL / NO-GO; kein FULL AUDIT oder Release.

## 25.11.2026 — ETW-CPU-Messung nativ bestanden; Echtzeit-Pufferreserve angepasst

- Windows 45647772582 auf fbf0b1d9: ETW-CPU-Messung mit Busy-/Idle-Kontrollen bei 44,1/48/96 kHz PASS. Prozesslast 10,4782/11,7349/11,1827 % eines logischen Kerns, unter Polyphonie 12,0292/12,1302/12,7183 %. Keine Messung auf dem Besitzer-PC. p95 13,9002/14,6042/13,8646 ms. Alle Callback-Fristen eingehalten.
- Experimenteller Offline-Zeitpfad erneut PASS (drei Raten), alle 128 Cheezoids-Presets im Vergleich, Send-/Effektregister und unabhängiger Original-Creative-DLL-Klangtest bestanden; Transport 45647772654 und Version 45647772761 SUCCESS.
- Gesamt-FAIL: 48-kHz-60-s-Test meldet 59 fehlende native PCM-Frames bei minimal 73 gepufferten Frames, maximal 8,0011 ms Host-Wakeup-Verspätung; 44,1/96 kHz null Unterläufe. Ursache der konkreten Verzögerung noch nicht eindeutig isoliert.
- Kandidat: anfängliche native PCM-Reserve des bisherigen Echtzeitpfads um 192 Frames (4,35 ms) erhöhen, von mindestens 384 auf mindestens 576 Frames. Für größere Blöcke ebenfalls +192. Originalkern, Treiber, GUI, Ringkapazität und experimenteller Host-Demand-Pfad unverändert. Null-Unterlauf-Gate und bisheriges p95-Ziel bleiben zwingend. Wirkung und Latenz noch nicht nativ geprüft.
- Audit bleibt PARTIAL / NO-GO; offene Lebenszyklus-, State- und weitere Auditbefunde sowie unabhängiges Codex-Nachaudit nicht als bestanden ausgeben. 0.0.0-dev, nur main.

## 25.11.2026 — Native Offline-Zeitsteuerung PASS; CPU-Messung auf ETW-Kontextwechsel umgestellt

- [Windows 45643787800](../../actions/runs/45643787800) auf a09c1751: experimenteller Host-Demand-Pfad besteht erstmals tatsächliches Windows-VST3-Offline-Rendering, NoteOn-Offset-Gegenprobe und Processing-Pause/Fortsetzung bei 44,1/48/96 kHz. Gemeldete Latenz 986/1047/1581 Hostsamples; beobachtete Offsetdifferenz 415/421/416 bei Soll 384 und bisheriger 2-ms-Toleranz. Kein samplegenauer Nachweis und keine Freigabe als produktiver Standardpfad.
- Bisheriger Echtzeitpfad: p95 13,4921/12,8958/12,9583 ms, 9.254/28.522/14.269 Queue-Blöcke, jeweils null Unterläufe, Audio-/MIDI-Drops, stumme gehaltene Blöcke und Callback-Fristüberschreitungen; 60-s-Polyphonie, Master-PCM-Recall und Sustain PASS. [Transport 45643787854](../../actions/runs/45643787854) und [Version 45643788163](../../actions/runs/45643788163) SUCCESS.
- Gesamt-FAIL: GetProcessTimes meldet 44,1/96 kHz exakt 0 Zeiteinheiten trotz 5.989.800.923/6.238.868.245 Prozesszyklen. Bei 48 kHz in 76,86 s lediglich 15,625 ms trotz über 25 Milliarden Zyklen. Keine belastbare CPU-Zeit. Intel dokumentiert diese Fehlabtastung bei kurzen Rechenquanten; Microsoft empfiehlt für präzise CPU-Analyse Kontextwechsel und warnt vor Umrechnung der CPU-Zyklen in Zeit.
- Neuer Testhost-Kandidat: eigene begrenzte ETW-Systemlogger-Sitzung mit Thread-/CSwitch-Ereignissen, QPC-Messfenstern, Auswertung erst nach Gast-Shutdown, Prüfung auf Verlust/fehlende Zeitabdeckung und separate Busy-/Idle-Kontrolle. Ermittelt geplante Thread-Laufzeit im Windows-CI-Gast (einschließlich möglicher DPC/ISR-Anteile), keine physische Host- oder Besitzer-CPU-Messung. Originale Zeit-/Zykluszähler bleiben nur Diagnose. Keine ETL-/Originaldaten hochladen; temporäre Datei löschen. Keine Produktänderung.
- Portable Kontextwechsel-Auswertung inkl. überlappender Threads, Thread-ID-Wiederverwendung, fehlendem/doppeltem Switch-in, abgeschnittenem Trace und ungeordneten Events in Release und ASan/UBSan lokal PASS. Exakte portable Test- und Header-Git-Blobs d27a039d431656874c483127ea3d37b8606bf06b und 77a56b3a2e1c84d6ebb0c86f31bbd9ea7420ba1f. Die neue Windows-ETW-Erfassung ist **nicht nativ kompiliert oder ausgeführt**; ETW-Berechtigungen, Traceformat und Erfassung unter CI noch zu bestätigen. Unverifizierte CPU-Messung bleibt Gesamt-FAIL.
- Unabhängige Cheezoids-/Send-/Effektgates werden nicht mehr allein durch CPU- oder Timingfehler unterdrückt. Beide Gate-Exitcodes bleiben vor dem abschließenden Gesamt-PASS Pflicht. PARTIAL / NO-GO; 0.0.0-dev. Kein Studio-Pro-Test, keine Releasefreigabe.

## 25.11.2026 — Original-SF2 native repariert; CPU-Orakel und Timing-Gates unabhängig

- [Windows 45639030249](../../actions/runs/45639030249) auf b59400f4: VST3-Build, 47/47 Validator, Dual-Original-Creative, SF2-Recall und aktiver SF2-Gastneustart, vier RAM-Spektralprüfungen, zehn Regler mit Neutralrestore, GUI/MOD RESET PASS. Transport [45639030223](../../actions/runs/45639030223) Linux/Windows und Version [45639030243](../../actions/runs/45639030243) SUCCESS.
- Gesamt-FAIL: 44,1-kHz-Performance-Test meldete ungültige oder flache CPU-Zähler trotz bestandener Positivkontrolle; der genaue fehlerhafte Zähler war nicht protokolliert. 48/96 kHz: 28.522/14.269 Queue-Blöcke, 0 Audio-/MIDI-Drops und Unterläufe, p95 14,9375/14,6146 ms. Die sehr niedrigen gemeldeten CPU-Prozentwerte sind kein belastbarer Nachweis für die Besitzer-CPU. Experimentelles Timing und Cheezoids-Sweep wurden nicht erreicht.
- Testkorrektur ohne Produkt-/DSP-/GUI-Änderung: Rohintervalle für Prozesszeit und Zyklen getrennt protokollieren; flache Zeit, flache Zyklen, rückläufige Zähler und Abfragefehler unterscheiden. Keine Umrechnung von CPU-Zyklen in CPU-Zeit. Lokaler Release- und ASan/UBSan-Test der Klassifikation PASS. Ungültige CPU-Messungen bleiben NOT VERIFIED und Gesamt-FAIL, aber unabhängige PCM-, Latenz-, Sustain-, Recall- und Queue-Prüfungen laufen samt regulärem Shutdown zu Ende.
- Timing-Gate läuft unabhängig vom Performance-Ergebnis für alle drei Raten, mit begrenzter test-eigener Fehlerdiagnose; beide Gates bleiben zwingend vor langem Sweep. Native Bestätigung dieser Testkorrektur offen. Audit PARTIAL / NO-GO; 0.0.0-dev, kein Studio-Pro-Praxistest, kein Release.

## 25.11.2026 — Original-GM-SF2: belegte historische ROM-Zone kompatibel halten

- [Windows 45634211957](../../actions/runs/45634211957) auf b9321a20: Gesamt-FAIL beim externen GM-SF2-Recall (IComponent::setState), noch vor Gaststart. DLL- und VST3-Build, Original-UART-/PCM-/Hardware-Smokes, synthetische Einzel-/Dual-VST3, Negativkontrolle, Steinberg-Validator, P165/MOD RESET, authentischer Original-Einzelklang (99 aktive Notenblöcke) und Dual-Creative-PCM/Lifecycle bestanden. [Transport 45634212099](../../actions/runs/45634212099) Linux/Windows und [Version 45634212149](../../actions/runs/45634212149) SUCCESS. Spätere Klang-, Timing-, CPU- und Performance-Gates wurden nicht erreicht.
- Ursache am tatsächlichen 42.726-Byte-Original-SYNTHGM.SF2 (Archiv-Git-Blob 205a540e5545945b58ab136ef5423327e2066d2c) untersucht: Instrument 172, Bag 649 enthält zwei Generatorgruppen mit jeweils sampleID 127, ein ROM-Mono-Sample. Der neue Validator lehnt den zweiten Verweis pauschal ab. Die unabhängige Strukturanalyse der Originalbank fand genau diese Verletzung der neuen Linkregel. Keine Änderung der Originalbank oder des Creative-Laders.
- Eng begrenzte Ausnahme nur für wiederholte identische, gültige ROM-Sample-Verweise innerhalb einer Instrumentzone; verschiedene Sample-IDs, RAM-Duplikate, terminale/ungültige IDs und doppelte Preset-Instrument-Links bleiben unzulässig. Lokaler C++20-Gegenversuch: neuer synthetischer ROM-Positivfall scheitert am alten Code, besteht am korrigierten; drei neue Negativfälle abgewiesen, alle bisherigen 27 synthetischen Fälle und acht Besitzerbanken erneut korrekt behandelt. Keine Originalbytes in Git. Das Verhalten der tatsächlichen Originalbank im nativen VST3 bleibt bis zum neuen Windows-Lauf offen.
- Keine Änderung der GUI, des EMU8000-Kerns, des experimentellen Timing-Modus oder bestehender Klangtestgrenzen. PARTIAL / NO-GO, 0.0.0-dev.

## 25.11.2026 — Audit-Korrekturkandidat (PARTIAL / NOT VERIFIED)

- Referenz: `164d2141cefdcaacb3806b37ec84d66fcb25f4c2`. Vorherige native CI 45609264124, Transport 45609264137 und Versionsprüfung 45609264252 erfolgreich. Die dort ausgegebene CPU-Nullmessung ist kein belastbarer Leistungsnachweis.
- A02: MIDI-Overflow-Recovery für verlorene Note-Offs und Sustain. A03: struktureller SF2-Preflight; synthetische Positiv-/Negativdateien werden ausschließlich im Build-Verzeichnis erzeugt. Die portablen Kandidaten wurden lokal mit Gegenproben getestet.
- A01/A04/A07: Host-Demand-Timing als ausdrücklich experimenteller Opt-in-Pfad; der bisherige Gast-Takt bleibt Standard. Native Timing-, PCM-, Offline- und Pause/Resume-Gegenproben sind ergänzt, aber noch nicht auf Windows ausgeführt. CPU-Messung (A08) mit Positivkontrolle und Ablehnung unplausibler Nullmessungen.
- A05/A06/A09: transaktionaler Laufzeitwechsel und Zustandswechsel noch nicht integriert. A10–A14 sowie Gesamt-Audit und unabhängiger Audit offen. Kein FULL AUDIT, kein Release, keine Freigabe als fertiges Plugin. Version bleibt `0.0.0-dev`.
- Alle nativen Ergebnisse dieses Kandidaten stehen aus. P165-GUI, Original-Creative-Dateien und gepinnter EMU8000-Quellkern bleiben unverändert. Keine privaten Bänke im Repository.

## 25.11.2026 — Echter synthetischer Dual-VST3-Test PASS; Original-Launcher korrigiert

- [Windows 45535905418](../../actions/runs/45535905418): DLL-Build, zwei unabhängige DLLs ohne Host-SDL-Variablen, echter Einzel- und Dual-VST3-Synthetic-PCM-/Lifecycle-Test, Negativkontrolle, Steinberg-Validator und P165/MOD RESET PASS. Original-Creative-Einzel-VST3 liefert erneut musikalisches PCM (98 aktive Notenblöcke, 58105 nichtstumme Frames, kein Near-Full-Scale-Frame im gehaltenen Ton). Gesamt-FAIL beim Aufruf des neuen Dual-Originaltests: Windows PowerShell 5.1 wertet normale native stderr-Ausgabe `LOG: Early LOG Init complete` bei `2>&1` und ErrorActionPreference Stop als NativeCommandError. Kein Dual-Original-PASS und keine nachfolgenden Gesamtgates. Transport 45535905360, Version 45535905267 SUCCESS.
- Nur Testlauncher geändert: vorhandener Python-Hostrunner startet --dual-original mit 90s Watchdog, getrennt erfasstem stdout/stderr, unverändertem zwingendem Exit 0 und exakt einmaligem vollständigem nativem PASS-Marker. Datenfreie Regression für normale stderr-Ausgabe, fehlenden/falschen/doppelten Marker und Nichtnull-Exit. Kein roher Original-DOS-Log-Upload. Produktcode, GUI, EMU8000 und alle nativen Prüfbedingungen unverändert; Windows-Gesamtnachweis weiterhin offen.

## 25.11.2026 — SDL-Dummy-Start bestätigt; Direct3D-Neustart unterbunden

- [Windows 45533700405](../../actions/runs/45533700405) FAIL nach 5:09, nun im frühen Dual-DLL-Gate ohne SDL-Umgebungsvariablen. Native Diagnose bestätigt ausdrücklich `Video dummy, Audio dummy`: vorheriger Hint-Fix wirkt. Erster Gast stürzt danach mit Access Violation 0x10 vor MIDI-Ready ab; VST3-Build und spätere Gates nicht erreicht. Transport 45533700459 und Version 45533700439 SUCCESS.
- Entscheidender Logunterschied zum erfolgreichen Lauf 45531460147: nur im fehlschlagenden umgebungsfreien Lauf erscheint `Resetting to WINDIB mode`. Gepinnter Originalcode `src/output/output_direct3d.cpp:d3d_init` prüft getenv statt aktiven SDL-Treiber, beendet das Video-Subsystem und initialisiert es erneut; der zuvor erzeugte Dummy-Fensterzustand kann dadurch ungültig werden. Embedded-only frühe Rückkehr über vorhandenes `OUTPUT_SURFACE_Select()` vermeidet diesen gesamten unnötigen Direct3D-/WINDIB-Pfad und entspricht dem bisherigen erfolgreichen Surface-Fallback. Standalone-Code bleibt vorhanden; neuer Quelltest, unveränderte strenge Dual-DLL-Prüfung ohne Host-Variablen. EMU8000, Creative und P165 unverändert. Kausalität des Absturzes und neuer Windows-PASS noch zu bestätigen.

## 25.11.2026 — SDL-Dummy-Video-Bootstrap: fehlender interner Hint korrigiert

- [Windows 45531460147](../../actions/runs/45531460147) FAIL im ersten synthetischen VST3-PCM-Test: native Diagnose `Can't init SDL dummy not available`, 40s Watchdog. Vorherige Original-DLL- und Dual-DLL-Gates sowie VST3-Build PASS; Transport 45531460136 und Version 45531460085 SUCCESS. Echter Dual-VST3-Test und Originalklangregression nicht erreicht.
- Konkreter Unterschied im gepinnten SDL-Quelltext belegt: `vs/sdl2/src/video/dummy/SDL_nullvideo.c` verlangt in `DUMMY_Available()` den Hint `SDL_HINT_VIDEODRIVER=dummy`, selbst wenn `SDL_VideoInit("dummy")` explizit aufgerufen wird. Der erfolgreiche Dual-DLL-Test setzte bislang beide SDL-Umgebungsvariablen; der normale VST3-Host nicht. Die statisch eingebettete SDL-Kopie setzt jetzt ihren eigenen Video-Hint mit `SDL_HINT_OVERRIDE` vor dem VideoInit-Aufruf, ohne Prozessumgebungsvariable. Der frühe Dual-DLL-Test startet jetzt ohne beide SDL-Driver-Umgebungsvariablen und behält sämtliche MIDI-/PCM-/Shutdown-Kriterien. Quellankerregression ergänzt. EMU8000, Creative-Klangpfad und P165 unverändert; neuer Windows-Nachweis offen.

## 25.11.2026 — VST3-Mehrfachinstanz-Build kompiliert; früher Synthetic-Timeout

- [Windows 45529738628](../../actions/runs/45529738628) FAIL erst beim ersten synthetischen VST3-PCM-Test (40s Watchdog); beide getrennten DLLs inklusive Shutdown erneut PASS, VST3 und neuer nativer Testhost kompiliert. Transport 45529738663 und Version 45529738632 SUCCESS. Echter Dual-VST3-Test und spätere Klangregression noch nicht erreicht. Bisherige Eininstanz-Referenz 6a80bb94 bleibt bestanden. Timeout-Ursache nicht endgültig nachgewiesen, weil Python bisher die native Ausgabe bei Timeout verwarf.
- Konkrete Startpfadabweichung behoben: statisches eingebettetes SDL wählt von der ersten Initialisierung an Dummy-Audio UND Dummy-Video (wie der erfolgreiche separate Dual-DLL-Test), nicht erst beim Mixer. Standalone-Treiberwahl kann im eingebetteten Pfad nicht mehr `using_windib=true` trotz Dummy-Video setzen oder SDL-Treiber-Umgebungsvariablen ändern. Ankerregressionen erweitert. Synthetischer Timeout druckt jetzt native stdout/stderr-Tails und bleibt ein hartes FAIL (keine Zeitgrenze erhöht). Keine DSP-/GUI-Änderung oder weiteren Features. Frühe Gates unverändert vor Archivtests; vollständiger Nachweis offen.

## 25.11.2026 — Dual-DLL-Gate und Gesamtregression SUCCESS; echte VST3-Mehrfachinstanz implementiert

- [Windows 45525422435](../../actions/runs/45525422435) auf 6a80bb94 vollständig SUCCESS: zwei getrennte DLL-Module, MIDI/PCM-Isolation, A-Stop/Join/Unmap und B-PCM danach, B-Stop/Join/Unmap, Exit 0x00000000. Kein Absturz mehr in der synthetischen Forschungsprobe. Alle bisherigen Einzelinstanz-Gates einschließlich 10/10 Regler, MOD RESET zweimal, Steinberg 47/47, Cheezoids 128/128 sowie 44,1/48/96 kHz ohne Unterläufe/Drops erneut PASS. Transport 45525422262 und Version 45525422233 SUCCESS.
- Produktintegration: exklusives Ownership durch separate, bytegleiche Original-DLL-Kopie pro Aktivierung mit eindeutigem Basename und Verzeichnis ersetzt. Nur nicht-RT CopyFile/LoadLibrary, bestehender Stop/Join/Callback-Freigabe/Unmap-Pfad und temporäre Bereinigung. Unveränderte private Quelldateien; fallback Bootlog pro D:-Staging, bestehende explizite Einzeltestlogs unverändert. Eingebettete statische SDL2 wählt den bereits im Dual-DLL-Test erprobten Dummy-Video-Treiber explizit statt prozessweiten SDL_app-Fensterklassen; keine externe Emulator-EXE, keine Änderung am EMU8000-/Creative-Klangkern oder P165-GUI. Neue native Zwei-Komponenten-Tests mit tatsächlichem IComponent/IAudioProcessor/IEventList, getrennten Masterwerten, A-only, simultanem Ton, A-Teardown und B-PCM danach, sauberem B-Teardown; zuerst synthetisch, zusätzlich mit verifizierten originalen Creative-Archivdaten und deaktiviertem PC-Speaker/OPL. Neuer vollständiger Windows-Nachweis ausstehend; keine Release-/Studio-Pro-Freigabe. Parallel laufende Bankwechsel, mehr als zwei Instanzen und private Besitzer-DAW noch nicht abgenommen.

## 25.11.2026 — Mehrfachinstanz-Shutdown: native Callback-Kollision eingrenzen

- [Windows 45523468156](../../actions/runs/45523468156) FAIL: getrennte Module, MIDI/PCM-Isolation, A Stop/Join, Callback-Abmeldung, FreeLibrary/Unmapping sowie B-PCM nach A-Join und nach A-Unload sämtlich PASS. Erst B-Stop nach A-Unmap endet in nativer Access Violation, Exit 0xc000041d (unbehandelte Ausnahme im Windows-Callback), kein B-Main-Return. Native Fehleradresse/Stack noch unbekannt. Transport 45523467969 und Version 45523468058 SUCCESS. Eininstanz-Gesamtnachweis bleibt 28b923e7.
- Konkret im gepinnten Originalquelltext geprüft: sdlmain.cpp registriert mit SetConsoleCtrlHandler einen prozessweiten Handler, der SIGTERM auslöst; keine korrespondierende Abmeldung im main-Cleanup. SDL2-Windowsjoystick initialisiert eine feste Win32-Klasse `Message` über GetModuleHandle(NULL), meldet bei fehlgeschlagener Registrierung im Cleanup dieselbe Klasse ab. Beide Einrichtungen sind für das eingebettete MIDI-Instrument unnötig. Nur für AWE64_VST3_RUNTIME werden ConsoleCtrlHandler-Registrierung und physische SDL-Joystick-Erkennung ausgelassen (0 Host-Joysticks); Standalone-Zweige erhalten. Anker-/Präprozessor-Regression ergänzt. Das ist gezielte Beseitigung nachweisbarer Prozessressourcen-Konfliktrisiken, noch KEIN Beweis, dass dies die konkrete Access Violation behebt. Keine EMU8000-/Creative-/GUI-Änderung, keine Änderung am exklusiven VST3-Runtime-Ownership, neue Windows-Gates ausstehend.

## 25.11.2026 — Parallele DLL-MIDI/PCM-Isolation belegt; Shutdown separat lokalisieren

- [Windows 45519885347](../../actions/runs/45519885347) FAIL im neuen frühen Forschungs-Gate, nicht im Produkt: `DUAL_DISTINCT_MODULES_PASS` und `DUAL_RUNTIME_MIDI_PCM_ISOLATION_PASS` belegen zwei gleichzeitig aktive DLL-Kopien und voneinander getrennte synthetische MIDI/PIT-PCM-Pfade. Kein Shutdown-PASS; Kindprozess endet unmittelbar danach ohne Fehlercode/Stack im veröffentlichten Bericht. Exakte Ursache noch unbekannt (Stop-Aufruf, Main-Cleanup, Join, Callback-Abmeldung oder Unload). Kein Mehrfachinstanz-VST3-/Creative-Klangnachweis. Vorherige Einzel-DLL-Gates PASS; Transport 45519885300 und Version 45519885221 SUCCESS. Vollständiger Eininstanz-Referenzstand bleibt 28b923e7 / 45515971767.
- Forschungsprobe trennt jetzt Stop/Join, Callback-Abmeldung, DLL-FreeLibrary und tatsächliches Unmapping; weiterlaufende B-PCM nach A-Join und erneut nach A-Unload zwingend. Ungepufferte Stage-Marker, native Windows-Exitcode in Hex, Python-Faulthandler/Thread-Dumps und erweiterter Diagnoseauszug. Nur synthetischer Tonhaltezeitraum verlängert (36 auf 126 BIOS-Ticks), damit Überlebensmessung nicht zufällig in natürliches Tonende fällt. Keine gelockerten Isolation-/Shutdown-Gates, keine Produkt-/DSP-/GUI-Änderung. Gemeinsame Win32-Prozessressourcen im Upstream identifiziert (ConsoleCtrlHandler, CWD, SDL-Umgebung/Windowclass); erst anhand tatsächlicher Diagnose korrigieren. Neuer Lauf offen.

## 25.11.2026 — MOD RESET und Gesamtregression SUCCESS; Mehrfachinstanz-Forschung

- [Windows 45515971767](../../actions/runs/45515971767) auf 28b923e7 vollständig SUCCESS. Echter MOD RESET Klick zweimal bestanden, je 12 Parameter null, 12 ausgeglichene Host-Gesten, 5 unabhängige Werte erhalten, Zustandsversion korrekt. P165-Lifecycle 1000x708, Steinberg 47/47, Regler-PCM 10/10, Cheezoids 128/128, alle drei Performance-Raten ohne Audio-/MIDI-Drops und Unterläufe. Transport 45515971818 und Version 45515971748 SUCCESS. Dies ist der aktuell abgenommene Eininstanz-Referenzstand.
- Mehrfachinstanzfähigkeit noch nicht vorhanden. Bestehendes exklusives Runtime-Ownership und Produktcode unverändert; keine Annahme, dass DLL-Kopien bereits sichere Instanzen ergeben. Neuer isolierter Windows-Forschungsprobe: zwei byteidentische, unterschiedlich benannte DLL-Kopien mit statischer SDL2 im selben Testprozess, getrennte temporäre DOS-Pfade, selbst erzeugte MIDI/PIT-EXE mit verschiedenen MIDI-Triggern/Tönen, Null-ROM. Beide gleichzeitig bereit, A allein getriggert (B muss stumm bleiben), dann B, A stoppen und entladen während B weiter PCM liefern muss, beide sauber schließen. Frühes Gate vor VST3-Build; kein Creative-/VST3-Mehrfachinstanznachweis und keine Freigabe. Mögliche gemeinsame Prozessressourcen (CWD, SDL/Win32, Umgebungsvariablen) bleiben zu untersuchen. Kein zusätzlicher externer Prozess im späteren Produkt; Testprozess ausschließlich zur Crash-/Hang-Isolation. Neuer Forschungsnachweis offen.

## 25.11.2026 — MOD-RESET-Workflow-Syntax repariert

- Run 45515877422 scheiterte bereits beim Einlesen der Workflow-Datei: versehentlich umgebrochener PowerShell-Regexabschluss in YAML. Kein Windows-Job und kein MOD-RESET-Funktionsbefund. Regex auf eine vollständige Zeile korrigiert; Bytezugriff im neuen State-Test vorsorglich mit explizitem reinterpret_cast unabhängig vom SDK-Pointertyp. Produkt und bisheriger SUCCESS-Prüfstand unverändert. Neuer Windows-Nachweis ausstehend.

## 25.11.2026 — Vollständiger Regler-Gesamtlauf SUCCESS; echtes MOD RESET GUI-Gate

- [Windows 45512094711](../../actions/runs/45512094711) auf c00a9034e vollständig SUCCESS. LF/CRLF-Parser und Negativkontrollen PASS; echte PCM-Regler 10/10 bei Mitte/Maximum, Neutralrestore, Filter-Referenz, neun Ruhefenster und alle drei unabhängigen DCYSUS-Feldgates PASS. Steinberg 47/47, Cheezoids 128/128, P165-Lifecycle 1000x708 mit zweimaligem Open/Close. 9254/28522/14269 gemessene Blöcke bei 44,1/48/96 kHz ohne Audio-/MIDI-Drops und ohne Unterläufe. Transport 45512094470 und Version 45512094498 SUCCESS. Regler-Klangfunktion auf diesem Prüfstand bestätigt, keine weiteren Klangänderungen vorgesehen.
- Neues natives MOD-RESET-Gate im vorhandenen Win32-Editorhost: alle zwölf Modulationsparameter zunächst ungleich null, Master/Bank/Programm und Effekttypen separat sichern, echte Win32-Mausnachrichten an eindeutig identifiziertes Editor-Kindfenster an Original-Buttonmitte 884/586 (an Clientgröße angepasst), keine direkte Reset-Methode. Alle zwölf Werte müssen null werden, fünf unabhängige Werte erhalten bleiben, zwölf vollständige Host-Gesten begin/perform/end mit Wert null, v3-Komponentenzustand statt v4/v5. Zwei Durchgänge beim bisherigen Editor-Open/Close. Editor-Gate vor teurer Archivprüfung; bei frühem FAIL kein Cheezoids-Sweep. Kein Produkt-/GUI-Code geändert, neuer Windows-Nachweis ausstehend. Mehrfachinstanzfähigkeit derzeit nicht gegeben (exklusives DOSBox-Runtime-Ownership), separates Architekturthema; privates Paket und Studio-Pro-Besitzerabnahme weiterhin offen.

## 25.11.2026 — Alle zehn Regler PCM-PASS; Python-CRLF-Auswertung korrigiert

- [Windows 45509839214](../../actions/runs/45509839214) insgesamt FAIL ausschließlich im Python-Reportparser, nachdem der native Host `REAL_VST3_VOICE_PCM_SUMMARY=10/10 NEUTRAL_RESTORE=PASS` und `REAL_VST3_VOICE_CONTROLS_PCM_PASS` gemeldet hatte. Alle neun Filter-Ruhefenster bestanden, Referenz gültig (Peak 0,018775, late held 0,0179317, Release 450 ms 0,00541522). Filter Decay Mitte/Max 0,000894323/0,00092274 > 0,000366615; Sustain 0,000682304/0,00110327 > 0,000366615; Release 0,000468215/0,000660281 > 0,000387587. Unabhängiger Trace 26 passende Paare, alle drei Feldgates PASS (+44/+88, -32/-64, +44/+88). Steinberg 47/47 und übrige Vorprüfungen PASS. Keine vollständige Gesamtregression dieses Runs, da Parser den nachfolgenden Performance-/Cheezoids-Teil blockierte.
- Ursache: native stdout aus binärem Windows-Spool hat CRLF; Regex mit `$` und `re.M` erwartet LF ohne CR. `splitlines()` vor vollständigem zeilenweisen Match; exakte zehn eindeutige IDs, neun Ruheberichte, eindeutige 10/10-/Neutralrestore-/PASS-Summaries. Portable Regression für LF/CRLF und Negativkontrollen (fehlend, doppelt, FAIL, falsche ID, falsche Summary), Pflicht vor Windows-Build. Keine Änderung an Produktcode, GUI, musikalischen Prüfschwellen oder Fixtures. Neuer Gesamt-Windows-Nachweis ausstehend. Weiter offen: tatsächlicher MOD RESET GUI-Test, Mehrfachinstanzfähigkeit (derzeit absichtlich exklusiver DOSBox-Globalzustand), privates installierbares Paket und Besitzerabnahme in Studio Pro. Kein Release.

## 25.11.2026 — Alle drei DCYSUS-Felder nachgewiesen; PCM-Referenzstreuung isolieren

- [Windows 45508070522](../../actions/runs/45508070522) FAIL: sieben Regler erneut PCM-PASS, Neutralrestore und Program-17-Gültigkeitsprüfung PASS (Peak 0,0186952, late held 0,0179354, Release 450 ms 0,0141889). Filter Decay/Sustain/Release scheitern am PCM-Gate, weil die drei Neutralreferenzen selbst stark streuen (0,00531281 / 0,00406544 / 0,00308254). Erstmals 26 eindeutig passende Note-/Release-Tracepaare; echte DCYSUS-Feldänderungen bei 50/100 % +44/+88 (Decay), -32/-64 (Sustain), +44/+88 (Release). Registeranbindung und Richtung belegt, ausreichende musikalische Wirkung nicht. Validator 47/47, übrige Vorprüfungen und Editor PASS.
- Hypothese: lange Ausklänge vorheriger Program-17-Noten überlagern nachfolgende Messungen, CC120 war nicht als tatsächliche Stille validiert. Nur Program-17-Fixture verkürzt nominellen Amp-Release 16 auf 4 s, bewiesene Filterbasis/Note unverändert. Vor allen neun Program-17-Trials nun drei aufeinanderfolgende 50-ms-PCM-RMS-Fenster <=0,00025 zwingend, tatsächliches Ausklingen bis max. 5 s abwarten, kein Host-Muting. Referenz-Gültigkeits- und alle ursprünglichen PCM-Schwellen unverändert. Zusätzliche amplitudennormierte Spektraldiagnostik, nicht als Ersatz-PASS. Fallbezogene DCYSUS-Richtung/Abstufung künftig zwingendes zusätzliches Gate; 26 eindeutige Paare erforderlich. Nur isolierter Overlay-Host erhält längeren Watchdog, kein Produkt-/DSP-/GUI-Eingriff. Neuer Lauf offen, keine Freigabe.

## 25.11.2026 — Filter-Referenz zu leise; vollständige Diagnose ohne Produktänderung

- [Windows 45504615247](../../actions/runs/45504615247) FAIL im neuen Referenz-Gate `Filter probe amplitude masks modulation sustain or release`; kein neuer Einzelreglerbefund, da die Prüfung vor den sechs nichtneutralen Filter-Trials abbrach. 47/47 Validator, übrige vorherige Gates und Editor PASS. Trace: 19 gehaltene DCYSUS-Writes, 19 Release-Writes, 19 nichtneutrale PEFE-Writes; jeweils zwei verschiedene Werte-/Routenvarianten. Das belegt zwei verwendete Presets, nicht die Wirkung der drei fehlenden Regler. Transport/Version SUCCESS.
- Nur dedizierte zweite Test-Fixture angepasst: Program 17 Grundcutoff 7800 Cent, Filtermodulation 3600 Cent, Amp-Release nominal 16 s, MIDI 48 statt 60 (Grundton unterhalb des Cutoffs, dennoch obertonreich). Absicht: stärkere messbare Filterbewegung bei geringer Amplitudenmaskierung; noch nicht empirisch belegt. Vorhandenes Program 0 und alle Produktparameter unverändert. Ausgabe der tatsächlichen Peak-/Sustain-/Release-RMS-Werte. Alle sechs Filter-Trials werden auch bei ungültiger Referenz durchgeführt, aber Referenz- und PCM-Gate bleiben zwingend FAIL. DCYSUS-Trace zusätzlich fallbezogene Feld-Deltas, nur bei exakt 26 eindeutig geordneten Note-/Release-Paaren, gleicher Stimme und identischer Neutralreferenz; sonst ausdrücklich keine Zuordnung. Keine Schwellenänderung, kein Eingriff in Original-Synthesekern, kein Release. Neuer Windows-Nachweis ausstehend.

## 24.11.2026 — Echter Regler-PCM-Test 7/10; gezielter Filter-Gegenversuch

- [Windows 45502431315](../../actions/runs/45502431315) FAIL im neuen Regler-Gate, nicht beim Build. Steinberg 47/47, P165-Lifecycle und vorangehende Gates erfolgreich. PCM-Regler 1100–1104, 1108, 1109 PASS; Neutralrestore PASS. 1105 Filter Decay Mitte/Max Delta 0,000171616/0,000378153 bei Schwelle 0,000723318; 1106 Filter Sustain 0,000141145/0,000255719 bei 0,000199637; 1107 Filter Release 0,00007679/0,000127777 bei 0,000199637. Noch kein belastbarer Nachweis, dass die drei Regler ausreichend wirken oder dass die schwachen Änderungen allein am Testklang liegen. Transport 45502431397 und Version 45502431454 SUCCESS.
- Gegenversuch ohne Produkt-/Regleränderung und ohne Schwellenabsenkung: bisheriges Program 0 für sieben bestandene Fälle exakt beibehalten; Program 17 erhält eigenständige stärker ausgeprägte Filterhüllkurve (4800 Cent Modulation, Grundcutoff 6600 Cent, 1 s nominale Filter-Decay/Release, Amp-Sustain ungedämpft, 4 s nominales Amp-Release). Zwei eigene Neutralreferenzen und Rückkehr auf null für drei Filterregler; Mindest-PCM bei spätem Sustain und nach NoteOff als Gültigkeitskontrolle. Gleiche 1%-/2x-Wiederholungsabweichungs-Schwellen, echte Automation. Zusätzliche temporäre DCYSUS-/PEFE-Trace-Aggregatdiagnose auch bei PCM-FAIL, keine Rohdaten veröffentlicht; keine per-Regler-Registerursache allein aus diesen Aggregaten ableiten. Generatorstruktur vor Windows prüfen. Ein gebündelter Lauf, kein teurer Sweep bei frühem Filter-FAIL. Noch kein neuer PASS, keine Freigabe.

## 24.11.2026 — Windows-Gesamtlauf PASS; gebündelter echter Regler-PCM-Test ergänzt

- [Windows 45494869093](../../actions/runs/45494869093) auf e2babc20 vollständig SUCCESS: portable Rohregistermathematik 65.536 Wörter, VST3-v5-Recall aller zehn Parameter inkl. v1–v4-Neutralrestore, Steinberg 47/47, alle vier RAM-Spektralgates, Cheezoids 128/128, Master/Sustain und P165-Editor PASS. Alle drei Performance-Raten 44,1/48/96 kHz wieder ohne Audio-/MIDI-Drops und Unterläufe (9254/28522/14269 Blöcke), p95 13,5601/13,6667/12,9583 ms. Transport 45494869096 und Version 45494868984 ebenfalls SUCCESS. Dies belegt Neutralverhalten und Recall, nicht individuelle Regler-Klangwirkung.
- Neue separate `--overlay`-SF2-Fixture mit eigener obertonreicher PCM-Welle, nichttrivialen AMP- und Filterhüllkurven, echter Modulationsroute. Bisherige trockene/nasse Fixture unverändert. Ein nativer Hostlauf prüft alle zehn Regler einzeln bei 50/100 %, über echte VST3-Automation, mit zwei Neutralreferenzen und Rückkehr auf null. Phasenunabhängige RMS- und Differenz-RMS-Zeitfenster, Release-Regler nur im Release-Fenster, Differenz muss über Wiederholungsabweichung und 1 % Referenzpeak liegen. Alle zehn müssen nachweislich PCM verändern; Neutralrestore separat. Original-Registertrace vorhanden und temporär, aber noch kein automatisierter per-Regler-Registervergleich. Neuer Klangtest liegt vor teurem Cheezoids-Sweep, ein Windows-Build, kein Produktcode geändert. **Neuer Test noch nicht ausgeführt, keine Regler-Klangabnahme, kein Release.**

## 24.11.2026 — Windows-Compilerfehler der Regleranbindung korrigiert

- Windows 45493826921 FAIL beim DLL-Build, vor Klang-/Reglerprüfung: erzeugter MPU-/EMU-Code enthielt literale Escape-Sequenzen statt Zeilenumbrüchen; gemeinsamer Header verwendete C++17-Namespace-Syntax im älteren DOSBox-MSVC-Projekt sowie min/max-Aufrufe, die mit Windows-Makros kollidieren. Portable Vorprüfung und Transport 45493826793, Version 45493826836 bestanden; Editor-Folgefehler mangels gebauter Binärdatei kein GUI-Funktionsbefund.
- Korrigiert ausschließlich die drei belegten Compilerursachen: korrekte Python-Newline-Escapes, C++14-kompatible Namespaces, makrosichere (std::min)/(std::max). Neue Patch-Assertions prüfen erzeugte echte Zeilenumbrüche und entfernen #pragma once aus eingebettetem Header; portable Mathematik jetzt unter C++14 statt C++17. Keine Änderung der musikalischen Reglerlogik oder Testschwellen. Neuer Windows-Nachweis offen.

## 24.11.2026 — P165-Regler gemeinsam an Original-Registerpfad angebunden; Nachweis offen

- Gemeinsame Implementierung aller zehn AMP-/FILTER-/Cutoff-/Resonanzregler: Host-Parameter, Automation, neue v5-State-Variante (v1–v4 bleiben lesbar und bei Neutralstellung schreibbar), Original-P165-Regler und vorhandener MOD RESET (setzt alle zwölf Modulationsregler zurück). Keine Änderung an der Chip-Synthese oder DEMO.EXE. Guest-Thread nutzt die portierte gemeinsame Mathematik; Kontextmarkierung an tatsächlich aus der MPU-FIFO gelesenen vollständigen Host-Nachrichten, Ende am nächsten Statuspoll. Boot/RAM-Upload ohne musikalischen Kontext; reservierte Voices und DMA ausgeschlossen. Filterpaar IFATN/PEFE in beiden Schreibreihenfolgen zusammenführen. Wirkung primär bei neuen Noten, Release bei NoteOff/entsprechenden CCs, wie beim alten Overlay. Keine Aussage über musikalische Vollständigkeit, bis Register- und PCM-Gegenproben vorliegen.
- Vor Windows: portable C++-Rohregistermathematik und Patch-Anker gegen tatsächlich gepinnten Upstream zwingend prüfen; Windows-Gate hängt nun von deren Erfolg ab. Neuer echter Komponenten-v5-Recall-/Altzustands-Neutraltest. Performance-Gate vor langem Cheezoids-Sweep; Null-Unterläufe unverändert verlangt. Separater Validator-Diagnoseworkflow nur manuell, DLL-Cache berücksichtigt gemeinsamen Mathematikheader. Ein gebündelter Windows-Lauf. **Noch kein Windows-Kompilier-, Regler-PCM-, GUI-Reset- oder Release-Nachweis.**

## 24.11.2026 — Gemeinsame P165-Rohregistermathematik lokal geprüft

- Vorläufige gemeinsame Implementierung unter `research/p165-control-port/VoiceOverlay.hpp`, unabhängiger `VoiceOverlayTest.cpp`. Portiert alte P175-Logik auf rohe DOSBox-Registerwörter mit Bitmasken für Trigger/Engine/Release/DMA/Adresse und Filterroute. Lokaler Linux C++17-Test tatsächlich ausgeführt, `-Wall -Wextra -Werror -O2`, alle 65.536 Registerwörter auf neutrale Identität, fünf nichtneutrale Werte auf Masken und Richtung, Endpunkte, monotone Rate/Skalierung, alle 256 dunklen Cutoff-Basen geprüft. Ergebnis `P165_RAW_REGISTER_MATH_PASS`. Kein DOSBox-/VST3-Klangtest, kein Nachweis der sicheren Voice-Schreibgrenze. Noch nicht in Produkt eingebunden; kein neuer Windows-Gesamtlauf.

## 24.11.2026 — Sustain-PCM PASS; sieben Audio-Unterlaufframes bei 44,1 kHz

- [Windows 45487683372](../../actions/runs/45487683372) auf 97a8eaa0 insgesamt FAIL ausschließlich wegen 7 Unterlaufframes bei 44,1 kHz (9254 gemessene Blöcke, 0 Audio- und MIDI-Drops, Queue-Minimum 121 bei Blockgröße 128, Host-Wakeup-Verzögerung bis 7,62096 ms, keine Callback-Fristüberschreitung). Kein Nachweis für die genaue Ursache oder den Zeitpunkt des Unterlaufs; Host-Jitter ist eine mögliche Erklärung, nicht bewiesen. Striktes Null-Unterlauf-Gate bleibt unverändert.
- Neuer echter Sustain-Test PASS: RMS ohne Note 0,000152588, gehalten 0,0275908, NoteOff ohne Pedal 0,00012207, NoteOff mit Pedal 0,0274312, nach Pedal-Off 0,000107896. Steinberg 47/47, Cheezoids 128/128, Master-Recall, P165-Lifecycle PASS. p95 13,5601/14,1875/12,9792 ms bei 44,1/48/96 kHz; 48 und 96 kHz weiterhin 0 Unterläufe und Drops. Transport 45487683433 und Version 45487683504 SUCCESS.
- Kein neuer teurer Gesamtlauf nur wegen dieses einzelnen Unterlaufs; mit dem gebündelten Reglerport gezielte 44,1-kHz-Queue-Diagnose und frühe Performance-Prüfung vor dem langen Preset-Sweep vorbereiten. Kein Produktcode geändert, keine Freigabe, keine gelockerten Testgrenzen.

## 24.11.2026 — P175/P165-Reglerport: alte Implementierung als verbindliche Vorlage

- Nutzerentscheidung: penibel, effizient; alle verbliebenen Regler gemeinsam portieren und in einem gebündelten Windows-Prüflauf testen. Vorhandene P175/P165-Implementierung und Tests aus dem alten Projekt wiederverwenden, keine Rückkehr zur alten nativen Synth-Engine. Quellabgleich in `docs/P165-CONTROL-PORT.md`.
- Alter Code auf 1f9051fa ist nachgewiesen vorhanden, aber kein Nachweis für die aktuelle DOSBox-Version. Der Registerabgleich zeigt insbesondere Trigger-, Engine- und Release-Bits, die bei direkten Portwrites zwingend erhalten werden müssen. Kein blindes Übertragen der alten Masken; keine Änderung am aktuellen Produktcode und kein zusätzlicher langer Windows-Lauf während des laufenden Sustain-Gates 45487683372.

## 24.11.2026 — Steinberg 47/47 und vollständiger Original-Klanglauf PASS; Sustain-PCM-Gate

- [Windows 45479848893](../../actions/runs/45479848893) vollständig SUCCESS auf 1a009a4d; Version 45479848909 SUCCESS. Ungefilterter Original-Steinberg-Validator: 47 bestanden, 0 fehlgeschlagen. Kein Stale-Module-/Guest-Exit-/Restart-Refused-Marker im Joblog. Original-Creative-Archivvergleich Cheezoids 128/128, alle PCM-/Effekt-/Recall-Gates und nativer P165-Editor erneut PASS. p95 bei 44,1/48/96 kHz 13,288/14,9583/13,2812 ms, 7296/28522/14269 Queue-Blöcke mit jeweils 0 Audio-/MIDI-Drops und 0 Unterläufen. Master-RMS 100/50/0/Recall 0,0275934/0,013732/0/0,0137307.
- Nächster echter musikalischer Controller-Nachweis im 44,1-kHz-RAM-Test: Original-Steinberg `ParameterChanges` für gemapptes CC64, reale NoteOn/Off, unabhängige PCM-Fenster. Negativkontrolle ohne Pedal muss abklingen; mit Pedal muss nach NoteOff weiterhin mindestens halber RMS gehalten werden, Pedal-Off muss abklingen. Kein bloßer Parameter-Mapping-PASS. Test neu implementiert, Windows-Nachweis offen; keine Produkt-/DSP-/GUI-Änderung. AMP-/FILTER-ADSR und Cutoff/Resonance bleiben offen (Original-DOS-SF2LIB unterstützt NRPN nicht), Mehrfachinstanzen und Installer-/Besitzer-DAW-Abnahme ebenfalls offen. Keine Releasefreigabe.

## 24.11.2026 — Verbliebene DOSBox-DLL bei sofortiger Deaktivierung; frühe Validator-Pflichtprüfung

- [Kurzdiagnose 45477175960](../../actions/runs/45477175960) FAIL: `AWE64_RUNTIME_MODULE_STILL_MAPPED_AFTER_STOP` unmittelbar vor erneutem `DOSBoxMenu::alloc_item() name '_separator_' already taken` in Valid State Transition 32bits. Damit ist der verbliebene Modulzustand bestätigt; genaue Ursache der zusätzlichen Loader-Referenz weiterhin offen. Die vorige Abbruchgrenzen-Korrektur allein reicht nicht.
- VST3-Host wartet bei vorzeitiger Deaktivierung außerhalb RT maximal drei Sekunden auf den tatsächlichen UART-Ready-Zustand oder Guest-Ende, bevor er den regulären Shutdown anfordert. Bereits bereite Gäste ohne Zusatzwartezeit. Fehlercode des Guest-Endes protokolliert; Start eines nachweislich noch gemappten DLL-Pfads wird ausdrücklich abgelehnt, statt die globalen DOSBox-Strukturen erneut zu initialisieren. Keine erzwungene Entladung und keine Änderung an EMU8000/P165. Windows-Wirkung noch nicht nachgewiesen.
- Vollständiger unveränderter Steinberg-SDK-Test wird jetzt vor der zeitaufwendigen Original-Archivprüfung ausgeführt, Live-Protokoll statt Ausgabe erst nach Abschluss. Kein Pflichtgate entfernt oder abgeschwächt; erst bei Validator-PASS folgt wieder der vollständige Creative-Klangnachweis. Release weiter NO-GO.

## 24.11.2026 — Validator nach Stop-Korrektur erneut Timeout; Kurzdiagnose statt Gesamtwiederholung

- [Windows 45470071317](../../actions/runs/45470071317) FAIL auf ad4b2905 ausschließlich wegen erneutem 8-Minuten-Validator-Timeout. Neu kompilierte DLL, Stop/Join, MPU/PCM, Negativkontrolle, vollständige Creative-Archivprüfung und P165-Lifecycle jeweils SUCCESS. Transport 45470071331 und Version 45470071262 SUCCESS. Die Korrektur hat den Gesamt-Validator noch nicht nachweislich repariert; das gepufferte Protokoll zeigt nicht, ob derselbe oder ein anderer Fehler vorliegt.
- Nur den isolierten Validator-Diagnoseworkflow erneut auslösen; er protokolliert den aktuellen Commit und die tatsächlichen Validator-Ausgaben live. Keine erneute Originalklangprüfung, keine Produktänderung auf Verdacht, kein Release. Der vollständige Validator bleibt Pflicht.

## 24.11.2026 — Validator-Reaktivierung: DOSBox-Abbruchgrenze korrigiert

- [Diagnose 45467327483](../../actions/runs/45467327483) FAIL im ersten `Valid State Transition 32bits`: Parameter-/MIDI-/Unit-Prüfungen bestanden, beim zweiten DOSBox-Start `DOSBoxMenu::alloc_item() name '_separator_' already taken`, danach Timeout. Der gepinnte Steinberg-Test deaktiviert unmittelbar nach setActive(true), wiederholt viermal und terminiert/initialisiert neu.
- Konkrete Quellabweichung: unser Stop-Hook war auch während DOSBox-Initialisierung und -Abbau in GFX_Events aktiv, außerhalb des originalen DOSBOX_RunMachine/kill-switch try/catch. Abbruch wird jetzt bis zur originalen sicheren Abbruchgrenze vorgemerkt, dort einschließlich vorzeitiger Stop-Anforderung verarbeitet; RAII deaktiviert den Hook während Abbau. Kein Reset der Stop-Anforderung beim Eintritt. Zusätzlich Diagnose für nach FreeLibrary noch gemappte DLL. Keine Veränderung am EMU8000-Kern oder der P165-Grafik. Ursache des verbliebenen Modulzustands muss im Windows-Lauf bestätigt werden, kein Validator-PASS. Patch-Regression aktualisiert.

## 24.11.2026 — Steinberg-Validator: Timeout, isolierter Diagnose-Workflow

- [Windows 45459861521](../../actions/runs/45459861521) FAIL ausschließlich wegen 8-Minuten-Timeout im offiziellen Validator-Schritt. Windows-Build, synthetische Positiv-/Negativkontrollen, vollständige Original-Archivtests und P165-GUI-Lifecycle erneut PASS. Der Validator selbst wurde erfolgreich kompiliert; kein Testresultat und keine belegte Absturzursache. Version 45459861708 SUCCESS.
- Separater schneller Diagnose-Workflow baut denselben gepinnten SDK-Validator und dasselbe VST3 mit synthetischem DOS-Gast, ohne erneute 20-minütige Original-Archivprüfung. Ungefilterter Validator mit fortlaufender statt erst am Ende ausgegebener PowerShell-Protokollierung und separatem 5-Minuten-Timeout; kein künstlicher PASS. Nur dieser neue Workflow wird durch seine eigene Datei ausgelöst. Vollständiger bestehender Integrations- und Validator-Pflichtlauf bleibt unverändert und ist vor Freigabe nötig. Keine Veröffentlichung von Artefakten oder Creative-Daten, keine Produkt-/DSP-/GUI-Änderung. Diagnoseergebnis offen.

## 24.11.2026 — Validator-Workflow: YAML-Einfügungsfehler korrigiert

- [Windows 45458638773](../../actions/runs/45458638773) FAIL ohne Jobs: Fehler beim Einfügen des neuen Validator-Schritts verdoppelte den YAML-Dateischluss an der Regex-Endmarke. Kein Validator ausgeführt. Transport 45458642821 und Version 45458642659 SUCCESS.
- Workflow exakt aus dem letzten erfolgreichen b6975b8a rekonstruiert, ausschließlich um denselben Validator-Schritt ergänzt; unveränderte übrige Schritte und einmaliger Upload-Block geprüft. Keine Produkt-/GUI-/DSP-Änderungen. Neuer Windows-Nachweis offen.

## 24.11.2026 — P165-GUI-Lifecycle PASS; vollständiger Steinberg-Validator als Pflichtgate

- [Windows 45453837727](../../actions/runs/45453837727) SUCCESS auf b6975b8a; Transport 45453837608 und Version 45453837600 SUCCESS. Tatsächlicher nativer Editor zweimal erstellt/angehängt/entfernt, jeweils anfänglich und nach Attach 1000x708, Win32-Client ebenfalls 1000x708. Das fehlende IPlugFrame war eine Abweichung des Testhosts; P165-Produktgrafik unverändert.
- Original-Archivtest erneut PASS einschließlich Cheezoids 128/128, Master-PCM-Recall und 60-Sekunden-Polyphonie. Latenz p95 bei 44,1/48/96 kHz 14,4444/12,9167/15,6146 ms, 7296/28522/14269 Queue-Blöcke ohne Audio-/MIDI-Drops oder Unterläufe.
- Neuer Pflichtschritt: kompletter originaler Steinberg-Validator aus dem gepinnten SDK 3.7.12, ohne Suite-Filter, auf der tatsächlichen VST3-Binärdatei mit internem DOSBox-DLL und synthetischem Gast. Exit 0 und explizite Zusammenfassung mit >0 bestandenen und 0 fehlgeschlagenen Tests erforderlich; keine automatische Freigabe aus Build allein. Original-Archivtests und GUI-Lifecycle unverändert. Validator-Ergebnis offen; kein Nachweis der deaktivierten AMP-/FILTER-Regler, keine Installer-/DAW-Abnahme, kein Release.

## 24.11.2026 — OLE-/Klangprüfung PASS; fehlendes IPlugFrame im GUI-Testhost ergänzt

- [Windows 45446318739](../../actions/runs/45446318739) insgesamt FAIL ausschließlich beim nativen Editor-Größengate: OLE/InitDll, createView und attached erfolgreich; Größe nicht 1000x708. Sämtliche vorhergehenden Windows-Original-Archiv-, PCM-, Performance-, Stabilitäts- und Negativkontrollschritte SUCCESS. Transport 45446318910 und Version 45446318523 SUCCESS. OLE-Korrektur behebt den früheren Abbruch, aber GUI-Abnahme noch nicht vollständig.
- Der native Testhost übergab bisher kein IPlugFrame. VSTGUI 4.x `VST3Editor::requestResize` benötigt dieses Interface zur Host-Größenanforderung. Jetzt echtes IPlugFrame mit Win32-Clientgrößenanpassung und onSize-Bestätigung; initiale und tatsächliche Größen, Rückgabecode und Host-Resize-Anforderung werden ausgegeben. Größen-Sollwert 1000x708 unverändert; keine erzwungene Rückgabe, keine Grafik-/Produkt-/DSP-Änderung. Neuer Windows-Nachweis offen; kein Release.

## 24.11.2026 — Nativer GUI-Test: fehlender Host-OLE-Start und getrennte Diagnose

- [Windows 45442319908](../../actions/runs/45442319908) FAIL: Windows-Kompilierung und DLL-/MPU-/Hardware-Smokes bestanden; neuer gemeinsamer GUI-/Audio-Hosttest brach ohne native Diagnose vor PCM ab. Original-Archivtest wurde deshalb nicht ausgeführt. Transport 45442319845 und Version 45442320269 SUCCESS. Bisherige Klang-/Stabilitätsnachweise bleiben gültig.
- Steinbergs gepinnter Windows-Hosting-Code `source/vst/hosting/module_win32.cpp` initialisiert OLE vor dem Laden des Moduls. Der eigene Testhost hatte diesen Schritt ausgelassen; ergänzt samt RAII und Link gegen ole32. Dies ist eine konkrete Abweichung des Testhosts, die Absturzursache ist noch nicht abschließend bewiesen.
- GUI-Gate in eigenem Prozess `--editor` nach den unveränderten Klangtests, mit etappenweisen Diagnosen und Exitcode. GUI bleibt verpflichtend und ein Fehlschlag weiterhin rot; keine Unterdrückung des Gates. Keine Produkt-/DSP-/Grafikänderung. Neuer Windows-Nachweis offen.

## 24.11.2026 — 60-Sekunden-Polyphonie und Master-PCM-Recall PASS; nativer GUI-Lifecycle-Test

- [Windows 45434254976](../../actions/runs/45434254976) SUCCESS auf b60b5b22; [Transport 45434255359](../../actions/runs/45434255359) und [Version 45434255242](../../actions/runs/45434255242) SUCCESS. Auch 45430630922 abgeschlossen SUCCESS. 48 kHz 60 Sekunden/32 MIDI-Noten: 28522 Queue-Blöcke ohne Unterlauf, Audio-/MIDI-Drop, stumme gehaltene Blöcke oder Callback-Fristüberschreitung; Release-Peak 0,000152588. Master-RMS 100/50/0/Recall: 0,0275808/0,013734/0/0,0137242. Latenz p95 12,3583/14,6042/13,6458 ms bei 44,1/48/96 kHz; alle 128 Cheezoids und RAM-Spektralgates erneut PASS.
- Neu: nativer Host ruft die laut Steinberg-Windows-SDK vorgeschriebenen InitDll/ExitDll-Exporte auf. Im synthetischen Test öffnet und entfernt er zweimal den echten VSTGUI-Editor an einem realen Win32-HWND, verlangt exakt 1000x708. Keine Änderung an Produktcode oder P165-Grafik. Noch kein Windows-Ergebnis für GUI-Lifecycle und kein Nachweis aller Bedienelemente: AMP-/FILTER-ADSR, Cutoff/Resonance und MOD RESET weiterhin deaktiviert; vollständige Funktionsabnahme und Installer offen. Kein Release.

## 24.11.2026 — Master-Volume: echter PCM-/Projekt-Recall-Test ergänzt

- Windows 45430630922 zum Änderungszeitpunkt noch im Original-Archivtest; Transport 45430630997 und Version 45430631151 bereits SUCCESS. Kein vorweggenommener 60-Sekunden-PASS.
- Neue unabhängige Master-Prüfung im 44,1-kHz-VST3-Host nach den unveränderten CPU-/Latenzmessungen: gehaltene originale RAM-Stimme bei 100 %, 50 % (halbe PCM-Amplitude), 0 % (exakt stumm), anschließend echter IComponent-State-Recall auf 50 % und erneute PCM-Messung. NoteOn/Off-Zähler werden mitgezählt. Keine Änderung an Produktcode, Originalkern oder P165-Grafik. Windows-Nachweis offen; kein GUI-Klick- oder Release-Nachweis.

## 24.11.2026 — 96-kHz-Abnahme PASS; erweiterter 48-kHz-Stabilitätstest

- [Windows 45425096309](../../actions/runs/45425096309) SUCCESS auf 2d711cac; tatsächlicher RAM-735-Hz-Klang bei 96 kHz nachgewiesen. [Transport 45425096302](../../actions/runs/45425096302) und [Version 45425096276](../../actions/runs/45425096276) SUCCESS. Alle 128 Cheezoids-Programme und bisherigen Klanggates erneut bestanden.
- Native 128-Sample-Latenz p95/max bei 44,1/48/96 kHz: 12,9478/13,875/13,9479 ms; 6569/7147/14269 gemessene Queue-Blöcke, jeweils null Unterläufe, Audio-/MIDI-Drops, stumme gehaltene Blöcke und Callback-Fristüberschreitungen. 32-MIDI-Noten-Last 1,24976/2,50002/5,31309 % eines logischen CI-Kerns, nicht Besitzer-CPU.
- Nächster Prüfpunkt: 48-kHz-Polyphonietest von 5 auf 60 Sekunden mit 32 gehaltenen MIDI-Noten verlängert; nach NoteOff zwei Sekunden Release und abschließender Peak <0,002 verlangt. Vorhandene Queue-, Stille- und Latenzgates bleiben unverändert. Timeout für diesen Einzeltest erhöht, 44,1/96-kHz-Proben unverändert. Keine Produkt-/DSP-Änderung, keine erneute Optimierung auf Verdacht. Neuer Windows-Nachweis offen; kein Release.

## 24.11.2026 — 96-kHz-End-to-End-Abnahme erweitert (Windows-Ergebnis offen)

- Nativer VST3-Hosttest akzeptiert nun 96 kHz zusätzlich zu 44,1/48 kHz. Die vorhandene RAM-735-Hz-Spektralprüfung und die unveränderte 128-Sample-Performance-/Queue-Prüfung werden auch bei 96 kHz ausgeführt. Keine Änderung an Produktcode, EMU8000, P165-GUI, Puffern oder bisherigen Grenzwerten.
- Bisherige 44,1/48-kHz-PASS-Ergebnisse bleiben erhalten; der neue Windows-Prüflauf muss tatsächlichen 96-kHz-Klang und Transport bestätigen. Keine Gesamt- oder Releasefreigabe.

## 24.11.2026 — Guest-Tick-PCM: Windows-Klang und Performance vollständig PASS im bisherigen Testumfang

- [Windows 45415014969](../../actions/runs/45415014969) SUCCESS auf 983c96ce, beide Jobs und alle ausgeführten Schritte bestanden; [Transport 45415014916](../../actions/runs/45415014916) und [Version 45415014891](../../actions/runs/45415014891) SUCCESS.
- Native 128-Sample-Messung bei 44,1/48 kHz: MIDI→PCM Median 12,8571/13,9792 ms, p95 und Max 13,6054/14,5625 ms. Vorher p95 96,08/97,31 ms. Keine Queue-Unterläufe, Audio-/MIDI-Drops, stummen gehaltenen Blöcke oder Callback-Fristüberschreitungen in 6569/7147 gemessenen Blöcken. CPU-Prozess 3,46047/2,20306 % eines logischen CI-Kerns, 32-MIDI-Noten-Last 2,49988/2,50003 %. Keine Behauptung einer CPU-Messung auf Besitzer-i9 oder einer DAW-Abnahme.
- Exakt hashidentische Cheezoids-Bank erneut 128/128 Originalvergleich PASS, kein verlorenes Sustain oder >20-dB-Pegelfensterabweichung. Alle drei RAM-Frequenzfälle, absoluten Send-/Recall-Register-Gates und Effekttypen erneut PASS. Keine Änderung des EMU8000-DSP.
- Nutzer hat 20 ms ausdrücklich als Optimierungsziel statt starrer Produkt-Abnahmegrenze bestätigt; selbst höhere Latenz ist bei technisch unvermeidbarem Bedarf akzeptabel. Bestehendes 20-ms-Regressionstestziel bleibt unverändert, jetzt erreicht. Priorität: Originalklang, störungsfreie Wiedergabe, dann Performance. Keine weitere riskante Optimierung ohne Befund.
- Noch keine Releasefreigabe: vollständige GUI-/Parameterfunktion, längere Stabilitäts-/Timingtests, Installer und vollständige VST3-Validierung, abschließender Studio-Pro-Praxistest bleiben separat zu verifizieren. Historische offene Befunde unten gelten nur bis zum nachgewiesenen Folgetest.

## 24.11.2026 — Guest-Tick-PCM statt unabhängiger SDL-Dummy-Uhr

- Windows 45402226002: 44,1/48 kHz p95 96,08/97,31 ms, 23040/23135 Unterlauf-Frames, keine Audio-/MIDI-Drops oder Callback-Fristüberschreitungen. CPU 1,81/2,36 % eines CI-Kerns. Exakte Cheezoids-Revision (128 Programme), RAM-Frequenzen, alle drei absoluten Sends/Recall und Effekttypen erneut PASS. Transport/Version PASS.
- Neue ABI-Option nur für authentischen VST3-Start: SDL-Dummy-Gerät bleibt geöffnet, aber pausiert. Derselbe originale PCM-Konvertierungscallback leert im vorhandenen Gast-Tick die fertig gerenderten Frames. Interne SDL-Vorpufferung entfällt; einmalige Host-Vorfüllung (384 native Frames bzw. größerer Block +128). Kein Eingriff in Chip-DSP oder Creative-Treiber. Direkter Referenzpfad unverändert.
- Implementierung noch nicht auf Windows verifiziert. Alle bisherigen Klang- und Performance-Grenzen bleiben verbindlich. Kein Release.

## 24.11.2026 — Windows-Performance-Befund und CRLF-Auswertung

- Windows 45314956767 FAIL: 44,1/48 kHz p95 95,7823/86,0625 ms, Unterlauf 23168/23252 Frames, keine Audio-/MIDI-Drops, keine Callback-Fristüberschreitungen. Queue-Mittel 560,30/562,56 Frames; CPU 2,28/3,46 % eines CI-Kerns. Alle drei absoluten Send-State-/Register-Gates und Cheezoids-128-Vergleich PASS. Transport 45314956648 und Version 45314956701 SUCCESS.
- Python-Wrapper wertet Queue-Zeilen jetzt mit splitlines/fullmatch aus, damit Windows-CRLF nicht fälschlich als fehlende Statistik gemeldet wird. Grenzwerte unverändert. Transportoptimierung und erneute Windows-Abnahme offen; kein Release.

## 24.11.2026 — Transportmessung getrennt vom Klang-/CPU-Test

- Performance-Gate ergänzt: echte SPSC-Audio-/MIDI-Queue-Zähler, Startsnapshot erst beim ersten tatsächlichen NoteOn nach Warmup, Endsnapshot am letzten Audio-Callback vor Shutdown. Pufferfüllstand min/max/mittel vor jedem Block, maximale MIDI-Tiefe; verlorene Audioframes, verlorene MIDI-Events und Unterlauf-Frames müssen exakt 0 sein. Kein Logging im Audio-Callback, keine zusätzlichen Atomics pro erzeugtem Sample; Ausgabe nur bei explizitem Performance-Test nach Worker-Join. Damit werden auch sehr kurze Aussetzer sichtbar, die RMS-/Stille-Tests übersehen.
- Zählung der tatsächlich im nativen VST3-Test angebotenen NoteOn/Off-Events statt fest kodierter 44. Keine Änderung der Klangerzeugung, Puffergröße oder voreilige Optimierung. Neuer Snapshot-Unit-Test.
- Windows 45313059825 noch aktiv; neuer Performance-/Send-Test 45314023251 bislang eingereiht. Transport-/Versions-CI 45314023240/45314023242 SUCCESS. Noch keine belegten Performance-Messwerte und kein Release.

## 24.11.2026 — Exakte Cheezoids-Revision bestanden; Performance-Abnahme

- Windows 45310753140: Autorbank SHA-256 exakt identisch zur hochgeladenen 652206-Byte-Datei. Alle 128 Programme im echten VST3 gegen direkten originalen Creative-DOSBox-Pfad gemessen. Original und VST3 kurze Abklingverläufe identisch [10,22,28,47,49,73,75,77], verlorenes Sustain [], Pegelfensterabweichungen >20 dB []. `CHEEZ_ALL_128_ENVELOPE_COMPARISON_PASS`. Der frühere Mehrheits-Bleep reproduziert sich in diesem Test nicht. Keine Behauptung bitidentischer Hardware oder subjektiver Klangabnahme.
- RAM-Frequenzgates 441/735 Hz (44,1/48 kHz) und Effekt-Register erneut PASS. Drei absolute Send-Hosttests liefen nicht an: SDL_AUDIODRIVER fehlte im neuen Schleifen-Setup; korrigiert, früher Hostfehler vor Bootlog wird nun ausgegeben. Register-Gates bleiben unverändert verbindlich.
- Nutzerziele: zusätzliche tatsächlich wahrgenommene Plugin-Latenz höchstens 20 ms, CPU in Studio Pro auf i9-11900K höchstens 10 %, stabile Polyphonie, Originalklang unverändert. Erst messen, dann bei Bedarf optimieren. Nativer Benchmark ohne Trace-I/O: 128 Samples bei 44,1/48 kHz, 12 wiederholte NoteOn-PCM-Onsets (Median/p95/Max, p95-Gate 20 ms), Prozess-CPU aus Windows GetProcessTimes inklusive Emulatorworker als Ein-Kern-Äquivalent und Anteil aller logischen CPUs, Callback-Fristüberschreitungen und Host-Wakeup-Jitter getrennt, fünf Sekunden 32 verschiedene gehaltene MIDI-Noten als Lasttest (keine unbelegte 32-Stimmen-Zusage), stumme Blöcke während gehaltenem Ton als Aussetzerindikator. CPU des CI-Rechners ist NICHT CPU des i9-11900K oder Studio-Pro-Performanceanzeige. Test neu implementiert, Windows-Messwerte noch offen. Kein Eingriff in EMU8000-Kern.

## 24.11.2026 — Exakt erneut hochgeladene Cheezoids-Problemrevision festgeschrieben

- Besitzerdatei `Cheezoids.sf2`, 652206 Bytes, SHA-256 `5adad4b3b95a33ef64ae06dd1627038140435b395d183aab622efcb4c920359b`. SF2 2.01, alle 128 Bank-0-Programme, 160 Samples, 1275 Instrumentzonen mit SampleMode 1 (kontinuierlicher Loop). Originalbytes nicht verändern/hochladen. Die öffentliche Autorenbank ist **nur dann** ein Ersatz für diesen konkreten Test, wenn ihr SHA-256 exakt übereinstimmt.
- Tatsächlich lokal gerenderte unabhängige Referenz via libfluidsynth.so.3: alle 128 Programme bei MIDI 60/Velocity 100, 44,1 kHz, 1,75 Sekunden gehalten, Reverb/Chorus aus. Frühfenster 0,2–0,6 s, spät 1,1–1,6 s. 114 Programme mit spätem RMS >=0,0005 und mindestens 10 % des frühen RMS; 11 kurze Abklingverläufe (6,10,20,22,28,45,47,49,52,73,75), 3 sehr leise (23,53,77). Damit ist die Mehrheit kurzer Bleeps **nicht** in dieser Datei bei allgemeiner SF2-Wiedergabe inhärent. FluidSynth ist keine AWE-Klangreferenz; Original-Creative-DOS und reales VST3 bleiben gesondert zu prüfen. Warnungen für Instrumente PNO Bass/PIANO (ungültige Generatoren verworfen).
- Neuer exakter Hash-Gate für Autorendownload und private lokale Datei `--owner-bank` bzw. `-Cheezoids`; andersartige öffentliche Revision darf die konkrete Benutzerregression nicht bestehen. Originale Benutzerdatei nicht im Repo, keine Weitergabe. Noch kein tatsächlicher Windows-VST3-Klangnachweis dieser exakten Revision.

## 24.11.2026 — Effektumschaltung im echten VST3 bestanden

- Windows 45309812893: alle drei RAM-Spektralfälle bestanden, unveränderter Originalgast Effekt-Register 0/0, korrigierter Originalgast 1/1, tatsächlicher VST3 `EFFECT_TRACE_PLATE=True; EFFECT_TRACE_FEEDBACK=True`. Damit sind beide getesteten originalen Effekttypen im VST3 nachgewiesen. Der ältere Lauf scheiterte noch an den unveränderten CC-Sends, nicht an der Effekttyp-Umschaltung.
- Windows 45310753140: neuer Cheezoids-/Absolute-Send-Lauf noch aktiv. Linux-Quellpatchtest scheiterte ausschließlich an einer unvollständigen synthetischen EMU8000-Testvorlage (fehlende neue Zeitbasis-/Write-/Mix-Anker). Vorlage ergänzt, Send-Export und Registerwrite-Aufruf explizit geprüft, keine Änderung des Produkt-DSP. Versions- und Transportregressionen 45310753256/45310753337 bereits PASS. Keine Freigabe.

## 24.11.2026 — Cheezoids: gezielte Regression aller 128 Programme

- Nutzer meldet aus früherem Stand mehrheitlich falsche kurze Bleep-Klänge. Die konkrete private Datei war in der Bibliotheksliste nicht vorhanden. Öffentlich freigegebene Originalbank von Analoguesque Sound Designs (Ian Wilson) im Autor-Repository `Analoguesque/freepatches`, Commit `681907983ec024c1994f6a02615d650877913696`, Archiv-Git-Blob `c12461b3e016f70b36e39eca2f78eac5685a0fbf`. Nur temporär herunterladen, keine Bankdaten, PCM oder Traces hochladen. Identität zur früheren privaten Kopie nicht behaupten.
- Autor beschreibt kurze Single-Cycle-Wellenformen und Kompatibilitätsprobleme mancher SF2-Player. Deshalb ausdrücklich kein einzelner Peak als Klangbeweis: alle 128 tatsächlichen Bank-0-Programme, Note 60 Velocity 100, etwa 1,75 s gehalten, frühe/späte RMS-Fenster und Original-Creative-DLL-Direktvergleich mit identischen trockenen Hardware-Sends; VST3-Programmauswahl und Bankwechsel über echten Host. Meldet verlorenen Sustain, abweichende Programmpegel und kurze Originalreferenzen getrennt; Mehrheits-Bleep auch im direkten Originalpfad bleibt Fehler zur Untersuchung. Originalquelle unverändert. Noch kein ausgeführter Cheezoids-Klang-PASS.

## 24.11.2026 — Absolute Hardware-Sends statt additiver Creative-CCs

- Die bereits nachgewiesene Nicht-Null des originalen CC93 trotz Wert 0 wird nicht als Produktfunktion ausgegeben. Die P165-Sends 1110/1111 setzen jetzt ausschließlich die beiden originalen EMU8000-Registerbytes PTRX[15:8] und CSL[31:24] für Stimmen 0–29, via originaler Register-Schreibroutine im DOSBox-Gerätewrapper. Keine Änderung der gepinnten Chip-DSP-Implementierung. Der Host publiziert nur ein atomares Wertepaar; kein Chipzugriff vom DAW-Callback. Nach Gastbereitschaft wird 0/0 als Standard gesetzt, bei Live-Änderung auch bestehende Stimmen aktualisiert; Original-Referenz ohne gesetzte Host-Overrides unverändert. Reservierte Stimmen 30/31 und andere Registerfelder bleiben unangetastet.
- GUI-Kompatkopie aktiviert die beiden bestehenden Sendregler, Prozentwerte, State v4 (v1–3 weiter lesbar, Default dry). Neue Windows-Tests am echten VST3 mit eingebetteten hohen SF2-Sends: 0/0, 255/0, 0/255, jeweils echte RAM-Voice-Register und Controller-State-Recall. Original-CC-Referenz bleibt als separate Beobachtung erhalten. Native Ausführung und Wirkungsprüfung stehen noch aus.
- Quelltext-Regressionsprüfung nach der bewiesenen SysEx-Korrektur an den tatsächlich dynamisch zusammengesetzten DOS-Aufruf angepasst; vorheriger Stringvergleich war veraltet. Kein Release.

## 24.11.2026 — Original-SysEx-Korrektur tatsächlich bewiesen und in VST3 integriert

- Windows 45306739416: originale unveränderte Creative DEMO.EXE liefert beide Registersignaturen 0/0, die ausschließlich temporär um ein Byte korrigierte Kopie liefert Plate=1 und Short Delay Feedback=1. Damit ist der konkrete Vergleichsfehler experimentell bestätigt, nicht mehr nur Disassemblierungshypothese. Beide RAM-Programme, 48-kHz-Klang und Sparse-Recall weiter PASS.
- VST3-Start übernimmt nun die exakt geprüfte Korrektur ausschließlich für die bekannte 40197-Byte-Originaldatei nach vollständiger Git-Blob-SHA1-Prüfung via Windows BCrypt und Instruktionsanker. Temporäre FXDEMO.EXE im ohnehin privaten, pro Start angelegten Verzeichnis; Original bleibt byteidentisch. Kein neuer Treiber, keine Änderung des EMU8000-Kerns, keine Veröffentlichung von Creative-Binärdaten. Eigener synthetischer C++-Regressionstest. Echter VST3-Effektregister-Nachweis dieser Integration steht noch aus.
- Unveränderte Original-CC91/93-Endpunktprüfungen weiterhin FAIL. Kein Release.

## 24.11.2026 — Original-SysEx-Vergleichsfehler auf Instruktionsebene isoliert

- Korrektur des neuen Prüfskripts: Git-Blob-Header mit echtem NUL-Byte und eigener YAML-Testschritt; keine Änderung der Originaldaten oder Produktlaufzeit.
- Windows 45301207140: Byte-exakter echter 33-Byte-MPU-UART-Test PASS; alle drei RAM-Spektralfälle und Sparse-State-Negativtests PASS. Original-Direktgast empfängt beide GS-Makros, aber beide Effektregister fehlen, ebenso im VST3. CC91/93=0 liefert bei expliziten SF2-Sends weiterhin Chorus 250, 127 liefert 255; Reverb-Messung 0/0. Keine Send-Freigabe.
- Analyse der hashverifizierten originalen Creative DEMO.EXE (MZ-Datei, 40197 Bytes, dokumentierter Git-Blob): bei Datei-Offset 21834+0x35 lädt die SysEx-Musterprüfung AL aus CS:[SI], führt CBW aus und vergleicht danach AX mit nullerweitertem DX. Statusbyte F0 wird FFF0, empfangenes F0 wird 00F0, also zwangsläufig ungleich. AH ist beim Einstieg in beide Muster durch AX=0x12/0x28 bereits 0; eine lokale NOP statt CBW (Offset 21892) erhält AH=0. Keine Änderung der Effektalgorithmen, ROM-Daten oder Chipemulation.
- Nur für isolierten privaten Archiv-Referenztest: exakter Original-Git-Blob und Instruktionsanker vor der Änderung, temporäre FXDEMO.EXE mit einem Byte Unterschied, Original niemals überschreiben, kein Upload. Original unverändert als Negativkontrolle, korrigierte Kopie muss die originalen INIT-Registersignaturen zeigen. Der Produktions-VST3-Pfad bleibt bis zum Nachweis unverändert. Kein Release.

## 24.11.2026 — Original-Send-Endpunkte prüfen (0 % muss tatsächlich aus sein)

- Original ADIP DOS awe32Controller dokumentiert CC91/CC93, nicht aber eine verlässliche absolute Nullsetzung für bereits im SF2 enthaltene Sends. Vor Aktivierung der beiden P165-Sendregler wird dies im originalen Creative-Treiber nachgemessen.
- Erweitert ausschließlich selbst erzeugtes RAM-Testmaterial um explizite 1000-Promille-SF2-Sendgeneratoren (15/16), getrennt vom unveränderten trockenen Frequenztest. Zwei unabhängige Originalgast-Läufe setzen CC91/93 = 0 bzw. 127. Original-PTRX/CSL-Register an späten RAM-Voice-CCCA-Schreibzugriffen ausgewertet; 0 verlangt beide Register-Sends exakt null, 127 beide größer null. Kein approximierter Effekt und kein fiktiver Regler. Windows-Prüfung offen.
- Windows 45299958012 bestätigte erneut beide 44,1-kHz-RAM-Klänge und 48 kHz, Effektregister weiterhin FAIL. Keine Gesamtfreigabe.

## 24.11.2026 — Sparse-Preset-Recall ohne falsche Programmauswahl

- Bestehender Fehler: nach SF2-Wechsel konnte setState ein nicht vorhandenes Preset oder unbewiesene Banknummer erneut setzen. Browser konnte vor Aktivierung noch die alte Liste zeigen. Jetzt vor jedem State-Wechsel tatsächliches phdr-Preset validieren, ungültige Bank/Preset-Kombination ohne Mutation zurückweisen; auch inaktive Auswahl aktualisiert Browser-Snapshot. Kein ROM-Padding in externen SF2.
- Echtzeit-Programmautomation akzeptiert nur vorhandene Programme; kBank bleibt 0, solange höhere Banken nicht nachgewiesen sind. Controller-Readback für Bank/Programm/Master/Effekttypen direkt aus dem tatsächlichen atomaren Zustand. Native negative Tests prüfen nicht vorhandenes Programm 1 und Bank 1 in der 0/17-RAM-Bank, einschließlich unverändertem gültigen State. Windows-Nachweis noch offen.
- Original-P165-Grafik, originaler Creative-Treiber und EMU8000 unverändert.

## 24.11.2026 — SysEx-Fehler am Originalpfad isolieren

- Windows 45296517847 bestätigt die RAM-Programme 0/17 und 48-kHz-Audiowiedergabe; keine Änderung dieser bestandenen Pfade. Original-Effekt-Recall funktioniert, aber beide späten INIT-Registersignaturen fehlen.
- Originale Creative ADIP-DOS-API dokumentiert beide GS-Makros; die originale SF2LIB/RAWE32S.LIB enthält tatsächlich die SysEx-Erkennung sowie Reverb-/Chorus-Funktionen. Kein Ersatz-DSP und keine Änderung des Originaltreibers.
- Neuer bytegenauer 33-Byte-UART-Test (Bank/Programm, zwei 11-Byte-Effekt-SysEx, NoteOn/Off), größer als der 32-Byte-FIFO; prüft vollständige Übertragung einschließlich Backpressure. Zusätzlich direkter Original-DLL-Vergleich derselben Effektbefehle ohne VST3/Controller: vollständige Paketfolge und originale späte INIT1/INIT3-Signaturen. Beide Gates bleiben verbindlich, kein Freigabetrick. Windows-Ergebnis offen.

## 24.11.2026 — Windows-RAM- und Hostrate-Nachweis

- Windows 45296517847: Original-RAM-Referenz und VST3-Programme 0/17 bei 44,1 kHz sowie Programm 17 bei 48 kHz mit erwarteter Frequenz nachgewiesen; 48-kHz-Leistungsverhältnis >223. Temporäre Referenzbereinigung erfolgreich. Effektregisterprüfung weiterhin FAIL (Plate=False, Feedback=False).
- Separater Quellpatch-Test scheiterte an veralteter exakter UART-Guard-Zeichenfolge. Assertion auf neuen Guard, Bereitschaftsexport und ACK-Reihenfolge aktualisiert. Transport- und Versionsworkflows PASS. Keine Gesamtfreigabe.

## 24.11.2026 — RAM-Klang bei 44,1 kHz nachgewiesen; Initialisierungs-PCM unterdrücken

- Windows 45294125748: originale RAM-SF2 Programme 0/17 im echten VST3 mit korrekten 441/735-Hz-Signaturen (Leistungsverhältnis >114), RMS 0,0249977/0,0277133. Direkte Original-DLL-Referenz RMS 0,025432038/0,027798085; RAM-Adressschreibzugriffe vorhanden. Direkter Test scheiterte nach der Messung ausschließlich an Windows-Freigabe von RAMBOOT.LOG. Parent besitzt nun das temporäre Verzeichnis und entfernt es nach Prozessende.
- 48-kHz-Fall scheiterte an PCM-Grenzwert vor vollständigem Bankwechsel; natives 44,1-kHz-Startsignal erreicht bereits im direkten Originalmixer Vollaussteuerung. Sinc kann bei solchen Initialisierungssignalen überschwingen. Statt Limiter/Clipping oder Änderung des Chips: Produktions-PCM erst nach UART-ACK und anschließendem echten MIDI-Statuspoll aus dem unveränderten DEMO-Empfangsloop in den Ring übernehmen. Vorher Stille; Bank/Programm und MIDI erst bei Bereitschaft. Exakte Startfolge anhand DEMO.C überprüft. Neu: Block-/Sample-/Rate-Diagnose bei PCM-Grenzverletzung; Schwelle unverändert.
- Effekt-Type-State-Roundtrip PASS, erwartete Originalregister jedoch nicht nachgewiesen. Separate Bool-Diagnose; SysEx-Format im Creative ADIP bestätigt (https://www.manualslib.com/manual/2031598/Creative-Awe32.html?page=20). Keine Freigabe, Windows-Regression notwendig.

## 24.11.2026 — Eigenes RAM-Test-SoundFont korrigiert und unabhängig validiert

- Lokale unabhängige Prüfung mit libfluidsynth.so.3: Vorheriges selbst erzeugtes SF2 wurde wegen ungerader INAM-Chunklänge (21 Byte statt 22) vom strengen Parser zurückgewiesen. Fehler im Testmaterial bestätigt; keine Änderung der Originalemulation oder des Originaltreibers.
- Korrektur: NUL-Ausrichtung innerhalb der INAM-Länge; Dateigröße unverändert 15972 Byte. Korrigierte SF2 lokal erfolgreich geladen, Programm 0 und 17 jeweils eine Sekunde bei MIDI 60/100 gerendert, RMS 0,01864656 / 0,01864633; richtige Frequenz gegenüber jeweils anderer mit Leistungsverhältnis >479000. Dies prüft ausschließlich die Gültigkeit des eigenen Testmaterials, **kein** Original-Creative-Klangnachweis.
- Struktureller Regressionstest für gerade Chunklängen, SF2 2.00, zwei RAM-Sampletypen, korrekte sparse Programme. Windows-Direktvergleich und natives Original-VST3-Gate weiterhin zwingend.

## 24.11.2026 — RAM-Fehler isolieren, Original-Effektprüfung unabhängig ausführen

- Windows 45292853463: Original-ROM-GM und externer ROM-GM-Bankwechsel erneut nachgewiesen (Held-RMS 0,00845848 bzw. 0,00839579). Erste selbst erzeugte RAM-SF2 (441 Hz) dagegen **FAIL**, Held-RMS 0,0000127707, 0 aktive Notenblöcke. Keine Gleichsetzung von ROM-Bankwechsel mit RAM-Klang.
- Neuer direkter Original-DLL-Referenztest derselben selbst erzeugten SF2 für Programme 0/17, unabhängig von VST3-State und Hostrate. Aggregierte EMU8000-DRAM-Datenregister-, Eigen-PCM-Signatur- und späte RAM-Adress-Schreibdiagnose, keine Originaldaten, Roh-Traces oder Audio veröffentlichen. RAM-Fälle werden getrennt ausgewertet; Fehler blockieren die unabhängige Original-Effektprüfung nicht und verhindern weiterhin den Gesamt-PASS.
- Portable Hostrate-/SysEx-Tests und Version 45293022107/45293391865/45293391664 PASS. Windows 45293391812 noch ausstehend. Keine Klangbehauptung ohne Auswertung; unveränderte Chipemulation.

## 24.11.2026 — Original-Creative-Reverb-/Chorustyp über DOS-SysEx

- Originales ADIP v3.01, Part II, awe32Sysex (https://doczz.net/doc/8351592/developer-s-information-pack): dokumentiert 11-Byte-GS-Effektvariationsbefehle; die unveränderte Creative-DEMO empfängt SysEx bereits. Erweiterung ausschließlich der Host-MIDI-Transportlänge/FIFO: vollständige Nachricht oder warten, keine Teilpakete, normale MIDI-Nachrichten unverändert.
- Aktiviert beide vorhandenen P165-Typmenüs im SDK-Kompatibilitätsquelltext, Hall 2 = 4, Chorus 3 = 2, echte Projekt-/Parameteranbindung; v3-State für abweichende Effektvariationen, v1/v2 kompatibel. Kein Eingriff in Chip-DSP. NRPN laut ADIP und originalem DEMO ausdrücklich **nicht** durch SF2LIB unterstützt; ADSR/Filter und exakte Send-Stärken bleiben offen. Keine falsche Funktionsfreigabe.
- Neuer nativer Originaltest: Plate/Short Delay Feedback über IEditController und State-Roundtrip, anschließende Original-Registertrace-Kontrolle (INIT1 3 = B4FF, INIT3 9 = E6C0; unabhängige Referenz https://codebrowser.dev/linux/linux/sound/isa/sb/emu8000.c.html). Noch unbestätigt. Keine Besitzerdateien/Audio/Traces veröffentlichen, nur main.

## 24.11.2026 — Native 44,1 kHz unverändert; Hostrate-Konverter

- 44,1-kHz-DOSBox-Mixer unverändert, bittransparenter Bypass bei 44,1-kHz-DAW. Für höhere Hostraten bis 192 kHz: 32-Tap-Blackman-Sinc, 1024 Phasen mit Interpolation, exakte rationale Phase, 16 native Samples FIR-Verzögerung, keine Allokation im Callback. Keine Änderung des EMU8000-Takts oder Creative-Treibers. Asynchrone Transportlatenz und Pufferstabilität bleiben separate offene Prüfungen.
- Portable Tests prüfen 48/88,2/96/192 kHz gegen analytischen 3-kHz-Sinus, Stereo-Isolation, exakte Eingangsverbrauchsrate und Blockgrößenunabhängigkeit. Windows-Original-RAM-Preset-735-Hz-Gate zusätzlich bei echter VST3-Hostrate 48 kHz. Beide neuen Prüfungen noch offen. Vorheriger RAM-Test bleibt in CI-Warteschlange; keine Freigabe.

## 24.11.2026 — Original-SF2-Wechsel PASS; separate RAM-Sample-/Preset-Klangprüfung

- **[Windows 45290310337](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45290310337) vollständig SUCCESS** auf 0f7588a1; Transport 45290310165 und Version 45290310749 PASS. Tatsächliches IComponent-State-Roundtrip, aktiver Originalgast-Neustart, externe GM-Referenzbank, NoteOn/Off, Held-RMS 0,00828747 (ROM-GM 0,00837682), EMU8000-Writes 12903, Quelle unverändert. Vorheriger Timeout nach Umstellung auf temporäre Ausgabespools behoben. Kein bloßer synthetischer PASS.
- Jetzt zusätzliche selbst erzeugte SF2-2.00 mit zwei RAM-Sinus-Samples und exakt zwei Presets (0/17), keine ROM-Samples und keine fremden Audiodaten. Nativer Original-VST3-Test muss je Programm die passende 441/735-Hz-Signatur deutlich über der anderen Frequenz messen, zusätzlich zu den bisherigen Note/Ruhe/Release-Gates. Fixture nur temporär erzeugt, SHA unverändert. Originaler Creative-Lader und EMU8000 bleiben unverändert. Neue RAM-Prüfung noch nicht bestätigt. Keine Freigabe.

## 24.11.2026 — Aktiver SF2-Test: Timeout mit sicherer Log-Erfassung untersuchen

- Windows 45288976627: Build, synthetische Positiv-/Negativkontrollen und musikalischer Original-GM-Test erreicht (Held Note RMS 0,00795573). Separater aktiver SF2-/Recall-Test nach 90 s Timeout; Ursache nicht nachgewiesen. Transport/Version PASS.
- Beseitigt einen realen Deadlock-Pfad im Python-Testhost: Windows-Subprozess schrieb in nicht kontinuierlich geleerte stdout/stderr-PIPEs, während Elternprozess nur pollte. Temporäre lokale Dateien statt Pipes, begrenzte Diagnose bei Timeout, sofort geflushte native Marker vor/nach Wechsel. Keine Änderung der Klangschwellen oder Emulation. Neuer aktiver Windows-Test offen.

## 24.11.2026 — Native Recall-Testhost korrekt mit SDK MemoryStream linken

- Windows 45287430560 und 45287808293: Build des VST3 selbst nicht als fehlerhaft ausgewiesen; nativer Testhost scheiterte beim Linken an fünf undefinierten Steinberg-MemoryStream-Symbolen. Kein SF2-Klangurteil aus diesen Läufen. Transporttests 45287430562 und 45287808294 sowie Versionsprüfungen PASS.
- CMake bindet die Implementierung `public.sdk/source/common/memorystream.cpp` aus demselben gepinnten SDK explizit im Testhost ein. Keine Änderung an EMU8000, Creative oder GUI. Neuer Windows-Lauf erforderlich; kein Release.

## 24.11.2026 — Aktiver SF2-/ROM-Wechsel und echte sparse Presetliste

- Transport 45287430562 und Version 45287430420 PASS; Windows-Originalgate 45287430560 läuft noch. Neuer Lauf darf die ausstehende Originalprüfung nicht abbrechen (CI-Warteschlange statt Abbruch).
- SF2-/ROM-Knopf und State-Wechsel starten denselben originalen Creative-DOS-Gast im selben VST3 neu. Kein externer Prozess, keine DSP-Nachbildung. Lifecycle-Mutex ausschließlich blockierend außerhalb des Audiothreads; process() liefert beim nicht blockierenden try_lock-Ausfall Stille. Vorherige Bank bei synchronem Startfehler erneut versuchen. Originalgrafik byte-identisch; kompiliertes Kompatibilitätsdialog-Verhalten ersetzt. Wiederholter Gaststart im selben Prozess ausdrücklich **noch nicht nachgewiesen**, möglicher Blocker: globaler DOSBox-/SDL-Zustand.
- Bank-0-Presets aus tatsächlichem phdr, keine Auffüllung externer SF2. Höhere Bänke nicht freigegeben. Browser erhält getrennten Snapshot über UI-Revision, nicht vom Audiothread. Alte Presetnummern ohne vorhandenen Eintrag werden auf ersten vorhandenen Eintrag gesetzt. Kein Anspruch auf vollständige Treiber-Banksemantik.
- Der Windows-Originaltest lädt nun zuerst den ROM-Gast und wechselt über die echte IComponent-State-Schnittstelle bei aktivem Plugin zur externen GM-Referenz; danach vollständiges Note/Ruhe/Release-Gate. Dieser neue Test steht aus. Keine Besitzerdateien publizieren, kein Release, nur main.

## 24.11.2026 — SF2-Prüfskript korrigiert

- Windows 45287143543 und Transport 45287143763 brachen vor dem Klangtest wegen eines fehlerhaften PowerShell-Regex-Einfügevorgangs ab. Doppelt eingefügten Skriptabschluss entfernt; Trace-Filter wieder korrekt. Keine Änderung an Plugin, DSP oder Originaltreiber. Externer SF2-Recall-/Klangtest weiterhin offen.

## 24.11.2026 — Echter VST3-Recall-/SF2-Ladepfad als separates Windows-Gate

- [Windows 45284478927](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45284478927), [Transport 45284478906](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45284478906), [Version 45284478902](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45284478902) SUCCESS auf 17bb039b. Bisheriger GM-Originalpfad bleibt funktionsfähig; dieser Lauf hat noch keine externe Bank geladen.
- Neuer separater Originaltest: tatsächliches IComponent::setState/getState mit externer UTF-8-SF2-Datei, fehlender Pfad muss fehlschlagen, State bytegenau zurücklesen, danach realer VST3-Notentest mit externer temporärer GM-Referenzkopie über D:\\BANK.SF2. Creative-Bootdiagnose, EMU8000-Trace und unveränderter SHA-256 der Quelle erforderlich. Identische GM-Referenz prüft den Ladeweg, **nicht** RAM-Sample-Klang, andere Banken oder Live-Wechsel. Keine Besitzerdateien hochgeladen. Neuer Testlauf nicht bestätigt.
- Original-P165-Grafik, Creative-Treiber und EMU8000 bleiben unverändert. `0.0.0-dev`, nur main, kein Release. Keine ungefragten Zwischenberichte; Abnahme erst bei vollständigem Instrument.

## 23.11.2026 — Externe SF2-Startauswahl und Projektpfad

- [Windows 45281711814](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45281711814), [Transporttests 45281711812](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45281711812), [Version 45281711986](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45281711986): SUCCESS auf d4bf7111; Mapping und Original-GM-Pfad bestanden.
- Externe SF2-Auswahl über vorhandenen P165-SF2-Knopf im generierten SDK-Kompatibilitätsquelltext; Original-GUI-Blob und Grafik unverändert. Auswahl wird validiert und beim nächsten Aktivieren über die originale Creative-DEMO geladen (DOS-Pfad D:\\BANK.SF2, private temporäre Kopie, Quellbank nie überschrieben, SF2 2.01 nur Versionsheader angepasst). Keine Behauptung eines laufenden Bankwechsels. DBXD-State v2 speichert UTF-8-Pfad; v1 bleibt lesbar. Fehlende/inkompatible Bank wird abgelehnt, nicht still ersetzt. Katalog bleibt vorerst MIDI-Nummern, keine falschen Presetnamen/ROM-Mischung; aktive Live-Auswahl, vollständige Banksemantik und tatsächlicher RAM-Sample-Klang noch offen.
- Neue Implementierung nicht Windows-abgenommen. Originaler EMU8000 und Treiber unverändert, kein Release, nur main.

## 23.11.2026 — Original-GM-Klangvergleich PASS; VST3-MIDI-Controller angebunden

- **[Windows 45280056019](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45280056019) SUCCESS.** Direkter DOSBox-Mixer und VST3 (gleiche SHA-verifizierte Originaldateien, CC0/PC0/C4 Vel100): gehaltene Note RMS 0,008382091 vs 0,008381980, Ruhe 0,000100778 vs 0,000118569, Release 0,000309144 vs 0,000300079. 90 aktive Notenblöcke, keine Near-Full-Scale-Frames während Note. Direkter Mixer bereits beim Start Peak 1,0; kein DSP-Eingriff begründet. 12.904 EMU8000-Writes, Original-Gate und Negativkontrolle PASS. Keine samplegenaue oder allgemeine Banktreue behauptet.
- Neue produktive Host-Anbindung: standardkonformes IMidiMapping für 16 Kanäle, Modulation, Channel Volume, Pan, Expression, Sustain, CC91/93, Kanalmodus, Channel Pressure und 14-bit Pitch Bend über originalen MPU-401. Kein eigener Controller-DSP, keine künstlichen CC-Defaults beim Boot. Native Windows-Hostprüfung fragt Mapping über echte VST3-Schnittstelle ab. Musikalische Wirkung der Controller und samplegenaue Ereignisreihenfolge noch nicht abgenommen. Neue Windows-Prüfung ausstehend.
- Externe SF2-Auswahl, Recall, vollständige GUI-Regler und Host-Timing bleiben offen. Nur main, `0.0.0-dev`, keine Release-Freigabe.

## 23.11.2026 — Direkter DOSBox-Mixervergleich vor der VST3-Klangabnahme

- [Windows 45276546802](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45276546802): synthetischer Positivtest (1002 nicht-stumme Frames) und strikte Nullsignal-Negativkontrolle PASS. Musikalischer Originaltest FAIL: Ruhe-RMS 0,0761665, gehaltene Note 0,0283831, Release 0,00548934, 100/100 aktive Notenblöcke, 0 Near-Full-Scale-Frames während der Note. Ursache offen; kein Emulationsfehler behauptet. [Linux/Windows 45276546834](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45276546834) und [Versionsprüfung 45276546950](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45276546950) PASS.
- Der Launcher erfasst vor dem VST3-Gate den direkten PCM-Ausgang derselben DOSBox-DLL ohne VST3-Queue: identisches ROM, GM-SF2, Creative-EXE, 44,1 kHz, MIDI CC0=0/PC0/Note60 Vel100/NoteOff. Vier Fenster (Warmup, Ruhe, Note, Release) mit RMS/Peak/Framezahl, vollständige MIDI-Lieferung und regulärer Stop. Keine samplegenaue Gleichheitsbehauptung oder musikalischer PASS aus der Referenzmessung.
- Beide Wege jetzt 800 Warmup-, 80 Ruhe-, 100 Noten- und 250 Release-Blöcke à 512 Samples. Native Host-Taktung absolut bei 44100 Hz statt kumulativer Sleep-Rundung. Musikalische Note/Ruhe-Anforderung unverändert. Neuer Lauf noch ausstehend. Kein DSP-/Treiber-/GUI-Eingriff, nur main, `0.0.0-dev`, kein Release.

## 23.11.2026 — Negativkontrolle vom musikalischen Originaltest getrennt

- [Windows 45275384157](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45275384157) FAIL ausschließlich in der synthetischen Negativkontrolle: gemessene Ruhe-, Note- und Release-RMS sämtlich exakt null. Die neue musikalische Prüfung meldete jedoch einen anderen erwarteten Fehlertext; deshalb wurde der eigentliche Original-Archivtest nicht mehr erreicht. Windows-Build und synthetischer Positivtest (783 nicht-stumme Frames) PASS; [Linux/Windows 45275384197](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45275384197) und [Version 45275384621](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45275384621) SUCCESS.
- Korrektur: eigenständiger nativer `--negative`-Modus mit identischem MIDI-Stimulus wie im synthetischen Positivtest. Prüft alle Frames einschließlich Start, verlangt exakt null Spitzenpegel und null nicht-stumme Frames, keine vorzeitige Erfolgsmeldung. Der echte musikalische Originaltest bleibt unverändert streng. Neuer Windows-Lauf noch nicht bestätigt.
- **Verbindlicher Klangmaßstab:** zuerst VST3 gegen dieselbe originale DOSBox-X-AWE32-Laufzeit mit identischer Bank, MIDI-Sequenz und Mixer-Konfiguration vergleichen. SoundFont-, Creative-Treiber- und Transportfehler voneinander trennen. Der anhand echter AWE32-Hardware kalibrierte Chipkern ([AWETEST-Dokumentation](https://github.com/turican0/AWE32Emu/blob/main/docs/TESTING.md)) bleibt unverändert, bis eine reproduzierbare Abweichung eine gezielte Änderung begründet. Kein Normalisieren oder klangliches Nachregeln auf Verdacht.
- `0.0.0-dev`, nur `main`, P165 und EMU8000 unverändert. Erster Original-PCM-Nachweis 45267065002 bleibt gültig, musikalische Abnahme offen.

## 23.11.2026 — Original-Creative-ROM/SF2 erzeugt erstmals PCM im tatsächlichen Windows-VST3

- **[Windows 45267065002](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45267065002) vollständig SUCCESS**, Original-Archiv-Gate tatsächlich ausgeführt, nicht übersprungen: SHA-verifiziertes AWE-ROM, originale Creative-DEMO/SF2, tatsächliches VST3-PCM: 52.224 Stereo-Frames, 818 nicht-stumm, Peak 0,999969, zwei NoteOn und ein NoteOff gesendet; 12.823 EMU8000-Writes. Separater DLL-Test: 12.902 Writes an sechs Ports, PCM16-Peak 6.539, vier MIDI-Pakete, SDL-Dummy-Isolation und Stop/Join PASS. Positiv-/Negativkontrollen PASS; [Versionsprüfung 45267064799](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45267064799) SUCCESS. Keine privaten Besitzerdateien hochgeladen.
- **Beweisgrenze:** Der erste VST3-Test belegt Original-PCM, jedoch noch keine saubere musikalische Wiedergabe. Der fast volle Peak und die kurze Messzeit verlangen getrennte Messung nach dem Start. Die neue native Originalprüfung wartet 250 Blöcke auf Initialisierung, misst 80 Blöcke Ruhe, spielt einen einzelnen Ton 100 Blöcke und erfasst 250 Blöcke Release. Sie verlangt mindestens acht aktive Note-Blöcke und einen klaren RMS-Anstieg gegenüber Ruhe; sie protokolliert Near-Full-Scale-Frames ohne den Chip oder die Verstärkung zu verändern. Keine Frühfreigabe aufgrund eines Startimpulses mehr. Synthetisches Gate unverändert.
- **Neue musikalische Prüfung noch ausstehend**, kein DAW-/Hardwaretreue-Nachweis, keine SF2-Bankwechsel-Abnahme. `0.0.0-dev`, nur `main`, unveränderter originaler EMU8000 und P165.

## 23.11.2026 — Original-Creative-Archivmaterial im echten Windows-VST3: Forschungsprüfung nach P30

- Nutzerentscheidung: den früheren P30-Prüfweg weiterverwenden, statt die Originalklangprüfung an den Besitzer zu delegieren. P30 hat originales DEMO/SF2-Laden gezeigt, P31 dokumentierte weiterhin stumme bzw. ungültige Notenversuche. Keine Klangfreigabe aus diesen Befunden ableiten.
- Windows-CI führt nach den synthetischen Positiv-/Negativkontrollen einen **isolierten Archiv-Referenztest** mit bereits öffentlich auffindbaren Referenzdateien durch. ROM-Quelle ist der festgeschriebene libretro-pcem-Commit `e07923e902dcedcf65a502fb3f14e75b9ded5271`, ROM-Größe und SHA-256 müssen exakt dem bisherigen Referenzwert entsprechen. Der unveränderte bestehende Originaltest verifiziert Creative-EXE/SF2 und prüft die echte VST3-PCM-Ausgabe plus EMU8000-Trace und separaten DLL-Lauf. Keine privaten Benutzerdateien, keine Veröffentlichung von ROM, EXE, SF2, Audio, Trace oder befülltem Bundle. Temporäre Forschungsdateien werden gelöscht. Diese ausdrücklich abgegrenzte Ausnahme ersetzt die frühere Beschränkung auf rein synthetische CI nur für die öffentlich auffindbaren Archivreferenzen.
- Vorangegangene Windows-Paketvorbereitung [45265939318](../../actions/runs/45265939318) und Versionsprüfung [45265939138](../../actions/runs/45265939138) erfolgreich. **Neuer Original-Archivtest noch nicht abgeschlossen; kein Klang-PASS**, keine Studio-Pro-Abnahme, `0.0.0-dev`, nur `main`. P165 und EMU8000 unverändert.

## 23.11.2026 — Vorbereitete Windows-Testpaketierung ohne private Originaldaten

- Der bestehende Windows-Integrationsworkflow bereitet das tatsächlich kompilierte VST3, den nativen Steinberg-Testhost, die beiden privaten Python-Prüfer und den lokalen PowerShell-Launcher **vor** dem Einbringen synthetischer DOS-Testdateien in einem separaten Verzeichnis vor. Ein expliziter Dateicheck verweigert ROM/SF2/SBK/DEMO.EXE/PRIVATE-Inhalte. Der CMD-Starter verwendet den lokalen Besitzer-ROM und den vorgebauten Testhost, ohne erneuten C++-Build.
- Ein GitHub-Actions-Download wird **nur bei einem privaten Repository** und nach erfolgreichen bestehenden Windows-Gates bereitgestellt. Das aktuelle Repository ist öffentlich; kein öffentlicher Binärdownload, keine Veröffentlichung proprietärer Bestandteile und kein Release. Windows-CI für die neue Paketierung noch nicht bestätigt.
- Der authentische Original-Creative-Klangtest bleibt offen. `0.0.0-dev`, nur `main`, P165 und originaler EMU8000 unverändert.

## 23.11.2026 — Original-Creative-DOS-Fehlerursachen werden im privaten echten VST3 sichtbar

- Original-Programmquelle `DOS/DEMO/DEMO.C` erneut kontrolliert: Hardware-Detect, Chip-Init, MPU-401-Init und SF2-Request/-Preset-Fehler erzeugen ursprüngliche DOS-`ERROR:`-Meldungen. Für private Windows-Testläufe bisher unsichtbar.
- **Ohne Creative-Treiber- oder EMU8000-Änderung** leitet `DosBoxRuntimeHost.hpp` nur im ausdrücklich privaten `AWE64_ORIGINAL_CREATIVE_ONLY=1`-Modus die originale DOS-Textausgabe von `DEMO.EXE` in eine strikt validierte eigene 8.3-Logdatei. `Run-Private-Creative-AWE-Test.ps1` verwendet einen pro Lauf neu erzeugten Namen und stellt geänderte Umgebungsvariablen zurück.
- `WindowsRunVst3HostPcmSmoke.py` meldet erkannte authentische Creative-`ERROR:`-Zeilen bereits während des isolierten Windows-VST3-Hostlaufs und bricht bei echten DOS-Startfehlern gezielt ab, ohne falsch einen Klangerfolg zu melden. Stale oder nie angelegte Logdateien sind FAIL.
- **[45263904252](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45263904252): Linux UND Windows SUCCESS**, einschließlich aller neuen `tests/test_private_boot_diagnostic.py`-Konstruktionen mit rein synthetischen DOS-Fehlermeldungen, Kindprozessen, negativen Datei- und 8.3-Pfadfällen. **[45263904174](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45263904174): echtes Windows-VST3, Original-DOSBox-X-DLL, synthetischer MIDI→PCM-POSITIV- und Creative-only-NEGATIVtest SUCCESS:** 62.464 Stereo-Frames, 921 hörbar, 8 NoteOn, Peak 0,304871; Negativtest 0 nicht-stumme Frames. [Versionsprüfung 45263904207](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45263904207) SUCCESS. Die originale P165-GUI, originaler EMU8000-Chip und öffentliche synthetic Soundproben unverändert. **Authentischer Original-Creative-ROM/SF2-Klang und der neue Logpfad am echten Besitzer-Windows-Gast bleiben NOCH NICHT getestet.** `0.0.0-dev`, kein Release.


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
