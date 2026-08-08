#!/usr/bin/env python3
"""Convert a TTF into a 4bpp grayscale anti-aliased font header for Flow32."""

from __future__ import annotations

import argparse
import os
import sys

from PIL import Image, ImageDraw, ImageFont


def pack_4bpp(alphas: list[int]) -> bytes:
    """Pack 8-bit alphas into 4bpp (high nibble first)."""
    out = bytearray()
    for i in range(0, len(alphas), 2):
        a0 = alphas[i] >> 4
        a1 = (alphas[i + 1] >> 4) if i + 1 < len(alphas) else 0
        out.append((a0 << 4) | a1)
    return bytes(out)


def render_glyph(font: ImageFont.FreeTypeFont, ch: str, pad: int = 1):
    ascent, descent = font.getmetrics()
    canvas_w = max(8, font.getlength(ch) + pad * 4 + 8)
    canvas_h = ascent + descent + pad * 4 + 8
    img = Image.new("L", (int(canvas_w) + 1, int(canvas_h) + 1), 0)
    draw = ImageDraw.Draw(img)
    baseline = ascent + pad
    draw.text((pad, pad), ch, font=font, fill=255)

    bbox = img.getbbox()
    if bbox is None:
        advance = max(1, int(round(font.getlength(ch))))
        return {
            "width": 0,
            "height": 0,
            "xAdvance": advance,
            "xOffset": 0,
            "yOffset": 0,
            "alphas": b"",
        }

    l, t, r, b = bbox
    cropped = img.crop((l, t, r, b))
    alphas = list(cropped.getdata())
    advance = max(1, int(round(font.getlength(ch))))
    y_offset = t - baseline
    x_offset = l - pad
    return {
        "width": cropped.width,
        "height": cropped.height,
        "xAdvance": advance,
        "xOffset": x_offset,
        "yOffset": y_offset,
        "alphas": pack_4bpp(alphas),
    }


def c_ident(name: str) -> str:
    out = []
    for c in name:
        if c.isalnum():
            out.append(c)
        else:
            out.append("_")
    return "".join(out)


def glyph_comment(code: int) -> str:
    if 32 < code < 127 and code not in (0x5C, 0x27):  # skip \ '
        return chr(code)
    return f"U+{code:04X}"


def generate(
    ttf: str,
    px: int,
    out_path: str,
    symbol: str | None = None,
    first: int = 0x20,
    last: int = 0xFF,
):
    font = ImageFont.truetype(ttf, px)
    ascent, descent = font.getmetrics()
    y_advance = ascent + descent + 2
    baseline = ascent

    glyphs = []
    bitmaps = bytearray()
    for code in range(first, last + 1):
        # Skip C1 controls — empty placeholders keep contiguous indexing.
        if 0x7F <= code <= 0x9F:
            g = {
                "width": 0,
                "height": 0,
                "xAdvance": 0,
                "xOffset": 0,
                "yOffset": 0,
                "alphas": b"",
            }
        else:
            g = render_glyph(font, chr(code))
        offset = len(bitmaps)
        bitmaps.extend(g["alphas"])
        glyphs.append({**g, "bitmapOffset": offset})

    if symbol is None:
        base = os.path.splitext(os.path.basename(ttf))[0]
        symbol = f"{c_ident(base)}{px}aa"

    lines = []
    lines.append(
        f"// Auto-generated 4bpp AA font from {os.path.basename(ttf)} @ {px}px"
    )
    lines.append(f"// Codepoints U+{first:04X}..U+{last:04X} (Latin-1 block)")
    lines.append("#pragma once")
    lines.append('#include "AAFont.h"')
    lines.append("")
    lines.append(f"const uint8_t {symbol}Bitmaps[] PROGMEM = {{")
    for i in range(0, len(bitmaps), 16):
        chunk = bitmaps[i : i + 16]
        hexes = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"  {hexes},")
    if not bitmaps:
        lines.append("  0x00,")
    lines.append("};")
    lines.append("")
    lines.append(f"const AAGlyph {symbol}Glyphs[] PROGMEM = {{")
    for code, g in zip(range(first, last + 1), glyphs):
        lines.append(
            "  {{{offset}, {w}, {h}, {adv}, {xo}, {yo}}}, // {ch}".format(
                offset=g["bitmapOffset"],
                w=g["width"],
                h=g["height"],
                adv=g["xAdvance"],
                xo=g["xOffset"],
                yo=g["yOffset"],
                ch=glyph_comment(code),
            )
        )
    lines.append("};")
    lines.append("")
    lines.append(f"const AAFont {symbol} PROGMEM = {{")
    lines.append(f"  {symbol}Bitmaps,")
    lines.append(f"  {symbol}Glyphs,")
    lines.append(f"  0x{first:02X}, 0x{last:02X},")
    lines.append(f"  {y_advance}, {baseline}")
    lines.append("};")
    lines.append("")

    max_off = max((g["bitmapOffset"] for g in glyphs), default=0)
    if max_off > 65535:
        raise SystemExit(
            f"bitmapOffset {max_off} exceeds uint16_t — widen AAGlyph.bitmapOffset"
        )

    with open(out_path, "w") as f:
        f.write("\n".join(lines))

    print(
        f"wrote {out_path}: {len(glyphs)} glyphs, {len(bitmaps)} bitmap bytes, "
        f"yAdvance={y_advance}, baseline={baseline}, maxOff={max_off}"
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ttf")
    ap.add_argument("size", type=int, help="pixel size")
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument("-n", "--name", help="C symbol base name")
    ap.add_argument("--first", type=lambda s: int(s, 0), default=0x20)
    ap.add_argument("--last", type=lambda s: int(s, 0), default=0xFF)
    args = ap.parse_args()
    generate(args.ttf, args.size, args.output, args.name, args.first, args.last)


if __name__ == "__main__":
    main()
