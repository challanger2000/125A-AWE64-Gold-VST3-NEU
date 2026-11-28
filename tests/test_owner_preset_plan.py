import random
import unittest
from OwnerPresetPlan import groups, range_of, screen_probes, select_probes


def signature(regions, key, vel):
    return tuple(i for i, (a,b,c,d) in enumerate(regions) if a <= key <= b and c <= vel <= d)


def verify(regions, probes):
    expected = {signature(regions,k,v) for k in range(128) for v in range(1,128)} - {()}
    actual = {signature(regions,k,v) for k,v in probes}
    if actual != expected:
        raise ValueError('missing or unreachable signatures')


class OwnerPresetPlanTest(unittest.TestCase):
    def test_global_override_and_legacy_subzones(self):
        zones = groups([0,2], [(0,0),(2,0),(6,0)],
                       [(43,0x500a),(44,0x7f01),(43,0x4020),(53,0),(43,0x6050),(53,1),(0,0)], 53)
        self.assertEqual(zones, [[{43:0x4020,44:0x7f01,53:0},{43:0x6050,44:0x7f01,53:1}]])
        with self.assertRaises(ValueError):
            range_of({43:0x007f},43,(0,127))
        with self.assertRaises(ValueError):
            groups([0,2],[(0,0),(1,0)],[(53,0),(0,0)],53)

    def test_split_and_overlap_with_negative_control(self):
        regions = [(0,35,1,127),(36,84,1,79),(36,84,65,127),(85,127,1,127)]
        probes = select_probes(regions)
        verify(regions,probes)
        for p in screen_probes(regions):
            self.assertTrue(signature(regions,*p))
        with self.assertRaises(ValueError):
            verify(regions,[(60,100)])
        with self.assertRaises(ValueError):
            verify(regions,[p for p in probes if signature(regions,*p)!=(1,2)])

    def test_random_rectangles_exhaustive(self):
        rng = random.Random(125)
        for _ in range(25):
            regions=[]
            for _ in range(rng.randrange(1,12)):
                a,b=sorted(rng.sample(range(128),2)); c,d=sorted(rng.sample(range(1,128),2))
                regions.append((a,b,c,d))
            verify(regions,select_probes(regions))
            self.assertTrue(all(signature(regions,*p) for p in screen_probes(regions)))

    def test_empty_and_narrow(self):
        self.assertEqual(select_probes([]),[])
        self.assertEqual(screen_probes([]),[])
        self.assertEqual(select_probes([(35,35,110,110)]),[(35,110)])
        self.assertEqual(screen_probes([(35,35,110,110)]),[(35,110)])


if __name__ == '__main__':
    unittest.main()
