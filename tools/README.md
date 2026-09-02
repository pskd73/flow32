# Tools

All asset generators run on the development host. They do not mount storage or
run on the ESP32.

| Tool | Output |
| --- | --- |
| `ttf_to_aafont.py` | anti-aliased font header from an application-licensed TTF |
| `lucide_icon_atlas.py` | tintable monochrome icon atlas |
| `noto_emoji_atlas.py` | RGB565 plus 4-bit-alpha emoji atlas |
| `flow32_configure.py` | optional physically reduced source package |

Examples:

```bash
python3 tools/lucide_icon_atlas.py --all \
  --baked 96 -o data/flow32/icons.atlas

python3 tools/noto_emoji_atlas.py --all --format bin \
  --baked 64 -o data/flow32/emoji.atlas

python3 tools/flow32_configure.py --check
```

Generated atlases are untrusted external input at runtime. Flow32 validates
their complete section layout and every glyph range before accepting them.
