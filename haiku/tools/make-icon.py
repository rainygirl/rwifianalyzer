#!/usr/bin/env python3
"""Draws icons/icon-large.raw and icons/icon-mini.raw, the app's B_CMAP8 icons.

    python3 tools/make-icon.py

Needs Pillow (`pip install pillow`), and runs anywhere - not only on Haiku.

Haiku icons are normally HVIF vector data out of Icon-O-Matic. Nothing can generate
HVIF from a script, so WifiAnalyzer.rdef imports these 32x32 and 16x16 bitmaps into
BEOS:L:STD_ICON and BEOS:M:STD_ICON instead, which is what BAppFileInfo falls back
to when a file has no vector icon. See AGENTS.md.

The glyph is the one already in ../winphone/Assets - three arcs and a dot in
Material green - with the proportions measured off Square150x150Logo.png, so all
three ports look like the same app. ../mac/tools/make-icon.py draws the same glyph
in the format macOS wants; the geometry constants below are the ones to keep in
step if the glyph ever changes.
"""

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

GLYPH_TOP = ARC_RADII[0] + ARC_HALF_WIDTH     # 0.517 above the dot centre
GLYPH_HEIGHT = GLYPH_TOP + DOT_RADIUS         # 0.567 tall
GLYPH_WIDTH = 2 * ARC_RADII[0] * 0.848        # 0.814 wide at +-58 degrees

SUPERSAMPLE = 8

# Haiku's 8-bit system palette (headers/private/interface/Palette.h), RGB only.
# Index 255 is B_TRANSPARENT_MAGIC_CMAP8 and is never a colour match.
PALETTE_HEX = (
    "000000080808101010181818202020282828303030383838404040484848505050585858606060686868707070787878"
    "808080888888909090989898a0a0a0a8a8a8b0b0b0b8b8b8c0c0c0c8c8c8d0d0d0d8d8d8e0e0e0e8e8e8f0f0f0f8f8f8"
    "0000ff0000e50000cc0000b300009a00008100006900005000003700001eff0000e40000cb0000b20000990000800000"
    "6900005000003700001e000000ff0000e40000cb0000b200009900008000006900005000003700001e00009833ffffff"
    "cbffffcbffcbcbff98cbff66cbff33cbff0098ffff98ffcb98ff9898ff6698ff3398ff0066ffff66ffcb66ff9866ff66"
    "66ff3366ff0033ffff33ffcb33ff9833ff6633ff3333ff00ff98ffff98cbff9898ff9866ff9833ff98000066ff0066cb"
    "cbcbffcbcbcbcbcb98cbcb66cbcb33cbcb0098cbff98cbcb98cb9898cb6698cb3398cb0066cbff66cbcb66cb9866cb66"
    "66cb3366cb0033cbff33cbcb33cb9833cb6633cb3333cb00ff66ffff66cbff6698ff6666ff6633ff6600006698006666"
    "cb98ffcb98cbcb9898cb9866cb9833cb98009898ff9898cb9898989898669898339898006698ff6698cb669898669866"
    "6698336698003398ff3398cb339898339866339833339800e68600ff33cbff3398ff3366ff3333ff3300006633006600"
    "cb66ffcb66cbcb6698cb6666cb6633cb66009866ff9866cb9866989866669866339866006666ff6666cb666698666666"
    "6666336666003366ff3366cb336698336666336633336600ff00ffff00cbff0098ff0066ff0033ffaf130033ff0033cb"
    "cb33ffcb33cbcb3398cb3366cb3333cb33009833ff9833cb9833989833669833339833006633ff6633cb663398663366"
    "6633336633003333ff3333cb333398333366333333333300ffcb66ffcb98ffcbcbffcbff003398003366003333003300"
    "cb00ffcb00cbcb0098cb0066cb0033ffe3469800ff9800cb9800989800669800339800006600ff6600cb660098660066"
    "6600336600003300ff3300cb330098330066330033330000ffcb33ffcb00ffff00ffff33ffff66ffff98ffffcbffffff"
)
TRANSPARENT_INDEX = 255


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


def icon(size):
    """Glyph only, edge to edge - Haiku icons carry no background plate."""
    big = size * SUPERSAMPLE
    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))

    scale = big * 0.88 / GLYPH_WIDTH
    dot_y = big / 2 + (GLYPH_TOP - GLYPH_HEIGHT / 2) * scale

    # Below ~24 px the innermost arc and the dot merge into a blob.
    draw_glyph(image, scale, big / 2, dot_y, GREEN, arcs=3 if size >= 24 else 2)

    return image.resize((size, size), Image.LANCZOS)


def palette():
    data = bytes.fromhex(PALETTE_HEX)
    return [tuple(data[i * 3:i * 3 + 3]) for i in range(256)]


def nearest_index(colour, colours):
    best, best_distance = 0, None
    for index in range(255):  # 255 is the transparent index, never a match
        r, g, b = colours[index]
        distance = (r - colour[0]) ** 2 + (g - colour[1]) ** 2 + (b - colour[2]) ** 2
        if best_distance is None or distance < best_distance:
            best, best_distance = index, distance
    return best


def to_cmap8(image, colours):
    """CMAP8 has no alpha channel, so the anti-aliased edge is quantised into a
    lighter shade of the glyph colour and everything faint becomes transparent."""
    solid = nearest_index(GREEN[:3], colours)
    edge = nearest_index(tuple(round(c + (255 - c) * 0.45) for c in GREEN[:3]),
                         colours)

    out = bytearray()
    for alpha in image.getchannel("A").tobytes():
        if alpha >= 165:
            out.append(solid)
        elif alpha >= 70:
            out.append(edge)
        else:
            out.append(TRANSPARENT_INDEX)
    return bytes(out)


if __name__ == "__main__":
    colours = palette()
    (HERE / "icons").mkdir(exist_ok=True)

    for size, name in ((32, "icon-large.raw"), (16, "icon-mini.raw")):
        path = HERE / "icons" / name
        path.write_bytes(to_cmap8(icon(size), colours))
        print(f"{path.relative_to(HERE)}  ({path.stat().st_size} bytes)")
