#!/usr/bin/env python3
"""Builds src/platform/app.ico: a friend standing on a pixel meadow in a rounded square.
Usage: python3 tools/make_icon.py <path to gf_dump_sprite> [speciesId]"""
import subprocess, sys
from PIL import Image, ImageDraw

exe = sys.argv[1]
sid = sys.argv[2] if len(sys.argv) > 2 else "0"
spr = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
for line in subprocess.check_output([exe, sid, "0"], text=True).split("\n"):
    if line.strip():
        x, y, r, g, b, a = map(int, line.split())
        spr.putpixel((x, y), (r, g, b, a))

def build(n):
    S = 4  # supersample for the rounded corners, sprite stays pixel-crisp
    big = n * S
    bg = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    d = ImageDraw.Draw(bg)
    # sky gradient
    for y in range(big):
        t = y / big
        d.line([(0, y), (big, y)], fill=(int(110 + 60 * t), int(190 + 40 * t), int(235 + 10 * t), 255))
    # meadow with a lighter top edge
    gy = int(big * 0.70)
    d.rectangle([0, gy, big, big], fill=(70, 160, 90, 255))
    d.rectangle([0, gy, big, gy + max(S, big // 40)], fill=(120, 205, 120, 255))
    d.rectangle([0, int(big * 0.86), big, big], fill=(46, 125, 79, 255))
    mask = Image.new("L", (big, big), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, big - 1, big - 1], radius=int(big * 0.22), fill=255)
    out = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    out.paste(bg, (0, 0), mask)
    # sprite: nearest-neighbour at an integer scale where possible
    target = int(big * 0.74)
    sc = max(1, target // 24)
    sp = spr.resize((24 * sc, 24 * sc), Image.NEAREST) if big >= 24 * 2 * S else spr.resize((target, target), Image.LANCZOS)
    sh = Image.new("RGBA", sp.size, (0, 0, 0, 0))
    sx = (big - sp.width) // 2
    sy = int(big * 0.80) - sp.height + sp.height // 12
    # soft shadow under the feet
    ImageDraw.Draw(out).ellipse([sx + sp.width * 0.2, sy + sp.height * 0.86, sx + sp.width * 0.8, sy + sp.height * 0.98], fill=(0, 0, 0, 70))
    out.alpha_composite(sp, (sx, sy))
    return out.resize((n, n), Image.LANCZOS)

sizes = [16, 24, 32, 48, 64, 128, 256]
imgs = [build(n) for n in sizes]
imgs[-1].save("src/platform/app.ico", format="ICO", sizes=[(n, n) for n in sizes], append_images=imgs[:-1])
imgs[-1].save("/tmp/icon_preview.png")
print("wrote src/platform/app.ico")
