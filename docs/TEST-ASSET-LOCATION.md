# Private ROM- und Soundbank-Testdateien – Fundort

**Gültig ab 23.11.2026.**

Diese Dateien liegen in der **privaten ChatGPT-Dateibibliothek des Projektinhabers**, nicht im GitHub-Repository und nicht in den GitHub-Actions-Runnern. Vor erneuter Bitte um Upload **zuerst die Dateibibliothek gezielt nach dem exakten Dateinamen durchsuchen**. Bei programmatischer Verwendung die gefundene Bibliotheksdatei mit der Files-Funktion materialisieren.

| Zweck | Exakter Bibliotheksdateiname | Erwartete Dateigröße |
| --- | --- | ---: |
| Original-AWE-Wave-ROM | `awe32.raw` | 1.048.576 Bytes |
| Originale Creative-Standardbank | `SYNTHGM.SBK` | 34.832 Bytes |
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
