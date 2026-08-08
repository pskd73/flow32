#!/usr/bin/env python3
"""Pack Lucide SVGs into a Flow32 tintable icon atlas (4bpp alpha).

Output: SD card file (magic F32I) for IconSd. Icons are monochrome — firmware
tints with the current text color. Bake large (default 96) so UI downscales
stay sharp; avoid drawing above bakedSize.

Requires: Pillow, rsvg-convert (librsvg).

Examples:
  python3 tools/lucide_icon_atlas.py -o sd/flow32/icons.atlas --baked 96
  python3 tools/lucide_icon_atlas.py --all -o sd/flow32/icons.atlas --baked 96
  python3 tools/lucide_icon_atlas.py --icons-dir /path/to/lucide/icons -o ...
"""

from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

# Curated set for the Flow32 demo (Lucide names).
DEFAULT_ICONS = [
    "house",
    "flame",
    "droplet",
    "zap",
    "wifi",
    "bell",
    "heart",
    "star",
    "settings",
    "user",
    "search",
    "check",
    "x",
    "plus",
    "minus",
    "chevron-right",
    "arrow-right",
    "volume-2",
    "sun",
    "moon",
    "battery",
    "wifi-off",
    "circle-alert",
    "info",
    "menu",
    "eye",
    "lock",
    "bluetooth",
    "radio",
    "gauge",
]

LUCIDE_TAG = "0.469.0"
LUCIDE_TGZ_URL = (
    f"https://github.com/lucide-icons/lucide/archive/refs/tags/{LUCIDE_TAG}.tar.gz"
)


def pack_4bpp(alphas: list[int]) -> bytes:
    out = bytearray()
    for i in range(0, len(alphas), 2):
        a0 = alphas[i] >> 4
        a1 = (alphas[i + 1] >> 4) if i + 1 < len(alphas) else 0
        out.append((a0 << 4) | a1)
    return bytes(out)


def find_rsvg() -> str:
    path = shutil.which("rsvg-convert")
    if not path:
        raise SystemExit(
            "rsvg-convert not found (install librsvg). "
            "On macOS: brew install librsvg"
        )
    return path


def ensure_icons_dir(cache_root: str, icons_dir: str | None) -> str:
    if icons_dir:
        if not os.path.isdir(icons_dir):
            raise SystemExit(f"--icons-dir not a directory: {icons_dir}")
        return icons_dir

    dest = os.path.join(cache_root, f"lucide-{LUCIDE_TAG}", "icons")
    if os.path.isdir(dest) and any(n.endswith(".svg") for n in os.listdir(dest)):
        return dest

    os.makedirs(cache_root, exist_ok=True)
    tgz = os.path.join(cache_root, f"lucide-{LUCIDE_TAG}.tar.gz")
    if not os.path.isfile(tgz):
        print(f"fetch {LUCIDE_TGZ_URL}")
        urllib.request.urlretrieve(LUCIDE_TGZ_URL, tgz)
    print(f"extract {tgz}")
    with tarfile.open(tgz, "r:gz") as tar:
        tar.extractall(cache_root)
    if not os.path.isdir(dest):
        raise SystemExit(f"expected icons at {dest} after extract")
    return dest


def list_all_icons(icons_dir: str) -> list[str]:
    names = []
    for fn in os.listdir(icons_dir):
        if fn.endswith(".svg"):
            names.append(fn[:-4])
    names.sort()
    return names


def rasterize_svg(rsvg: str, svg_path: str, png_path: str, baked: int) -> None:
    # Transparent background; Lucide currentColor defaults to black → alpha mask.
    subprocess.run(
        [
            rsvg,
            f"--width={baked}",
            f"--height={baked}",
            "--keep-aspect-ratio",
            "-b",
            "none",
            "-o",
            png_path,
            svg_path,
        ],
        check=True,
        capture_output=True,
    )


