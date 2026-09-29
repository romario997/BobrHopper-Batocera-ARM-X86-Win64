"""Menu artwork for Batocera PC (x86_64) / EmulationStation at 1920x1080 (16:9).

    python tools/make_batocera_x86_media.py [out_dir] [--bg <screenshot.png>]   (default: out/batocera-x86/media)

Writes, named the way EmulationStation matches media to ports/BobrHopper.sh:
    BobrHopper-image.png    1920x1080  the title art over a dimmed, blurred in-game screenshot (the big picture)
    BobrHopper-thumb.png    the title art alone, transparent (box / grid view)
    BobrHopper-marquee.png  the same art smaller, transparent (themes that show a logo)
Sources: assets_extra/images/title.png (beaver + wordmark + chicken) and docs/screenshot_1080.png (or --bg: any in-game
screenshot, e.g. one taken at 1920x1080 with bobrhopper --hidden --size 1920x1080 --shots ...). The background is
cropped to 16:9 (cover), never stretched. Adapted from the ARM port's tools/make_batocera_media.py (640x480).
"""
import os
import sys

from PIL import Image, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TITLE = os.path.join(ROOT, "assets_extra", "images", "title.png")
SHOT = os.path.join(ROOT, "docs", "screenshot_1080.png")  # bobrhopper --hidden --size 1920x1080 --no-hud, seed 5
W, H = 1920, 1080


def trimmed(img):
    box = img.getchannel("A").getbbox()
    return img.crop(box) if box else img


def cover(img, w, h):
    """scale to fill w x h keeping the aspect ratio, then crop the middle"""
    s = max(w / img.width, h / img.height)
    img = img.resize((max(w, round(img.width * s)), max(h, round(img.height * s))), Image.LANCZOS)
    x, y = (img.width - w) // 2, (img.height - h) // 2
    return img.crop((x, y, x + w, y + h))


def main(argv):
    bg_path = SHOT
    if "--bg" in argv:
        at = argv.index("--bg")
        bg_path = argv[at + 1]
        argv = argv[:at] + argv[at + 2:]
    out = argv[0] if argv else os.path.join(ROOT, "out", "batocera-x86", "media")
    os.makedirs(out, exist_ok=True)
    title = Image.open(TITLE).convert("RGBA")

    thumb = trimmed(title)
    thumb.save(os.path.join(out, "BobrHopper-thumb.png"), optimize=True)

    # the wordmark touches the beaver and the chicken, so it cannot be cut out cleanly: the logo is the whole art
    marquee = thumb.copy()
    marquee.thumbnail((600, 300), Image.LANCZOS)
    marquee.save(os.path.join(out, "BobrHopper-marquee.png"), optimize=True)

    bg = cover(Image.open(bg_path).convert("RGB"), W, H)
    bg = bg.filter(ImageFilter.GaussianBlur(24))
    bg = ImageEnhance.Brightness(bg).enhance(0.55).convert("RGBA")
    art = thumb.copy()
    # the logo at 80% of the height like the 640x480 picture (440 of 480), upscaled from the 800x464 art if needed
    s = min(W * 0.8 / art.width, H * 0.8 / art.height)
    art = art.resize((round(art.width * s), round(art.height * s)), Image.LANCZOS)
    bg.alpha_composite(art, ((W - art.width) // 2, (H - art.height) // 2))
    bg.convert("RGB").save(os.path.join(out, "BobrHopper-image.png"), optimize=True)

    for name in ("BobrHopper-image.png", "BobrHopper-thumb.png", "BobrHopper-marquee.png"):
        im = Image.open(os.path.join(out, name))
        print(f"media {name:24s} {im.width}x{im.height}")


if __name__ == "__main__":
    main(sys.argv[1:])
