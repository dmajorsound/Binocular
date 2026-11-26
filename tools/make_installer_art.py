#!/usr/bin/env python3
"""Draws the Binocular icon art for the installers (as in Ahh 1.1.1):
installer/mac_ui/icon.png (256), installer/win/welcome-icon-100/200.bmp (72/144),
small-100/200.bmp (55x58 / 110x116, icon top right of the following pages), wizard-100/200.bmp (164x314 / 328x628,
icon in the middle, final page). All on white except the Mac icon (transparent)."""
import os
from PIL import Image, ImageDraw

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "installer")
BG, BLUE, GREEN = (32, 33, 37), (127, 190, 255), (52, 199, 89)

def icon(px):
    S = 4; n = px * S
    im = Image.new("RGBA", (n, n), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.rounded_rectangle((0, 0, n - 1, n - 1), round(n * 0.225), fill=BG)
    r, w, cy = n * 0.205, max(1, round(n * 0.068)), n / 2
    for cx, col in ((n * 0.40, BLUE), (n * 0.60, GREEN)):   # the Binocular mark: two overlapping rings
        d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=col, width=w)
    return im.resize((px, px), Image.LANCZOS)

def on_white(size, ipx, pos=None):
    im = Image.new("RGB", size, "white"); ic = icon(ipx)
    if pos is None: pos = ((size[0] - ipx) // 2, (size[1] - ipx) // 2)
    im.paste(ic, pos, ic); return im

icon(256).save(os.path.join(ROOT, "mac_ui", "icon.png"))
w = os.path.join(ROOT, "win")
for s in (1, 2):
    on_white((72 * s, 72 * s), 72 * s, (0, 0)).save(os.path.join(w, f"welcome-icon-{100 * s}.bmp"))
    on_white((55 * s, 58 * s), 46 * s, (round(4.5 * s), 6 * s)).save(os.path.join(w, f"small-{100 * s}.bmp"))
    on_white((164 * s, 314 * s), 84 * s, (40 * s, 115 * s)).save(os.path.join(w, f"wizard-{100 * s}.bmp"))
print("written")
