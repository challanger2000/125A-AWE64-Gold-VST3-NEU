# Private ROM- und Soundbank-Testdateien – Fundort

**Gültig ab 23.11.2026.**

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
