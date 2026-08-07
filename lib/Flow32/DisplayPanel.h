#pragma once

#include <Arduino.h>

/** Which SPI controller silicon this glass uses. */
enum class PanelChip : uint8_t {
  ST7789,
  ST7735,
};

/**
 * Physical / wiring profile for a TFT module.
 *
 * width × height = design framebuffer + UI coordinates (may be hi-res).
 * nativeWidth × nativeHeight = glass after rotation (SPI present target).
 * When they differ, Display::present() box-averages design → native.
 */
struct DisplayPanel {
  const char *id = "";
  PanelChip chip = PanelChip::ST7789;

  // Design framebuffer / UI coordinates
  int16_t width = 240;
  int16_t height = 320;
  int16_t cornerRadius = 0;

  // Physical pixels after setRotation (defaults = same as design → 1:1 blit)
  int16_t nativeWidth = 0;
  int16_t nativeHeight = 0;
  uint8_t rotation = 0; // Adafruit setRotation 0..3

  // Controller GRAM + where visible row 0 is placed (pre-rotation init size)
  int16_t gramWidth = 240;
  int16_t gramHeight = 320;
  int16_t panelYOffset = 0;

  // SPI pins (ESP32-S3-DevKitC-1; AO=DC, SDA=MOSI, LED=BL)
  int pinCs = 10;
  int pinDc = 9;
  int pinRst = 14;
  int pinBl = 13;
  int pinMosi = 11;
  int pinSclk = 12;

  uint32_t spiHz = 40000000;
  bool blActiveHigh = true;

  void finalize() {
    if (nativeWidth <= 0) nativeWidth = width;
    if (nativeHeight <= 0) nativeHeight = height;
  }

  bool needsDownscale() const {
    return nativeWidth > 0 && nativeHeight > 0 &&
           (nativeWidth != width || nativeHeight != height);
  }

  /** Integer scale factors when evenly divisible; else present uses accumulators. */
  int16_t scaleX() const {
    return (nativeWidth > 0) ? static_cast<int16_t>(width / nativeWidth) : 1;
  }
  int16_t scaleY() const {
    return (nativeHeight > 0) ? static_cast<int16_t>(height / nativeHeight) : 1;
  }
};

/** Flow32 1.83" rounded IPS — design == native (1:1 present). */
inline DisplayPanel Panel183() {
  DisplayPanel p;
  p.id = "Panel183";
  p.chip = PanelChip::ST7789;
  p.width = 240;
  p.height = 284;
  p.nativeWidth = 240;
  p.nativeHeight = 284;
  p.rotation = 0;
  p.cornerRadius = 44;
  p.gramWidth = 240;
  p.gramHeight = 320;
  p.panelYOffset = 36;
  p.spiHz = 80000000;
  p.finalize();
  return p;
}

/**
 * 1.8" ST7735 SPI module — landscape, 2× design buffer, box-average present.
 *
 * Native glass after rotation(1): 160×128
 * Design FB: 320×256 (exact 2×2 downscale)
 *
 * Wiring: VCC→3V3, GND→GND, CS→10, RESET→14, AO→9, SDA→11, SCK→12, LED→13
 */
inline DisplayPanel Panel18() {
  DisplayPanel p;
  p.id = "Panel18";
  p.chip = PanelChip::ST7735;
  p.width = 320;
  p.height = 256;
  p.nativeWidth = 160;
  p.nativeHeight = 128;
  p.rotation = 3; // landscape (other way)
  p.cornerRadius = 0;
  p.gramWidth = 128;
  p.gramHeight = 160;
  p.panelYOffset = 0;
  p.spiHz = 16000000;
  p.blActiveHigh = true;
  p.finalize();
  return p;
}
