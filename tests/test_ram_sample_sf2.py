"""Strict generated fixture structure; no proprietary inputs."""
import struct
import unittest
from MakeRamSampleSf2 import build

def children(data, start, end):
    offset = start
    while offset < end:
        assert offset + 8 <= end
        kind, size = struct.unpack_from("<4sI", data, offset)
        # SF2 INFO strings include the alignment byte in their chunk size.
        assert size % 2 == 0, (kind, size)
        stop = offset + 8 + size
        assert stop <= end
        yield kind, data[offset + 8:stop]
        offset = stop
    assert offset == end

class RamFixtureTest(unittest.TestCase):
    def test_even_chunks_and_exact_programs(self):
        data = build()
        self.assertEqual(data[:4], b"RIFF")
        self.assertEqual(struct.unpack_from("<I", data, 4)[0], len(data)-8)
        self.assertEqual(data[8:12], b"sfbk")
        lists = dict((body[:4], dict(children(body, 4, len(body))))
                     for tag, body in children(data, 12, len(data))
                     if tag == b"LIST")
        self.assertEqual(lists[b"INFO"][b"ifil"], b"\x02\x00\x00\x00")
        self.assertTrue(lists[b"INFO"][b"INAM"].endswith(b"\0"))
        pdta = lists[b"pdta"]
        self.assertEqual(len(pdta[b"phdr"]), 3 * 38)
        self.assertEqual([struct.unpack_from("<H", pdta[b"phdr"], i*38+20)[0]
                          for i in range(2)], [0,17])
        self.assertEqual(len(pdta[b"shdr"]), 3 * 46)
        self.assertEqual([struct.unpack_from("<H", pdta[b"shdr"], i*46+44)[0]
                          for i in range(2)], [1,1])  # mono RAM, not ROM
        self.assertEqual(len(lists[b"sdta"][b"smpl"]), (4000+46+3600+46)*2)

    def test_wet_fixture_has_explicit_both_sends(self):
        data = build(wet=True)
        lists = dict((body[:4], dict(children(body, 4, len(body))))
                     for tag, body in children(data, 12, len(data)) if tag == b"LIST")
        igen = lists[b"pdta"][b"igen"]
        self.assertEqual(len(igen), 11*4)
        self.assertEqual(struct.unpack_from("<4H",igen,0),(15,1000,16,1000))
        self.assertEqual(struct.unpack_from("<4H",igen,20),(15,1000,16,1000))

    def test_overlay_fixture_has_distinct_envelopes_and_harmonics(self):
        data = build(overlay=True)
        lists = dict((body[:4], dict(children(body, 4, len(body))))
                     for tag, body in children(data, 12, len(data)) if tag == b"LIST")
        pdta = lists[b"pdta"]
        first, second, end = struct.unpack_from("<6H",pdta[b"ibag"])[::2]
        self.assertEqual(first, 0)
        self.assertEqual(end, second*2)
        values = dict(struct.iter_unpack("<Hh",pdta[b"igen"][:second*4]))
        self.assertEqual(set(values), {8,9,11,25,26,27,28,29,30,33,34,35,36,37,38,54,53})
        self.assertEqual(values[11],2400)
        self.assertEqual(values[38],1200)
        self.assertEqual(values[54],1)
        self.assertNotEqual(build(),data)
        self.assertNotEqual(build(wet=True),data)
        with self.assertRaises(ValueError):
            build(wet=True, overlay=True)

if __name__ == "__main__":
    unittest.main()
