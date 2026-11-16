#!/usr/bin/env python3
"""Frog tray icons v2 — 16px-first redesign.

Identical head silhouette / framing / palette / stroke weights across states.
State signatures that survive 16px rasterization:
  sleep   : NO white in eye bumps, thick dark curved closed lids
  awake   : big round white eyes (near-full bump) + deep wide smile
  focused : heavy angled lid bar across bump top, half-height white below,
            lowered pupil, small straight mouth
"""
import math
import os
import subprocess

OUT = os.path.dirname(os.path.abspath(__file__))
ICON_DIR = os.path.join(OUT, "icons")
os.makedirs(ICON_DIR, exist_ok=True)

GREEN, DARK, WHITE = "#4CAF50", "#1B5E20", "#FFFFFF"
STROKE, FEAT = 5.0, 5.0

# head geometry (100x100 design cell) — fills ~88% of canvas for tray use
EX, EY, RX, RY = 50.0, 64.0, 42.0, 27.0
LCX, LCY, R = 28.0, 26.0, 18.0
RCX = 72.0
EYE_R, PUPIL_R = 12.0, 5.5


def circle_ellipse_intersections(cx, cy, r):
    def f(t):
        x, y = cx + r * math.cos(t), cy + r * math.sin(t)
        return ((x - EX) / RX) ** 2 + ((y - EY) / RY) ** 2 - 1.0

    def bisect(a, b):
        fa = f(a)
        for _ in range(80):
            m = (a + b) / 2
            fm = f(m)
            if fa * fm <= 0:
                b = m
            else:
                a, fa = m, fm
        return (a + b) / 2

    pt = lambda t: (cx + r * math.cos(t), cy + r * math.sin(t))
    return pt(bisect(math.pi / 2, math.pi)), pt(bisect(0.0, math.pi / 2))


fmt = lambda p: f"{p[0]:.3f},{p[1]:.3f}"
outer_L, inner_L = circle_ellipse_intersections(LCX, LCY, R)
mirror = lambda p: (100.0 - p[0], p[1])
outer_R, inner_R = mirror(outer_L), mirror(inner_L)
left_pt = (EX - RX, EY)

HEAD_PATH = (
    f"M {fmt(left_pt)} "
    f"A {RX},{RY} 0 0 1 {fmt(outer_L)} "
    f"A {R},{R} 0 1 1 {fmt(inner_L)} "
    f"A {RX},{RY} 0 0 1 {fmt(inner_R)} "
    f"A {R},{R} 0 1 1 {fmt(outer_R)} "
    f"A {RX},{RY} 0 1 1 {fmt(left_pt)} Z"
)
HEAD = (f'<path d="{HEAD_PATH}" fill="{GREEN}" stroke="{DARK}" '
        f'stroke-width="{STROKE}" stroke-linejoin="round"/>')
NOSTRILS = (f'<circle cx="45" cy="52" r="2" fill="{DARK}"/>'
            f'<circle cx="55" cy="52" r="2" fill="{DARK}"/>')


def line(d, w=FEAT):
    return (f'<path d="{d}" fill="none" stroke="{DARK}" '
            f'stroke-width="{w}" stroke-linecap="round"/>')


def open_eye(cx):
    return (f'<circle cx="{cx}" cy="26" r="{EYE_R}" fill="{WHITE}"/>'
            f'<circle cx="{cx}" cy="26" r="{PUPIL_R}" fill="{DARK}"/>')


# focused lid: gentle 9.9deg slope, inner end lower (determined, not angry)
LID_SLOPE = 0.175
def lid_y_left(x):  return 21.0 + (x - 16.0) * LID_SLOPE
def lid_y_right(x): return 21.0 + (84.0 - x) * LID_SLOPE

