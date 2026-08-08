# Flow32

ESP32-S3 UI toolkit for SPI TFT panels (ST7789 / ST7735): framebuffer display,
canvas, composable UI, focus/scroll, and pluggable input.

## Layout

| Path | Role |
|------|------|
| `lib/Flow32/` | **Library** (reusable — no board-specific panels or demos) |
| `src/panels.h` | App panel profiles (`Panel18`, `Panel183`, …) |
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
