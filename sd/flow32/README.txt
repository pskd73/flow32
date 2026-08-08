Copy this folder to the root of a FAT32 microSD card:

  /flow32/emoji.atlas
  /flow32/icons.atlas

Rebuild the FULL single-codepoint emoji set (Noto PNGs @ 64px):

  python3 tools/noto_emoji_atlas.py --all --format bin \
      -o sd/flow32/emoji.atlas --baked 64

Rebuild Lucide icons (alpha @ 96px — tinted at draw time):

  python3 tools/lucide_icon_atlas.py -o sd/flow32/icons.atlas --baked 96
  # full set:
  python3 tools/lucide_icon_atlas.py --all -o sd/flow32/icons.atlas --baked 96

Firmware loads only indexes into RAM and streams glyphs from the card when
drawn (so full sets do not need to fit in PSRAM). Prefer draw size ≤ baked
size so icons stay sharp.

Fonts: Noto Color Emoji (OFL-1.1) — see lib/Flow32/NotoColorEmoji.LICENSE.txt
Icons: Lucide (ISC) — https://lucide.dev
