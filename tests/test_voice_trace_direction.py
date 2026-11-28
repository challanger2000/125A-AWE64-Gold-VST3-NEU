import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from WindowsPrivateCreativeGuestSmoke import summarize_filter_envelope_trace

PAIRS = ((2,3),(4,5),(6,7),(8,9),(10,11),(19,20),(21,22),(23,24),(12,13),(14,15))
EXPECTED = ((66,6),(69,12),(64,127),(69,12),(66,6),(69,12),(64,127),(69,12),(128,255),(8,15))

def fixture():
    a = [75,52,80,40,65,56,70,30,166,3]
    b = [80,61,127,25,76,45,63,38,151,3]
    profiles = [list(a if i < 17 else b) for i in range(26)]
    for k,(mid,maximum) in enumerate(PAIRS):
        profiles[mid][k],profiles[maximum][k] = EXPECTED[k]
    return profiles

def trace_text(profiles):
    lines = ['R 352800 A20 FFFF']
    for i,p in enumerate(profiles):
        start = 44100*(10+4*i)
        voice = i % 30
        def word(frame,port,reg,value,v=voice):
            lines.extend((f'{frame} E22 {(reg<<5)|v:04X}', f'{frame} {port:03X} {value:04X}'))
        # Other voice and disabled/DMA words must not provide the answer.
        word(start,0xa22,4,0x7f01,(voice+1)%30)
        word(start,0xa20,5,0x0081)
        word(start,0xa22,0,0xf400)
        word(start,0xa22,4,0x4000|p[0])
        word(start,0xa20,5,p[1]|p[2]<<8)
        word(start,0xa22,6,0x4000|p[4])
        word(start,0xa20,7,p[5]|p[6]<<8)
        word(start,0xe20,1,p[8]<<8|0x15)
        word(start,0xa22,0,p[9]<<12|0x25)
        word(start+70000,0xa20,5,0x8000|p[3])
        word(start+70000,0xa20,7,0x8000|p[7])
    return '\n'.join(lines)+'\n'

class TraceDirectionTest(unittest.TestCase):
    def run_trace(self, text):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'trace.txt'
            path.write_text(text)
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                summarize_filter_envelope_trace(path)
            return out.getvalue()
    def test_valid(self):
        output = self.run_trace(trace_text(fixture()))
        self.assertIn('VOICE_TRACE_ALL_TEN_FIELD_GATES_PASS',output)
        self.assertEqual(output.count('VOICE_TRACE_FIELD ID='),10)
    def test_each_reversed(self):
        for i,(mid,maximum) in enumerate(PAIRS):
            p = fixture()
            p[mid][i],p[maximum][i] = p[maximum][i],p[mid][i]
            with self.subTest(id=1100+i), self.assertRaisesRegex(RuntimeError, f'VOICE_TRACE_DIRECTION_OR_VALUE ID={1100+i}'):
                self.run_trace(trace_text(p))
    def test_each_cross_talk(self):
        for i,(mid,_) in enumerate(PAIRS):
            p=fixture();p[mid][(i+1)%10] ^= 1
            with self.subTest(id=1100+i), self.assertRaisesRegex(RuntimeError,f'VOICE_TRACE_CROSSTALK ID={1100+i}'):
                self.run_trace(trace_text(p))
    def test_neutral_or_missing_trial(self):
        p=fixture();p[16][8]^=1
        with self.assertRaisesRegex(RuntimeError,'NEUTRAL_REGISTER_VARIATION'):
            self.run_trace(trace_text(p))
        with self.assertRaisesRegex(RuntimeError,'CASE_ALIGNMENT'):
            self.run_trace(trace_text(fixture()[:-1]))
    def test_missing_field(self):
        text=trace_text(fixture()).replace('441000 A22 404B\n','')
        self.assertNotEqual(text,trace_text(fixture()))
        with self.assertRaisesRegex(RuntimeError,'FIELD_MISSING'):
            self.run_trace(text)

if __name__ == '__main__':
    unittest.main()
