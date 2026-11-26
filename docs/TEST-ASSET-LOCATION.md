# Private ROM- und Soundbank-Testdateien – Fundort

**Gültig ab 23.11.2026.**

**27.11.2026 – neue installierte Struktur:** `125A AWE64 Gold.vst3/Contents/Resources/ROM/` mit `awe32.raw`, `Driver.exe` (byteidentisches archiviertes `DEMO.EXE`) und `SYNTHGM.SF2`. `PRIVATE/` bleibt der nicht versionierte Arbeitsbereich; frühere Pfade/Dateinamen in historischen Beispielen beziehen sich auf alte Revisionen. Keine Originalbytes in GitHub; neue Paketprüfung noch ausstehend.

**Aktuelle Prüfentscheidung 23.11.2026 (hat Vorrang vor früheren rein synthetischen CI-Regeln):** Der Nutzer verlangt Fortsetzung des P30-Forschungswegs. Ausschließlich die unabhängig öffentlich auffindbaren, hashverifizierten Archiv-Referenzdateien dürfen temporär im isolierten Windows-Archivtest verwendet werden. Keine privaten Nutzerdateien in CI, kein Upload von Creative-/ROM-/SF2-Bytes, Audio, Traces, Logs oder befüllten Bundles; kein Release. Der alte synthetische Positiv-/Negativtest bleibt getrennt. Ein Original-Archivtest-PASS ist kein Besitzer-/DAW-/Hardwaretreue-PASS. Der neue Lauf ist noch nicht abgeschlossen.


Diese Dateien liegen in der **privaten ChatGPT-Dateibibliothek des Projektinhabers**, nicht im GitHub-Repository und nicht in den GitHub-Actions-Runnern. Vor erneuter Bitte um Upload **zuerst die Dateibibliothek gezielt nach dem exakten Dateinamen durchsuchen**. Bei programmatischer Verwendung die gefundene Bibliotheksdatei mit der Files-Funktion materialisieren.

| Zweck | Exakter Bibliotheksdateiname | Erwartete Dateigröße |
| --- | --- | ---: |
| Original-AWE-Wave-ROM | `awe32.raw` | 1.048.576 Bytes |
| Originale Creative-SBK-Standardbank (anderes Format) | `SYNTHGM.SBK` | 34.832 Bytes |
| Originale Creative-SF2LIB-GM-Referenzbank | `SYNTHGM.SF2` aus `DOS/DEMO/SYNTHGM.SF2` des originalen Creative ADIP 3.01; **privater Runtime-Pfad noch nicht bereitgestellt** | 42.726 Bytes |
| Haupttestbank | `Synth 512 Bank.sf2` | 555.754 Bytes |
| Zusätzliche Testbank | `Vintage Dreams Bank.sf2` | 315.982 Bytes |
| Zusätzliche Testbank | `Digital Bliss.sf2` | 1.722.362 Bytes |
| Zusätzliche Testbank | `FM Dreams Sampler.sf2` | 1.394.908 Bytes |
| Zusätzliche Testbank | `Callanish.SF2` | 8.251.362 Bytes |
| Zusätzliche Testbank | `Vintage Synth 2001.sf2` | 4.519.324 Bytes |
| Drum-Testbank | `ALESIS D4 Drum Module.sf2` | 26.010.610 Bytes |

Die Bibliothek enthält teils zusätzlich gleichnamige Kopien mit `(1)`, `(2)` oder `(3)`. **Zuerst die unnummerierte Originalbenennung prüfen.** Dateigrößen dienen als grobe Gegenprobe; vor Klangvergleichen außerdem reale Datei-Hashes prüfen. Diese Liste ist ein **Fundortverzeichnis**, keine Aussage, dass alle Testdateien schon lokal oder in GitHub Actions gemountet sind.

## Arbeitsablauf

