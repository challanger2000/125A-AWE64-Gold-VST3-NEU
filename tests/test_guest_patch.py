import unittest
import importlib.util
from pathlib import Path
script=Path(__file__).resolve().parents[1] / "tools" / "patch_creative_demo.py"
spec=importlib.util.spec_from_file_location("awe64_creative_patch",script)
mod=importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class GuestPatchTests(unittest.TestCase):
    original='#include "ctaweapi.h"\nint main(void){\n    loop:\n        if (kbhit()) return 1;\n}'
    def test_inserts_once_without_changing_demo_logic(self):
        result=mod.patch_creative_demo(self.original)
        self.assertIn('#include "AWE64_BRIDGE.H"',result)
        self.assertIn("AWE64_GuestPoll();\n        if (kbhit())",result)
        self.assertEqual(result.replace('#include "AWE64_BRIDGE.H"\n','')
                         .replace('        AWE64_GuestPoll();\n',''),self.original)
        with self.assertRaises(ValueError): mod.patch_creative_demo(result)
    def test_refuses_ambiguous_or_unknown_version(self):
        with self.assertRaises(ValueError): mod.patch_creative_demo("int main(){}")
        with self.assertRaises(ValueError): mod.patch_creative_demo(
            self.original + self.original)

if __name__ == "__main__":
    unittest.main()
