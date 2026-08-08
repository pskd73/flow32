Copy this folder to the root of a FAT32 microSD card:

  /flow32/emoji.atlas

Rebuild the FULL single-codepoint emoji set (Noto PNGs @ 64px):

  python3 tools/noto_emoji_atlas.py --all --format bin \
      -o sd/flow32/emoji.atlas --baked 64

Firmware loads only the glyph index into RAM and streams each face from the
card when drawn (so the full set does not need to fit in PSRAM).

Fonts: Noto Color Emoji (OFL-1.1) — see lib/Flow32/NotoColorEmoji.LICENSE.txt