1. **ChatGPT-Dateibibliothek** über die Files-Funktionen suchen/listen; exakten Namen verifizieren.
2. Nur für lokale Tests in einen **nicht versionierten privaten Arbeitsordner `PRIVATE/`** materialisieren. Dieser Ordner steht in `.gitignore`.
3. Keine ROMs, Creative-Binärdateien, SBK oder SF2 in GitHub einchecken, keine Bibliotheksdatei-IDs oder Zugangsdaten öffentlich dokumentieren.
4. GitHub Actions hat **keinen direkten Zugang zur privaten ChatGPT-Dateibibliothek**. Öffentliche CI muss ohne diese Dateien auskommen; echte ROM/SBK/SF2-Abnahmen laufen isoliert mit den privat bereitgestellten Originaldateien.
5. Bei späteren Aufgaben **nicht erneut um Upload bitten**, bevor die Bibliothek geprüft wurde. Falls eine Datei tatsächlich nicht mehr zugreifbar ist, die Einschränkung genau nennen.

**Original-Creative-DOS-Treiber und DOS-SF2LIB**: deren tatsächlichen Fundort und Prüfsumme getrennt dokumentieren, sobald die konkret verwendete Originalbinärdatei gesichtet wurde. Vorher keinesfalls behaupten, sie liege in GitHub oder im Testordner.

## Originale Creative-DOS-Executable und ROM-FM-Bank (Prüfort)

- Im Original-Creative-Archiv `qb40/sound-blaster-awe32` befinden sich `DOS/DEMO/DEMO.EXE` und `DOS/DEMO/SYNTHGM.SF2`; frühere private P30-Tests haben den tatsächlichen Original-Lauf dieser Kombination mit dem Besitzer-ROM nachgewiesen.
- **Original-`DEMO.EXE` SHA/Git-Blob:** `e10b8705c6a46444a998c4f6aa74a1cda2081506` (P30-Protokoll); **`SYNTHGM.SF2` SHA-256:** `78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f` (damaliger Originaldaten-Audit).
- Das aktuelle VST3 benötigt `DEMO.EXE` und `SYNTHGM.SF2` als **privates** Laufzeitmaterial neben dem ROM in `Contents/Resources/PRIVATE`. Die Dateien werden nicht ins öffentliche GitHub oder GitHub-Actions aufgenommen.
- Die originale SF2LIB-Demo erkennt Bankaufrufe an **`/SBK:`**, unabhängig von der falsch ausgegebenen `/SF2:`-Hilfemeldung. `SYNTHGM.SBK` ist nicht automatisch `SYNTHGM.SF2`.
- **Kein bestätigter VST3-Test dieser Originaldateien im neuen DOSBox-X-VST3**; sobald private Einbindung verfügbar ist, zuerst tatsächlichen DOS/Originalsound nachweisen.

## Windows-Originalklang-Gate

Nach privater Paketierung mit `tools/prepare_private_vst3.py` kann die tatsächliche originale DOSBox-X-DLL innerhalb des **unveröffentlichten** VST3-Bundles geprüft werden:

```powershell
python tests/WindowsPrivateCreativeGuestSmoke.py --bundle PRIVATE/AWE64GoldDOSBoxDev.vst3
```

Die Prüfung braucht neben dem bereits vorhandenen `awe32.raw` ausdrücklich die originale `DOS/DEMO/DEMO.EXE` (Git-Blob `e10b8705c6a46444a998c4f6aa74a1cda2081506`) und `SYNTHGM.SF2` (SHA-256 `78f8739a25d49e519b3fdc92a7f03d4ddd68588fd0260578a880296e304b7b5f`). Beide stammen aus dem in dieser Datei dokumentierten Creative-ADIP-Originalfundort. Nur nach Originalprüfung privat bereitstellen. Dies ist ein **DLL-Gast-Klangtest**, noch keine automatische VST3-Host-/Studio-Pro-Abnahme. Bei einer Fehlprüfung keine Nachweise erfinden.

## Privat nutzbare Windows-Prüfung ohne GitHub-Upload

Nach vollständig lokal gebautem VST3-Bundle kann der tatsächliche Original-DOS-Creative-Klang **nur auf dem Besitzer-Windows-PC** mit den nicht öffentlich verfügbaren Daten geprüft werden:

```powershell
pwsh -File tools/Run-Private-Creative-AWE-Test.ps1 -Bundle "build-vst3/VST3/Release/AWE64GoldDOSBoxDev.vst3" -Rom "C:\Private\awe32.raw"
```

