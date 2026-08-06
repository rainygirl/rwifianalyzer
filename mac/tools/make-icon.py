#!/usr/bin/env python3
"""Draws Resources/AppIcon.icns: the Wi-Fi glyph on a dark rounded square.

    python3 tools/make-icon.py

Needs Pillow (`pip install pillow`) and macOS's iconutil.

The glyph is the one already in ../winphone/Assets - three arcs and a dot in
Material green - with the proportions measured off Square150x150Logo.png, so all
three ports look like the same app. ../haiku/tools/make-icon.py draws the same
glyph in the format Haiku wants; the geometry constants below are the ones to keep
in step if the glyph ever changes.
"""

import shutil
import subprocess
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("Pillow is required: pip install pillow")

HERE = Path(__file__).resolve().parent.parent

GREEN = (76, 175, 80, 255)

# Glyph geometry as fractions of S, the glyph scale. From the 150x150 tile: radii
# 72/46/20.5 px, stroke half-width 5.5, dot radius 7.5, fan +-58 degrees.
ARC_RADII = (0.48, 0.307, 0.137)
ARC_HALF_WIDTH = 0.037
DOT_RADIUS = 0.05
FAN_DEGREES = 58

# Where the glyph's bounding box sits relative to the dot centre, again in S.
GLYPH_TOP = ARC_RADII[0] + ARC_HALF_WIDTH     # 0.517 above the dot centre
GLYPH_HEIGHT = GLYPH_TOP + DOT_RADIUS         # 0.567 tall
GLYPH_WIDTH = 2 * ARC_RADII[0] * 0.848        # 0.814 wide at +-58 degrees

SUPERSAMPLE = 8


def draw_glyph(image, scale, centre_x, dot_y, colour, arcs=3):
    """Draw the fan and dot onto `image`, in pixels."""
    draw = ImageDraw.Draw(image)

    half_width = max(ARC_HALF_WIDTH * scale, 1.0)
    stroke = max(int(round(half_width * 2)), 1)

    for radius in ARC_RADII[:arcs]:
        r = radius * scale
        draw.arc((centre_x - r, dot_y - r, centre_x + r, dot_y + r),
                 start=270 - FAN_DEGREES, end=270 + FAN_DEGREES,
                 fill=colour, width=stroke)

    dot = max(DOT_RADIUS * scale, half_width)
    draw.ellipse((centre_x - dot, dot_y - dot, centre_x + dot, dot_y + dot),
                 fill=colour)


def glyph_layout(size, coverage):
    """Scale and dot position that centre a glyph `coverage` wide in `size`."""
    scale = size * coverage / GLYPH_WIDTH
    dot_y = size / 2 + (GLYPH_TOP - GLYPH_HEIGHT / 2) * scale
    return scale, size / 2, dot_y


def gradient(size, top, bottom):
    strip = Image.new("RGB", (1, size))
    for y in range(size):
        t = y / max(size - 1, 1)
        strip.putpixel((0, y), tuple(
            round(a + (b - a) * t) for a, b in zip(top, bottom)))
    return strip.resize((size, size), Image.NEAREST)


def icon(size):
    """A dark rounded square with the glyph on it, the way a Dock icon looks."""
    big = size * SUPERSAMPLE

    # macOS leaves about 10% of the canvas empty around the rounded square.
    inset = big * 0.098
    radius = (big - 2 * inset) * 0.225

    mask = Image.new("L", (big, big), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (inset, inset, big - inset - 1, big - inset - 1), radius=radius, fill=255)

    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    image.paste(gradient(big, (0x26, 0x30, 0x28), (0x0D, 0x10, 0x0E)), (0, 0), mask)

    scale, centre_x, dot_y = glyph_layout(big, 0.60)

    # Below ~24 px the innermost arc and the dot merge into a blob.
    draw_glyph(image, scale, centre_x, dot_y, GREEN, arcs=3 if size >= 24 else 2)

    return image.resize((size, size), Image.LANCZOS)


if __name__ == "__main__":
    iconset = HERE / ".build" / "AppIcon.iconset"
    if iconset.exists():
        shutil.rmtree(iconset)
    iconset.mkdir(parents=True)

    for base in (16, 32, 128, 256, 512):
        for factor, suffix in ((1, ""), (2, "@2x")):
            icon(base * factor).save(iconset / f"icon_{base}x{base}{suffix}.png")

    icns = HERE / "Resources" / "AppIcon.icns"
    if shutil.which("iconutil") is None:
        sys.exit(f"iconutil not found - the .iconset is at {iconset}")

    subprocess.run(["iconutil", "-c", "icns", str(iconset), "-o", str(icns)],
                   check=True)
    print(f"{icns.relative_to(HERE)}  ({icns.stat().st_size} bytes)")
