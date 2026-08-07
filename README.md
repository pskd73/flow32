# Flow32

ESP32-S3 UI toolkit for SPI TFT panels (ST7789 / ST7735): framebuffer display,
canvas, composable UI, focus/scroll, and pluggable input.

## Layout

| Path | Role |
|------|------|
| `lib/Flow32/` | **Library** (reusable in other PlatformIO projects) |
| `src/main.cpp` | **Driver / demo** firmware for on-device testing |

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

Display display(Panel183());
Canvas canvas(display);
Page page(Rect(0, 0, display.panel().width, display.panel().height));
```

## Demo (this repo)

```bash
python3 -m platformio run -t upload
python3 -m platformio device monitor
```

Serial: `u/d/l/r` move/scroll, `e` focus/activate, `b` back.

Switch panel in `src/main.cpp`: `Panel18()` or `Panel183()`.

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
