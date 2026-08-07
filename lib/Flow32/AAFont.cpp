#include "AAFont.h"
#include "Display.h"

namespace AAFontDraw {

uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t cover4) {
  if (cover4 == 0) return bg;
  if (cover4 >= 15) return fg;

  const uint8_t a = cover4;       // 0..15
  const uint8_t ia = 15 - a;

  const uint8_t fr = (fg >> 11) & 0x1F;
  const uint8_t fg_ = (fg >> 5) & 0x3F;
  const uint8_t fb = fg & 0x1F;

  const uint8_t br = (bg >> 11) & 0x1F;
  const uint8_t bg_ = (bg >> 5) & 0x3F;
  const uint8_t bb = bg & 0x1F;

  const uint8_t r = (fr * a + br * ia) / 15;
  const uint8_t g = (fg_ * a + bg_ * ia) / 15;
  const uint8_t b = (fb * a + bb * ia) / 15;
  return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

const AAGlyph *glyph(const AAFont &font, char c) {
  uint8_t uc = static_cast<uint8_t>(c);
  if (uc < font.first || uc > font.last) {
    uc = static_cast<uint8_t>('?');
    if (uc < font.first || uc > font.last) return nullptr;
  }
  return &font.glyph[uc - font.first];
}

int16_t charWidth(const AAFont &font, char c) {
  const AAGlyph *g = glyph(font, c);
  return g ? g->xAdvance : 0;
}

int16_t textWidth(const AAFont &font, const char *text, size_t len) {
  int16_t w = 0;
  for (size_t i = 0; i < len; i++) {
    w = static_cast<int16_t>(w + charWidth(font, text[i]));
  }
  return w;
}

void drawChar(Display &display, const AAFont &font, int16_t baselineX,
              int16_t baselineY, char c, uint16_t color) {
  const AAGlyph *g = glyph(font, c);
  if (!g || g->width == 0 || g->height == 0) return;

  const int16_t x0 = static_cast<int16_t>(baselineX + g->xOffset);
  const int16_t y0 = static_cast<int16_t>(baselineY + g->yOffset);
  const uint8_t *bits = font.bitmap + g->bitmapOffset;
  const uint16_t total = static_cast<uint16_t>(g->width) * g->height;

  for (uint16_t i = 0; i < total; i++) {
    const uint8_t byte = pgm_read_byte(&bits[i >> 1]);
    const uint8_t cover = (i & 1) ? (byte & 0x0F) : (byte >> 4);
    if (cover == 0) continue;

    const int16_t x = static_cast<int16_t>(x0 + (i % g->width));
    const int16_t y = static_cast<int16_t>(y0 + (i / g->width));
    display.blendPixel(x, y, color, cover);
  }
}

} // namespace AAFontDraw
