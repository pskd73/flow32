# Flow32

ESP32-S3 UI toolkit for SPI TFT panels (ST7789 / ST7735): framebuffer display,
canvas, composable UI, focus/scroll, and pluggable input.

## Wiring (default pins)

| LCD | ESP32-S3 |
|-----|----------|
| VCC | 3V3 |
| GND | GND |
| DIN / SDA (MOSI) | GPIO 11 |
| CLK / SCK | GPIO 12 |
| CS | GPIO 10 |
| DC / AO | GPIO 9 |
| RST / RESET | GPIO 14 |
| BL / LED | GPIO 13 |

## Panels

Switch in `src/main.cpp`:

- `Panel183()` — 1.83" ST7789, 240×284
- `Panel18()` — 1.8" ST7735 landscape, design buffer + present

## Flash

```bash
python3 -m platformio run -t upload
```

Serial input: `u/d/l/r`, `e` select, `b` back.