Das Skript benötigt den eigenen ROM-Dateipfad und ein bereits gebautes VST3-Bundle, klont das öffentliche Original-Creative-Entwicklerarchiv nur **auf den lokalen Windows-PC**, validiert `DEMO.EXE` über den Git-Blob und `SYNTHGM.SF2`/ROM über die dokumentierten SHA-256-Hashes, verweigert das Überschreiben anderer privater Dateien und führt die vollständig echte `WindowsPrivateCreativeGuestSmoke.py`-Prüfung aus. Weder GitHub Actions noch das öffentliche Repository enthalten proprietäre Originaldateien. Das ist kein behaupteter Nachweis eines erfolgreichen privaten Windows-Laufs.

## Neues privates echtes VST3-/Creative-DOS-Sound-Gate (23.11.2026)

Der PowerShell-Launcher `tools/Run-Private-Creative-AWE-Test.ps1` prüft **nicht mehr nur die originale DOSBox-DLL**, sondern auch den **wirklichen Windows-VST3-`IAudioProcessor::process()`-Klangpfad** mit authentischer Creative-`DEMO.EXE`, `SYNTHGM.SF2` und dem Besitzer-ROM. Der native Testhost `awe64_vst3_host_pcm_smoke.exe` muss im selben lokalen Windows-Build vorhanden sein; optional `-HostExe <Pfad>` verwenden. Er liefert NoteOn/NoteOff über die echte Steinberg-`IEventList`. Erst bei nachgewiesener echter VST3-PCM-Ausgabe **und** EMU8000-Originalregisterwrites plus authentischen Asset-Hashes ist die private Testabnahme positiv.

```powershell
powershell -File tools/Run-Private-Creative-AWE-Test.ps1 -Bundle "build-vst3/VST3/Release/AWE64GoldDOSBoxDev.vst3" -Rom "C:\Private\awe32.raw" -Seconds 60
```

Kein Originalbestandteil wird durch diese Skripte ins öffentliche GitHub eingecheckt oder per Actions ausgeführt. Die private Windows-Ausführung **steht noch aus**; das Vorhandensein der Prüfschnittstelle bedeutet **keinen** Klang-PASS.

## Ein einzelner lokaler privater Windows-Build-und-Klangprüfaufruf

Neu: `tools/Build-And-Test-Private-AWE64.ps1` baut auf Windows x64 die gepinnte DOSBox-X-AWE32-SDL2-DLL aus einem **gitignorierten privaten Arbeitscheckout** und das echte P165-Windows-VST3 mit Steinberg SDK 3.7.12. Danach führt es automatisch den privaten Original-Creative-`DEMO.EXE`-/`SYNTHGM.SF2`-/Besitzer-ROM-Test durch, einschließlich echter MIDI/PCM/EMU8000-Portprüfungen. Kein Kopieren in einen öffentlichen GitHub-Runner, Originaldateien bleiben nur im lokalen gitignorierten `build-vst3`-/`PRIVATE`-Bereich und kurzzeitig im lokalen Windows-`TEMP` (mit anschließender Bereinigung), keine zusätzlichen Branches.

```powershell
powershell -File tools/Build-And-Test-Private-AWE64.ps1 -Rom "C:\Private\awe32.raw" -Seconds 60
```

Voraussetzungen: lokales Windows x64, Visual Studio 2022 mit MSBuild/C++, CMake, Python 3, Git und Netzwerkzugriff für den öffentlichen Steinberg-SDK- und Creative-Archiv-Checkout. Dieser Schritt **läuft nicht automatisch in GitHub Actions** und wurde hier **nicht** ausgeführt. Nur nach nachgewiesener ursprünglicher Creative-EMU8000-Klangausgabe kann ein solcher privater Test als PASS markiert werden; Studio Pro, GUI, Recall und Release bleiben getrennte spätere Gates.

## Private Original-Creative-Startdiagnose (nur Besitzer-Windows, 23.11.2026)

