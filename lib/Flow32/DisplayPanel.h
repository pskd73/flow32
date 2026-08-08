#pragma once

#include <Arduino.h>

/** Which SPI controller silicon this glass uses. */
enum class PanelChip : uint8_t {
  ST7789,
  ST7735,
};

/**
 * Physical / wiring profile for a TFT module.
 * width × height = framebuffer, UI coordinates, and SPI present size (1:1).
 *
 * Concrete panel presets (Panel18, Panel183, …) live in the app, not the library.
 */
struct DisplayPanel {
  const char *id = "";
  PanelChip chip = PanelChip::ST7789;

  int16_t width = 240;
  int16_t height = 320;
  int16_t cornerRadius = 0;
  uint8_t rotation = 0; // Adafruit setRotation 0..3

  // Controller GRAM + where visible row 0 is placed (pre-rotation init size)
  int16_t gramWidth = 240;
  int16_t gramHeight = 320;
  int16_t panelYOffset = 0;

  // SPI pins
  int pinCs = 10;
  int pinDc = 9;
  int pinRst = 14;
  int pinBl = 13;
  int pinMosi = 11;
  int pinSclk = 12;

  uint32_t spiHz = 40000000;
  bool blActiveHigh = true;

  /**
   * UI density vs design px (padding, gap, radius, fonts). Default 1.0.
   * AA text picks the nearest baked font — no bitmap stretch.
   */
  float uiScale = 1.0f;
};
