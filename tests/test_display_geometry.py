"""Contract for the recovered P168 display layout; not a screenshot test."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class DisplayGeometry(unittest.TestCase):
    def test_recovered_owner_coordinates(self):
        src=(ROOT/"gui/PresetBrowserView.cpp").read_text(encoding="utf-8")
        for marker in ('MakeLabel({370,96,570,112},"EMU8000 / AWE64 GOLD"',
                       'fileLabel_=MakeLabel({575,96,728,112}',
                       'programLabel_=MakeLabel({370,118,728,145}',
                       'hardwareInfoLabel_=MakeLabel({370,145,728,163}',
                       'hint_=MakeLabel({370,166,728,184}'):
            self.assertEqual(src.count(marker),1,marker)
        self.assertNotIn('CREATIVE WIN95"',src)
        self.assertNotIn('MakeLabel({350,',src)
    def test_owner_contrast_and_english_labels(self):
        src=(ROOT/"gui/PresetBrowserView.cpp").read_text(encoding="utf-8")
        cmake=(ROOT/"CMakeLists.txt").read_text(encoding="utf-8")
        self.assertEqual(src.count("CColor(7,11,16,186)"),2)
        for marker in ('Search instruments...', 'Bank selection failed: audio engine unavailable.'):
            self.assertIn(marker,src)
        for old in ('Instrument suchen...', 'Bank-Neuauswahl fehlgeschlagen', 'Alle Dateien (*.*)'):
            self.assertNotIn(old,src)
        self.assertIn('Host rejected the preset selection.',cmake)
        self.assertNotIn('Host hat die Preset-Auswahl abgelehnt.',cmake)
    def test_display_uses_validated_default_bank(self):
        shell=(ROOT/"src/DosBoxVst3Shell.cpp").read_text(encoding="utf-8")
        self.assertIn('DisplayStatus::bankName(requestedBankDigestValid_,requestedBank_)',shell)
        self.assertIn('guestStatus_.store(-4',shell)
        self.assertNotIn('DOSBOX-X: GAST',shell)
        self.assertNotIn('KEIN AUDIO-GAST',shell)
if __name__=="__main__":unittest.main()
