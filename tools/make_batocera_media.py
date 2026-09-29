"""Menu artwork for Batocera / EmulationStation (the R36S build ships none).

    python tools/make_batocera_media.py [out_dir]      (default: out/batocera/media)

Writes, named the way EmulationStation matches media to ports/BobrHopper.sh:
    BobrHopper-image.png    640x480  the title art over a dimmed in-game screenshot (the big picture in the list)
    BobrHopper-thumb.png    the title art alone, transparent (box / grid view)
    BobrHopper-marquee.png  the same art smaller, transparent (themes that show a logo)
Sources: assets_extra/images/title.png (beaver + wordmark + chicken) and docs/screenshot.png.
"""
import os
import sys

from PIL import Image, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TITLE = os.path.join(ROOT, "assets_extra", "images", "title.png")
SHOT = os.path.join(ROOT, "docs", "screenshot.png")
def trimmed(img):
    box = img.getchannel("A").getbbox()
    return img.crop(box) if box else img


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "out", "batocera", "media")
    os.makedirs(out, exist_ok=True)
    title = Image.open(TITLE).convert("RGBA")

    thumb = trimmed(title)
    thumb.save(os.path.join(out, "BobrHopper-thumb.png"), optimize=True)

    # the wordmark touches the beaver and the chicken, so it cannot be cut out cleanly: the logo is the whole art
    marquee = thumb.copy()
    marquee.thumbnail((400, 200), Image.LANCZOS)
    marquee.save(os.path.join(out, "BobrHopper-marquee.png"), optimize=True)

    bg = Image.open(SHOT).convert("RGB").resize((640, 480), Image.LANCZOS)
    bg = bg.filter(ImageFilter.GaussianBlur(9))
    bg = ImageEnhance.Brightness(bg).enhance(0.55).convert("RGBA")
    art = thumb.copy()
    art.thumbnail((600, 440), Image.LANCZOS)
    bg.alpha_composite(art, ((640 - art.width) // 2, (480 - art.height) // 2))
    bg.convert("RGB").save(os.path.join(out, "BobrHopper-image.png"), optimize=True)

    for name in ("BobrHopper-image.png", "BobrHopper-thumb.png", "BobrHopper-marquee.png"):
        im = Image.open(os.path.join(out, name))
        print(f"media {name:24s} {im.width}x{im.height}")


if __name__ == "__main__":
    main()
