"""Public synthetic test for the one-byte private research repair."""
import unittest
from WindowsPrivateCreativeGuestSmoke import repair_sysex_compare, corrected_sysex_copy

class CreativeSysexFixTest(unittest.TestCase):
    def test_exactly_one_byte_and_input_unchanged(self):
        src = bytearray(40197)
        offset = 21834 + 0x35
        # Synthetic representation of the instruction sequence only.
        seq = bytes.fromhex("2e8a04988e460a8bd903df2af6268a173bd0")
        src[offset:offset+len(seq)] = seq
        source = bytes(src)
        fixed = repair_sysex_compare(source)
        self.assertEqual(len(source),len(fixed))
        self.assertEqual([i for i,(a,b) in enumerate(zip(source,fixed)) if a!=b],[offset+3])
        self.assertEqual(source[offset+3],0x98)
        self.assertEqual(fixed[offset+3],0x90)
        with self.assertRaises(ValueError):
            repair_sysex_compare(fixed)
        with self.assertRaises(ValueError):
            corrected_sysex_copy(source)

if __name__ == "__main__":
    unittest.main()
