#!/usr/bin/env python3
"""Draw an original 256x256 icon for the Matrimelee recompilation.

No SNK artwork is used: a colourful field, a bubble, and block letters drawn
from a tiny bitmap font.
"""
import os
import struct
import zlib

W = H = 256
px = bytearray(W * H * 3)


def put(x, y, r, g, b):
    if 0 <= x < W and 0 <= y < H:
        i = (y * W + x) * 3
        px[i] = r
        px[i + 1] = g
        px[i + 2] = b


def rect(x0, y0, x1, y1, r, g, b):
    for y in range(max(0, y0), min(H, y1)):
        for x in range(max(0, x0), min(W, x1)):
            put(x, y, r, g, b)


# Background: deep red/orange gradient.
for y in range(H):
    for x in range(W):
        put(x, y, 70 + y // 4, 8 + y // 10, 16 + y // 16)

# Radiating wedges.
for y in range(H):
    for x in range(W):
        if 30 <= (x + y) % 170 <= 58:
            put(x, y, 235, 96 + (y % 50), 18)

# Ring.
cx, cy, rad = 128, 104, 62
for y in range(cy - rad - 6, cy + rad + 6):
    for x in range(cx - rad - 6, cx + rad + 6):
        d = (x - cx) ** 2 + (y - cy) ** 2
        if rad * rad <= d <= (rad + 5) ** 2:
            put(x, y, 255, 218, 54)

# Dark panel behind the lettering.
rect(10, 178, 246, 240, 8, 8, 18)
rect(12, 180, 244, 238, 24, 10, 18)

FONT = {
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
}


def draw_text(text, x, y, scale, r, g, b):
    cx = x
    for ch in text:
        glyph = FONT.get(ch)
        if glyph:
            for gy, row in enumerate(glyph):
                for gx, bit in enumerate(row):
                    if bit == "1":
                        rect(cx + gx * scale, y + gy * scale,
                             cx + (gx + 1) * scale, y + (gy + 1) * scale, r, g, b)
        cx += 6 * scale


draw_text("MATRIM", 18, 186, 4, 255, 226, 60)
draw_text("LEE", 150, 198, 3, 250, 250, 250)

raw = bytearray()
for y in range(H):
    raw.append(0)
    raw += px[y * W * 3:(y + 1) * W * 3]


def chunk(tag, data):
    return (struct.pack(">I", len(data)) + tag + data +
            struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))


out = b"\x89PNG\r\n\x1a\n"
out += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
out += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
out += chunk(b"IEND", b"")

path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "matrimelee.png")
with open(path, "wb") as f:
    f.write(out)
print("wrote", path, len(out), "bytes")