def focused_eye(cx, side):
    yf = lid_y_left if side == "L" else lid_y_right
    clip = (f'<clipPath id="lid{side}"><path d="M -20,{yf(-20):.2f} '
            f'L 120,{yf(120):.2f} L 120,120 L -20,120 Z"/></clipPath>')
    white = (f'<circle cx="{cx}" cy="26" r="{EYE_R}" fill="{WHITE}" '
             f'clip-path="url(#lid{side})"/>')
    pupil = f'<circle cx="{cx}" cy="33.5" r="3.0" fill="{DARK}"/>'
    x0, x1 = (16, 40) if side == "L" else (84, 60)
    bar = line(f"M {x0},{yf(x0):.2f} L {x1},{yf(x1):.2f}")
    return clip + white + pupil + bar


SLEEP = (
    line("M 17,26 Q 28,36 39,26") +
    line("M 61,26 Q 72,36 83,26") +
    line("M 42,70 Q 50,75.5 58,70")
)
HAPPY = open_eye(28) + open_eye(72) + line("M 32,66 Q 50,82 68,66")
FOCUSED = (focused_eye(28, "L") + focused_eye(72, "R") +
           line("M 43,70 L 57,70"))

STATES = [("sleep", SLEEP), ("awake", HAPPY), ("focused", FOCUSED)]


def icon_svg(feats):
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">'
            '<g transform="translate(0,0.5)">' + HEAD + NOSTRILS + feats +
            '</g></svg>')


def raster(svg_path, png_path, w, h=None):
    cmd = ["rsvg-convert", "-w", str(w)]
    if h:
        cmd += ["-h", str(h)]
    subprocess.run(cmd + [svg_path, "-o", png_path], check=True)


for name, feats in STATES:
    p = os.path.join(ICON_DIR, f"{name}.svg")
    with open(p, "w") as f:
        f.write(icon_svg(feats))
    for s in (16, 24, 32, 48, 64, 128, 256, 512):
        raster(p, os.path.join(ICON_DIR, f"{name}-{s}.png"), s, s)

CELL, GAP, MARGIN = 360, 120, 90
W, H = MARGIN * 2 + CELL * 3 + GAP * 2, MARGIN * 2 + CELL
S = CELL / 100.0
parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'viewBox="0 0 {W} {H}">']
for i, (name, feats) in enumerate(STATES):
    x = MARGIN + i * (CELL + GAP)
    parts.append(f'<g transform="translate({x},{MARGIN}) scale({S}) '
                 f'translate(0,0.5)">' + HEAD + NOSTRILS + feats + '</g>')
parts.append('</svg>')
row_svg = os.path.join(OUT, "frog-tray-icons.svg")
with open(row_svg, "w") as f:
    f.write("".join(parts))
raster(row_svg, os.path.join(OUT, "frog-tray-icons.png"), W, H)

# ---------------- readability proof sheet ----------------
from PIL import Image

LIGHT, DARKBG = (236, 239, 241, 255), (38, 50, 56, 255)
COLW, PAD = 170, 26
ZOOM = 4
band_h = PAD + 16 + 22 + 24 + 22 + 32 + 26 + 32 * ZOOM + PAD
sheet = Image.new("RGBA", (COLW * 3 + PAD * 2, band_h * 2), LIGHT)
for band, bg in enumerate((LIGHT, DARKBG)):
    y0 = band * band_h
    if band:
        sheet.paste(Image.new("RGBA", (sheet.width, band_h), bg), (0, y0))
    y = y0 + PAD
    for s in (16, 24, 32):
        for i, (name, _) in enumerate(STATES):
            im = Image.open(os.path.join(ICON_DIR, f"{name}-{s}.png"))
            cx = PAD + i * COLW + COLW // 2
            sheet.alpha_composite(im, (cx - s // 2, y + (32 - s) // 2))
        y += s + 22
    y += 4
    for i, (name, _) in enumerate(STATES):
        im = Image.open(os.path.join(ICON_DIR, f"{name}-32.png"))
        im = im.resize((32 * ZOOM, 32 * ZOOM), Image.NEAREST)
        cx = PAD + i * COLW + COLW // 2
        sheet.alpha_composite(im, (cx - 16 * ZOOM, y))
sheet.save(os.path.join(OUT, "readability-check.png"))
print("done", W, H, sheet.size)