Der private Launcher `tools/Run-Private-Creative-AWE-Test.ps1` erzeugt für jeden echten `--original`-VST3-Hostlauf den einzigartigen DOS-8.3-Dateinamen `Bxxxxxxx.LOG` (sieben zufällige Hexziffern, Großschreibung) im **bereits lokal vorhandenen** `AWE64GoldDOSBoxDev.vst3/Contents/Resources/PRIVATE/`-Verzeichnis. `DEMO.EXE` schreibt seine eigenen originalen DOS-Startfehler über die DOS-Shell-Umleitung in diese Textdatei. Der Controller liest ausschließlich diese Diagnose und beendet den Kindprozess bei `ERROR:` direkt, statt nach schweigender SoundFont-Initialisierung eine Klangbehauptung zu treffen. Die Datei **bleibt auf dem Besitzerrechner**, wird nicht auf öffentliche GitHub-Actions-Runner und nicht ins Repo hochgeladen. Bei fehlender Logdatei oder vorbestehendem Namen ist die private Testabnahme ungültig; der normale Produktstart besitzt diese Redirection **nicht**.

## 24.11.2026 — Cheezoids: gezielte Regression aller 128 Programme

- Nutzer meldet aus früherem Stand mehrheitlich falsche kurze Bleep-Klänge. Die konkrete private Datei war in der Bibliotheksliste nicht vorhanden. Öffentlich freigegebene Originalbank von Analoguesque Sound Designs (Ian Wilson) im Autor-Repository `Analoguesque/freepatches`, Commit `681907983ec024c1994f6a02615d650877913696`, Archiv-Git-Blob `c12461b3e016f70b36e39eca2f78eac5685a0fbf`. Nur temporär herunterladen, keine Bankdaten, PCM oder Traces hochladen. Identität zur früheren privaten Kopie nicht behaupten.
- Autor beschreibt kurze Single-Cycle-Wellenformen und Kompatibilitätsprobleme mancher SF2-Player. Deshalb ausdrücklich kein einzelner Peak als Klangbeweis: alle 128 tatsächlichen Bank-0-Programme, Note 60 Velocity 100, etwa 1,75 s gehalten, frühe/späte RMS-Fenster und Original-Creative-DLL-Direktvergleich mit identischen trockenen Hardware-Sends; VST3-Programmauswahl und Bankwechsel über echten Host. Meldet verlorenen Sustain, abweichende Programmpegel und kurze Originalreferenzen getrennt; Mehrheits-Bleep auch im direkten Originalpfad bleibt Fehler zur Untersuchung. Originalquelle unverändert. Noch kein ausgeführter Cheezoids-Klang-PASS.


## 24.11.2026 — Exakt erneut hochgeladene Cheezoids-Problemrevision festgeschrieben

- Besitzerdatei `Cheezoids.sf2`, 652206 Bytes, SHA-256 `5adad4b3b95a33ef64ae06dd1627038140435b395d183aab622efcb4c920359b`. SF2 2.01, alle 128 Bank-0-Programme, 160 Samples, 1275 Instrumentzonen mit SampleMode 1 (kontinuierlicher Loop). Originalbytes nicht verändern/hochladen. Die öffentliche Autorenbank ist **nur dann** ein Ersatz für diesen konkreten Test, wenn ihr SHA-256 exakt übereinstimmt.
- Tatsächlich lokal gerenderte unabhängige Referenz via libfluidsynth.so.3: alle 128 Programme bei MIDI 60/Velocity 100, 44,1 kHz, 1,75 Sekunden gehalten, Reverb/Chorus aus. Frühfenster 0,2–0,6 s, spät 1,1–1,6 s. 114 Programme mit spätem RMS >=0,0005 und mindestens 10 % des frühen RMS; 11 kurze Abklingverläufe (6,10,20,22,28,45,47,49,52,73,75), 3 sehr leise (23,53,77). Damit ist die Mehrheit kurzer Bleeps **nicht** in dieser Datei bei allgemeiner SF2-Wiedergabe inhärent. FluidSynth ist keine AWE-Klangreferenz; Original-Creative-DOS und reales VST3 bleiben gesondert zu prüfen. Warnungen für Instrumente PNO Bass/PIANO (ungültige Generatoren verworfen).
- Neuer exakter Hash-Gate für Autorendownload und private lokale Datei `--owner-bank` bzw. `-Cheezoids`; andersartige öffentliche Revision darf die konkrete Benutzerregression nicht bestehen. Originale Benutzerdatei nicht im Repo, keine Weitergabe. Noch kein tatsächlicher Windows-VST3-Klangnachweis dieser exakten Revision.

