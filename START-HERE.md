# START HERE – 125A AWE64 Gold VST3

**Verbindliche aktuelle Version und echte Prüfergebnisse:** [AKTUELLE-VERSION.md](AKTUELLE-VERSION.md).  
**Anweisungen für Codex und spätere Chats:** [AGENTS.md](AGENTS.md).  
**Chronologischer Änderungsverlauf:** [CHANGELOG.md](CHANGELOG.md).  
**Private ROM-/Creative-/SF2-Fundorte:** [docs/TEST-ASSET-LOCATION.md](docs/TEST-ASSET-LOCATION.md).

## Festgelegte Architektur (gemeinsame Entscheidung)

Ein einziges Windows-x64-VST3 mit internem gepinntem **DOSBox-X AWE32 + originalem Creative-DOS-SF2LIB/DEMO.EXE**. Originale P165-GUI mit nachträglich um 73 % abgedunkelten Reglergruppen. **Kein Win95/86Box-PC**, keine getrennt zu startende DOSBox-EXE, kein eigener nativer Ersatz-SF2-Player.

**Submodul:** `third_party/dosbox-x-AWE32` auf `f3a95b9f19a7f9863295e6481f271cf04733c635` gepinnt. **Einziger Branch:** `main`.

## Tatsächlich gebaut

- Der **Original-DOSBox-X-AWE32-Quellcode** ist als interne Windows-x64-DLL gebaut. Ein originaler DOSBox-DOS-Befehl wurde im Windows-Testprozess ausgeführt und der Emulator verließ den Prozess nicht.
- Die **ursprüngliche P165-GUI**, der Windows-VST3-Host und die originale DOSBox-X-DLL wurden bereits zusammen in einem einzigen VST3-Bundle kompiliert.
- Original-MPU401-MIDI-Anbindung, PCM-Bridge, verstecktes DOSBox-SDL-Fenster und internes audiofreies SDL-Backend sind implementiert; Windows-Integrations- und I/O-Tests wurden tatsächlich bestanden ([vollständige Windows-Integration 45175109078](https://github.com/challanger2000/125A-AWE64-Gold-VST3-NEU/actions/runs/45175109078)); **der echte Creative-DOS-/ROM-/SF2-Klangtest ist weiterhin offen**.
- **Noch KEIN fertiges Instrument:** Creative-DOS-Programm mit den echten privaten Originaldaten innerhalb des VST3, echter AWE-Sound, State/Host/Studio-Pro-Abnahme stehen weiterhin aus.

## Fortsetzung – ausschließlich dieser Pfad

1. Neuesten Actions-Lauf zur originalen DOSBox-X-In-Process-Laufzeit und den echten DOS-UART-MIDI-Tests prüfen und etwaige Fehler direkt korrigieren.
2. Private Original-ROM-, `DEMO.EXE`- und `SYNTHGM.SF2`-Dateien lokal bereitstellen und originalen Creative-Klang **im VST3**, nicht bloß im DOS-Shell-Smoke-Test, nachweisen.
3. SF2-Auswahl/GUI-Parameter und projektfesten Recall zuverlässig implementieren; Clock/PCM/Latenz/Audio-Sends und sämtliche Abnahmetests durchführen. Studio-Pro-Praxistest zuletzt.
4. Erst danach freigeben; Versionsnummer, Installer, Git-Tag und Download müssen identisch sein.

**Keine neuen Branches, keine Wiederaufnahme der alten Win95/86Box/P009-Wege, keine Zwischenberichte und kein Paket mit falscher Fertigmeldung.**
