#!/usr/bin/env python3
"""Draws the Mac disk image background (installer/mac_dmg/background.png and background@2x.png), the same layout
as in OLAAF: icon, name and subtitle on top, numbered steps below, white, IBM Plex Sans.
Usage: python3 tools/make_dmg_background.py <folder with IBMPlexSans-Regular.ttf and IBMPlexSans-Bold.ttf>"""
import sys, os
from PIL import Image, ImageDraw, ImageFont

NAME, SUB, DAW = "Binocular", "Correlation Meter", "your DAW"
BLUE, GREEN, LINE, TEXT, GREY = (76, 147, 220), (30, 158, 74), (226, 226, 226), (30, 30, 30), (90, 90, 90)
fonts = sys.argv[1] if len(sys.argv) > 1 else "."
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "installer", "mac_dmg")

def draw(s):                       # s = 1 or 2 (Retina)
    S = 4 * s                      # drawn 4x larger, then scaled down: smooth edges
    W, H = 640 * S, 420 * S
    im = Image.new("RGB", (W, H), "white"); d = ImageDraw.Draw(im)
    f = lambda w, px: ImageFont.truetype(os.path.join(fonts, f"IBMPlexSans-{w}.ttf"), int(px * S))
    u = lambda v: int(round(v * S))
    # the app icon: the Binocular mark (a blue and a green ring) on a dark rounded square
    d.rounded_rectangle((u(24), u(20), u(52), u(48)), u(6.5), fill=(32, 33, 37))
    for cx, col in ((33.6, (127, 190, 255)), (42.4, (52, 199, 89))):
        d.ellipse((u(cx - 6.2), u(34 - 6.2), u(cx + 6.2), u(34 + 6.2)), outline=col, width=u(2.1))
    # name, subtitle, rule
    tf, sf = f("Bold", 19.5), f("Regular", 13)
    d.text((u(62), u(34)), NAME, font=tf, fill=TEXT, anchor="lm")
    tw = d.textlength(NAME, font=tf)
    d.text((u(62) + tw + u(10), u(36)), SUB, font=sf, fill=GREY, anchor="lm")
    d.rectangle((u(24), u(62), u(616), u(62) + max(1, S // 2) - 1 + S // 2), fill=LINE)
    # numbered circles over Install and Blocked, a line before Uninstall
    nf = f("Bold", 12)
    for cx, n in ((150, "1"), (330, "2")):
        d.ellipse((u(cx - 11.25), u(89 - 11.25), u(cx + 11.25), u(89 + 11.25)), fill=BLUE)
        d.text((u(cx), u(89.5)), n, font=nf, fill="white", anchor="mm")
    d.rectangle((u(420), u(90), u(420) + S // 2 + S // 2 - 1, u(220)), fill=LINE)
    # the three steps
    stf, snf = f("Regular", 13), f("Bold", 10.5)
    steps = [(272, "1", [f"Double-click Install {NAME} and click through."]),
             (292, "2", ["macOS says it can’t be opened? Click Done, then double-click",
                         "Blocked? Open Privacy & Security, scroll down and click Open Anyway."]),
             (336, "3", [f"Double-click Install {NAME} again and choose Open."])]
    for y, n, lines in steps:
        d.ellipse((u(33 - 9.25), u(y - 9.25), u(33 + 9.25), u(y + 9.25)), fill=BLUE)
        d.text((u(33), u(y + 0.4)), n, font=snf, fill="white", anchor="mm")
        for i, t in enumerate(lines):
            d.text((u(52), u(y + i * 20)), t, font=stf, fill=TEXT, anchor="lm")
    d.text((u(24), u(394)), f"Quit {DAW} first. The installer is free and not signed by Apple, so macOS asks once.",
           font=f("Regular", 10.5), fill=GREY, anchor="lm")
    return im.resize((640 * s, 420 * s), Image.LANCZOS)

draw(1).save(os.path.join(out, "background.png"), optimize=True)
draw(2).save(os.path.join(out, "background@2x.png"), optimize=True)
print("written to", os.path.normpath(out))
