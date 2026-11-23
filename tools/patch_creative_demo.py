#!/usr/bin/env python3
"""Patch a PRIVATE copy of Creative's DEMO.C to poll the AWE64 VST3 mailbox.

No copyrighted Creative code is stored in this repository. The resulting
source must be compiled against the owner's legitimately held original DOS
Creative SF2LIB; redistribution requires separate rights review.

This patch intentionally does NOT alter original SF2 loading, preset mapping,
the EMU8000 driver or sound-generation code. Only inserts our host message
poller in the original Creative DEMO input loop.
"""
from pathlib import Path
import argparse

def patch_creative_demo(text: str) -> str:
    if "AWE64_GuestPoll();" in text or '"AWE64_BRIDGE.H"' in text:
        raise ValueError("Creative demo already patched; source must be pristine")
    include = '#include "ctaweapi.h"'
    loop = "    loop:\n        if (kbhit())"
    if text.count(include) != 1 or text.count(loop) != 1:
        raise ValueError("Unexpected Creative DEMO.C revision; refuse unsafe patch")
    return text.replace(include, include+'\n#include "AWE64_BRIDGE.H"').replace(
        loop, "    loop:\n        AWE64_GuestPoll();\n        if (kbhit())"
    )

if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("original",type=Path,help="Private original Creative DOS/DEMO/DEMO.C")
    parser.add_argument("output",type=Path,help="Private generated DEMO.C")
    options=parser.parse_args()
    if options.original.resolve()==options.output.resolve():
        parser.error("Never overwrite original Creative source")
    contents=options.original.read_bytes()
    try:
        decoded=contents.decode("latin-1")
        result=patch_creative_demo(decoded)
    except ValueError as exc:
        parser.error(str(exc))
    options.output.parent.mkdir(parents=True,exist_ok=True)
    options.output.write_bytes(result.encode("latin-1"))
    print("Private Creative DEMO.C patched; unchanged original kept separately")
