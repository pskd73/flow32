# Flow32

ESP32-S3 UI toolkit for SPI TFT panels (ST7789 / ST7735): framebuffer display,
canvas, composable UI, focus/scroll, and pluggable input.

## Layout

| Path | Role |
|------|------|
| `lib/Flow32/` | **Library** (reusable — no board-specific panels or demos) |
| `src/panels.h` | App panel profiles (`Panel18`, `Panel183`, …) |
| `src/storage.h` | App SD pin presets (`SdOceanLabzCam`, …) |
| `sd/flow32/` | Files to copy onto the microSD card |
| `src/main.cpp` | Demo firmware for on-device testing |

## Use as a library

From another PlatformIO project:

```ini
lib_deps =
  symlink://../flow32/lib/Flow32
  ; or copy lib/Flow32 into that project's lib/
build_flags =
  -DBOARD_HAS_PSRAM
```

```cpp
#include <Flow32.h>

// Define your own DisplayPanel (or copy src/panels.h).
DisplayPanel panel = /* ... */;
Display display(panel);
Canvas canvas(display);
Page page(Rect(0, 0, panel.width, panel.height));
```

## Demo (this repo)

```bash
python3 -m platformio run -t upload
python3 -m platformio device monitor
```

Serial: `u/d/l/r` move/scroll, `e` focus/activate, `b` back.

Switch panel in `src/main.cpp`: `Panel18()` or `Panel183()` from `src/panels.h`.

## Theme (daisyUI-style tokens)

Colors and radii live in `Theme::ThemeTokens`. Built-ins: `FlowTheme()` (warm dark,
default) and `WinterTheme()` (light, from a daisyUI winter palette).

```cpp
Theme::setActive(Theme::WinterTheme());
const auto &t = Theme::active();
page.setContentBackground(t.base100);
// buttons read primary / secondary / accent via Theme::buttonChrome()
```

## Wiring (default pins)

| LCD | ESP32-S3 |
|-----|----------|
| VCC | 3V3 |
| GND | GND |
| DIN / SDA | GPIO 11 |
| CLK / SCK | GPIO 12 |
| CS | GPIO 10 |
| DC / AO | GPIO 9 |
| RST | GPIO 14 |
| BL / LED | GPIO 13 |

## Storage (microSD)

`Storage` mounts the onboard card; pinouts live in `src/storage.h` (default:
OceanLabz ESP32-S3 N16R8 CAM — SDMMC 1-bit **CLK=39, CMD=38, D0=40**).

Color emoji atlases are **not** baked into flash. Build the full set and copy to the card:

```bash
python3 tools/noto_emoji_atlas.py --all --format bin -o sd/flow32/emoji.atlas --baked 64
# Copy the `flow32/` folder onto the SD card root → /flow32/emoji.atlas
```

Firmware loads the glyph **index** into PSRAM and streams each emoji from the card
when drawn (`ColorEmojiSd`). Change SD pins only in `src/storage.h`.
