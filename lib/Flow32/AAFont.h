#pragma once

#include <Arduino.h>

/** One glyph in a 4bpp grayscale anti-aliased font. */
struct AAGlyph {
  uint16_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset; // from baseline to top of bitmap (typically negative)
};

/**
 * Grayscale AA font (4 bits per pixel, high nibble first).
 * Coverage 0..15 is blended against the existing framebuffer pixel.
 */
struct AAFont {
  const uint8_t *bitmap;
  const AAGlyph *glyph;
  uint8_t first;
  uint8_t last;
  uint8_t yAdvance;
  uint8_t baseline; // top-of-line → baseline
};

class Display;

namespace AAFontDraw {
uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t cover4);
const AAGlyph *glyph(const AAFont &font, char c);
int16_t charWidth(const AAFont &font, char c);
int16_t textWidth(const AAFont &font, const char *text, size_t len);
void drawChar(Display &display, const AAFont &font, int16_t baselineX,
              int16_t baselineY, char c, uint16_t color);
} // namespace AAFontDraw
