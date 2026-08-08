#!/usr/bin/env python3
"""Pack Noto Color Emoji PNGs into a Flow32 flash atlas (RGB565 + 4bpp alpha).

Bakes at a single max size; firmware downscales for smaller text roles.
Source PNGs: googlefonts/noto-emoji (OFL-1.1).
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import urllib.request

from PIL import Image


# Top 50 face / reaction emoji (single codepoints; no ZWJ sequences yet).
DEFAULT_FACES = [
    (0x1F600, "grinning face"),
    (0x1F603, "grinning face with big eyes"),
    (0x1F604, "grinning face with smiling eyes"),
    (0x1F601, "beaming face with smiling eyes"),
    (0x1F606, "grinning squinting face"),
    (0x1F605, "grinning face with sweat"),
    (0x1F923, "rolling on the floor laughing"),
    (0x1F602, "face with tears of joy"),
    (0x1F642, "slightly smiling face"),
    (0x1F643, "upside-down face"),
    (0x1F609, "winking face"),
    (0x1F60A, "smiling face with smiling eyes"),
    (0x1F607, "smiling face with halo"),
    (0x1F970, "smiling face with hearts"),
    (0x1F60D, "smiling face with heart-eyes"),
    (0x1F929, "star-struck"),
    (0x1F618, "face blowing a kiss"),
    (0x1F617, "kissing face"),
    (0x1F61A, "kissing face with closed eyes"),
    (0x1F972, "smiling face with tear"),
    (0x1F60B, "face savoring food"),
    (0x1F61B, "face with tongue"),
    (0x1F61C, "winking face with tongue"),
    (0x1F92A, "zany face"),
    (0x1F61D, "squinting face with tongue"),
    (0x1F911, "money-mouth face"),
    (0x1F917, "hugging face"),
    (0x1F92D, "face with hand over mouth"),
    (0x1F92B, "shushing face"),
    (0x1F914, "thinking face"),
    (0x1F910, "zipper-mouth face"),
    (0x1F928, "face with raised eyebrow"),
    (0x1F610, "neutral face"),
    (0x1F611, "expressionless face"),
    (0x1F60F, "smirking face"),
    (0x1F612, "unamused face"),
    (0x1F644, "face with rolling eyes"),
    (0x1F62C, "grimacing face"),
    (0x1F60E, "smiling face with sunglasses"),
    (0x1F60C, "relieved face"),
    (0x1F614, "pensive face"),
    (0x1F62A, "sleepy face"),
    (0x1F634, "sleeping face"),
    (0x1F637, "face with medical mask"),
    (0x1F622, "crying face"),
    (0x1F62D, "loudly crying face"),
    (0x1F624, "face with steam from nose"),
    (0x1F621, "pouting face"),
    (0x1F92C, "face with symbols on mouth"),
    (0x1F97A, "pleading face"),
]

assert len(DEFAULT_FACES) == 50, len(DEFAULT_FACES)



NOTO_PNG_URL = (
    "https://raw.githubusercontent.com/googlefonts/noto-emoji/main/png/{px}/emoji_u{cp:x}.png"
)


def pack_4bpp(alphas: list[int]) -> bytes:
    out = bytearray()
    for i in range(0, len(alphas), 2):
        a0 = alphas[i] >> 4
        a1 = (alphas[i + 1] >> 4) if i + 1 < len(alphas) else 0
        out.append((a0 << 4) | a1)
    return bytes(out)


def rgb888_to_565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def download_png(cp: int, src_px: int, dest: str) -> None:
    url = NOTO_PNG_URL.format(px=src_px, cp=cp)
    print(f"  fetch {url}")
    urllib.request.urlretrieve(url, dest)


def render_glyph(path: str, baked: int):
    img = Image.open(path).convert("RGBA")
    # Fit into baked×baked preserving aspect, centered.
    img.thumbnail((baked, baked), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (baked, baked), (0, 0, 0, 0))
    ox = (baked - img.width) // 2
    oy = (baked - img.height) // 2
    canvas.paste(img, (ox, oy), img)

    pixels = []
    alphas = []
    for y in range(baked):
        for x in range(baked):
            r, g, b, a = canvas.getpixel((x, y))
            pixels.append(rgb888_to_565(r, g, b))
            alphas.append(a)

    # Baseline: sit emoji so bottom padding aligns with text baseline-ish.
    # yOffset from baseline to top of bitmap (negative = above baseline).
    y_offset = -(baked * 7 // 8)
    return {
        "w": baked,
        "h": baked,
        "xAdvance": baked,
        "xOffset": 0,
        "yOffset": y_offset,
        "pixels": pixels,
        "alpha": pack_4bpp(alphas),
    }


def c_ident(name: str) -> str:
    return "".join(c if c.isalnum() else "_" for c in name)


def generate(out_path: str, baked: int, src_px: int, cache_dir: str, faces=None):
    faces = faces or DEFAULT_FACES
    os.makedirs(cache_dir, exist_ok=True)

    glyphs = []
    pixel_blob = []
    alpha_blob = bytearray()

    for cp, label in faces:
        cache = os.path.join(cache_dir, f"emoji_u{cp:x}_{src_px}.png")
        if not os.path.isfile(cache):
            download_png(cp, src_px, cache)
        g = render_glyph(cache, baked)
        pix_off = len(pixel_blob)
        alpha_off = len(alpha_blob)
        pixel_blob.extend(g["pixels"])
        alpha_blob.extend(g["alpha"])
        glyphs.append({**g, "cp": cp, "label": label, "pix_off": pix_off, "alpha_off": alpha_off})
        print(f"  packed U+{cp:04X} {label}")

    symbol = "NotoColorEmoji"
    lines = []
    lines.append(f"// Auto-generated Noto Color Emoji atlas @ {baked}px (OFL-1.1).")
    lines.append("// Source: https://github.com/googlefonts/noto-emoji")
    lines.append("// Firmware downscales; never upscales past bakedSize.")
    lines.append("#pragma once")
    lines.append('#include "ColorEmoji.h"')
    lines.append("")
    lines.append(f"static const uint16_t {symbol}Pixels[] PROGMEM = {{")
    for i in range(0, len(pixel_blob), 12):
        chunk = pixel_blob[i : i + 12]
        lines.append("  " + ", ".join(f"0x{p:04X}" for p in chunk) + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"static const uint8_t {symbol}Alpha[] PROGMEM = {{")
    for i in range(0, len(alpha_blob), 16):
        chunk = alpha_blob[i : i + 16]
        lines.append("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"static const ColorEmojiGlyph {symbol}Glyphs[] PROGMEM = {{")
    for g in glyphs:
        lines.append(
            "  {{0x{cp:05X}, {po}, {ao}, {w}, {h}, {adv}, {xo}, {yo}}}, // {label}".format(
                cp=g["cp"],
                po=g["pix_off"],
                ao=g["alpha_off"],
                w=g["w"],
                h=g["h"],
                adv=g["xAdvance"],
                xo=g["xOffset"],
                yo=g["yOffset"],
                label=g["label"],
            )
        )
    lines.append("};")
    lines.append("")
    lines.append(f"static const ColorEmojiAtlas {symbol}Atlas = {{")
    lines.append(f"  {symbol}Pixels,")
    lines.append(f"  {symbol}Alpha,")
    lines.append(f"  {symbol}Glyphs,")
    lines.append(f"  {len(glyphs)},")
    lines.append(f"  {baked},")
    lines.append("};")
    lines.append("")
    lines.append(f"inline const ColorEmojiAtlas &NotoColorEmoji() {{ return {symbol}Atlas; }}")
    lines.append("")

    with open(out_path, "w") as f:
        f.write("\n".join(lines))

    pix_bytes = len(pixel_blob) * 2
    print(
        f"wrote {out_path}: {len(glyphs)} glyphs, "
        f"{pix_bytes} px bytes + {len(alpha_blob)} alpha bytes, baked={baked}"
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("--baked", type=int, default=40, help="max face size in px")
    ap.add_argument("--src-px", type=int, default=72, help="Noto PNG strike to download")
    ap.add_argument("--cache", default=None, help="PNG cache directory")
    args = ap.parse_args()
    cache = args.cache or os.path.join(os.path.dirname(__file__), ".emoji_cache")
    generate(args.output, args.baked, args.src_px, cache)


if __name__ == "__main__":
    main()