def render_glyph(png_path: str, baked: int):
    from PIL import Image

    img = Image.open(png_path).convert("RGBA")
    if img.size != (baked, baked):
        canvas = Image.new("RGBA", (baked, baked), (0, 0, 0, 0))
        ox = (baked - img.width) // 2
        oy = (baked - img.height) // 2
        canvas.paste(img, (ox, oy), img)
        img = canvas

    alphas = []
    for y in range(baked):
        for x in range(baked):
            _r, _g, _b, a = img.getpixel((x, y))
            alphas.append(a)

    # Sit like emoji: most of the box above the text baseline.
    y_offset = -(baked * 7 // 8)
    return {
        "w": baked,
        "h": baked,
        "xAdvance": baked,
        "xOffset": 0,
        "yOffset": y_offset,
        "alpha": pack_4bpp(alphas),
    }


def build_glyphs(names: list[str], icons_dir: str, baked: int, png_cache: str):
    rsvg = find_rsvg()
    os.makedirs(png_cache, exist_ok=True)

    glyphs = []
    alpha_blob = bytearray()
    name_blob = bytearray()
    skipped = 0

    for idx, name in enumerate(names):
        svg = os.path.join(icons_dir, f"{name}.svg")
        if not os.path.isfile(svg):
            print(f"  skip {name}: missing svg")
            skipped += 1
            continue
        png = os.path.join(png_cache, f"{name}_{baked}.png")
        if not os.path.isfile(png):
            try:
                rasterize_svg(rsvg, svg, png, baked)
            except subprocess.CalledProcessError as e:
                err = (e.stderr or b"").decode("utf-8", errors="replace")
                print(f"  skip {name}: rsvg failed: {err.strip()}")
                skipped += 1
                continue
        try:
            g = render_glyph(png, baked)
        except Exception as e:
            print(f"  skip {name}: render: {e}")
            skipped += 1
            continue

        name_off = len(name_blob)
        name_blob.extend(name.encode("ascii"))
        name_blob.append(0)

        alpha_off = len(alpha_blob)
        alpha_blob.extend(g["alpha"])
        glyphs.append(
            {
                **g,
                "id": idx,
                "name": name,
                "name_off": name_off,
                "alpha_off": alpha_off,
            }
        )
        print(f"  packed [{idx}] {name}")

    if skipped:
        print(f"  skipped {skipped}")
    # Re-assign contiguous ids after skips.
    for i, g in enumerate(glyphs):
        g["id"] = i
    if len(glyphs) > 0xFFFF:
        raise SystemExit(f"too many glyphs: {len(glyphs)}")
    return glyphs, bytes(name_blob), alpha_blob


def write_binary(out_path: str, baked: int, glyphs, name_blob: bytes, alpha_blob):
    os.makedirs(os.path.dirname(os.path.abspath(out_path)) or ".", exist_ok=True)
    with open(out_path, "wb") as f:
        f.write(b"F32I")
        f.write(struct.pack("<H", 1))  # version
        f.write(struct.pack("<H", baked))
        f.write(struct.pack("<H", len(glyphs)))
        f.write(struct.pack("<H", 0))  # flags
        f.write(struct.pack("<I", len(name_blob)))
        f.write(struct.pack("<I", len(alpha_blob)))
        f.write(name_blob)
        for g in glyphs:
            f.write(
                struct.pack(
                    "<HHIHHHhh",
                    g["id"],
                    g["name_off"],
                    g["alpha_off"],
                    g["w"],
                    g["h"],
                    g["xAdvance"],
                    g["xOffset"],
                    g["yOffset"],
                )
            )
        f.write(bytes(alpha_blob))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", required=True)
    ap.add_argument(
        "--all",
        action="store_true",
        help="pack every Lucide SVG in the icons directory",
    )
    ap.add_argument(
        "--baked",
        type=int,
        default=96,
        help="atlas pixel size (default 96; UI should draw ≤ this)",
    )
    ap.add_argument(
        "--icons-dir",
        default=None,
        help="local Lucide icons/ folder (otherwise download release tarball)",
    )
    ap.add_argument(
        "--icons",
        default=None,
        help="comma-separated Lucide names (default: curated demo set)",
    )
    ap.add_argument("--cache", default=None, help="download / PNG cache directory")
    args = ap.parse_args()

    if args.baked < 16 or args.baked > 256:
        raise SystemExit("--baked must be 16..256")

    cache = args.cache or os.path.join(os.path.dirname(__file__), ".lucide_cache")
    icons_dir = ensure_icons_dir(cache, args.icons_dir)

    if args.all:
        names = list_all_icons(icons_dir)
        print(f"packing all {len(names)} icons from {icons_dir}")
    elif args.icons:
        names = [n.strip() for n in args.icons.split(",") if n.strip()]
    else:
        names = list(DEFAULT_ICONS)

    png_cache = os.path.join(cache, f"png_{args.baked}")
    glyphs, name_blob, alpha_blob = build_glyphs(
        names, icons_dir, args.baked, png_cache
    )
    if not glyphs:
        raise SystemExit("no icons packed")

    # Rebuild name offsets if ids were reassigned after skips — name_blob is
    # already correct (appended in pack order matching final glyphs).
    write_binary(args.output, args.baked, glyphs, name_blob, alpha_blob)
    print(
        f"wrote {args.output}: {len(glyphs)} icons, "
        f"{len(name_blob)} name bytes + {len(alpha_blob)} alpha bytes, "
        f"baked={args.baked}"
    )


if __name__ == "__main__":
    main()
