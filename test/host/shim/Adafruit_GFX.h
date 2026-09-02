#pragma once

#include "Arduino.h"

struct GFXglyph {
  uint16_t bitmapOffset = 0;
  uint8_t width = 0;
  uint8_t height = 0;
  uint8_t xAdvance = 6;
  int8_t xOffset = 0;
  int8_t yOffset = 0;
};

struct GFXfont {
  uint8_t *bitmap = nullptr;
  GFXglyph *glyph = nullptr;
  uint16_t first = 0;
  uint16_t last = 0;
  uint8_t yAdvance = 8;
};

class Adafruit_GFX : public Print {
public:
  Adafruit_GFX(int16_t width, int16_t height) : WIDTH(width), HEIGHT(height) {}
  virtual void drawPixel(int16_t, int16_t, uint16_t) {}
  virtual void drawFastHLine(int16_t, int16_t, int16_t, uint16_t) {}
  virtual void drawFastVLine(int16_t, int16_t, int16_t, uint16_t) {}
  virtual void fillRect(int16_t, int16_t, int16_t, int16_t, uint16_t) {}
  void drawRGBBitmap(int16_t, int16_t, const uint16_t *, int16_t, int16_t) {}
  void setFont(const GFXfont *font = nullptr) { gfxFont = const_cast<GFXfont *>(font); }
  void setCursor(int16_t, int16_t) {}
  void setTextColor(uint16_t) {}
  void setTextSize(uint8_t size) { textsize_x = size; }
  void setTextWrap(bool) {}
  void getTextBounds(const char *text, int16_t x, int16_t y, int16_t *x1,
                     int16_t *y1, uint16_t *width, uint16_t *height) const {
    if (x1) *x1 = x;
    if (y1) *y1 = y;
    if (width) {
      *width = static_cast<uint16_t>((text ? std::strlen(text) : 0) * 6U *
                                     textsize_x);
    }
    if (height) *height = static_cast<uint16_t>(8U * textsize_x);
  }

protected:
  int16_t WIDTH;
  int16_t HEIGHT;
  GFXfont *gfxFont = nullptr;
  uint8_t textsize_x = 1;
};
