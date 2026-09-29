#!/usr/bin/env python3
"""The Windows exe's icon: the beaver's head cropped from the title picture (assets_extra/images/title.png).

Usage: python tools/make_windows_icon.py [out.ico]    (default port/windows/bobrhopper.ico, used by port/windows/bobrhopper.rc)
"""
import os
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOX = (52, 46, 280, 274)  # the head with the cap, square, in title.png's 800x464 pixels


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "port", "windows", "bobrhopper.ico")
    im = Image.open(os.path.join(ROOT, "assets_extra", "images", "title.png")).convert("RGBA").crop(BOX)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    im.resize((256, 256), Image.LANCZOS).save(out, sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("icon:", out)


if __name__ == "__main__":
    main()
