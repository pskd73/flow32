#include "ColorEmoji.h"
#include "Display.h"
#include "AAFont.h"

namespace ColorEmojiDraw {

bool nextUtf8(const char *&p, uint32_t &cp) {
  if (!p || !*p) return false;
  const uint8_t c0 = static_cast<uint8_t>(*p++);
  if (c0 < 0x80) {
    cp = c0;
    return true;
  }
  if ((c0 & 0xE0) == 0xC0) {
    if (!*p) return false;
    const uint8_t c1 = static_cast<uint8_t>(*p++);
    cp = (static_cast<uint32_t>(c0 & 0x1F) << 6) | (c1 & 0x3F);
    return true;
  }
  if ((c0 & 0xF0) == 0xE0) {
    if (!p[0] || !p[1]) return false;
    const uint8_t c1 = static_cast<uint8_t>(*p++);
    const uint8_t c2 = static_cast<uint8_t>(*p++);
    cp = (static_cast<uint32_t>(c0 & 0x0F) << 12) |
         (static_cast<uint32_t>(c1 & 0x3F) << 6) | (c2 & 0x3F);
    return true;
  }
  if ((c0 & 0xF8) == 0xF0) {
    if (!p[0] || !p[1] || !p[2]) return false;
    const uint8_t c1 = static_cast<uint8_t>(*p++);
    const uint8_t c2 = static_cast<uint8_t>(*p++);
    const uint8_t c3 = static_cast<uint8_t>(*p++);
    cp = (static_cast<uint32_t>(c0 & 0x07) << 18) |
         (static_cast<uint32_t>(c1 & 0x3F) << 12) |
         (static_cast<uint32_t>(c2 & 0x3F) << 6) | (c3 & 0x3F);
    return true;
  }
  // Invalid lead — skip as replacement.
  cp = 0xFFFD;
  return true;
}

const ColorEmojiGlyph *find(const ColorEmojiAtlas &atlas, uint32_t cp) {
  // Small tables — linear scan is fine (expand to bsearch later).
  for (uint16_t i = 0; i < atlas.count; i++) {
    if (pgm_read_dword(&atlas.glyphs[i].codepoint) == cp) {
      return &atlas.glyphs[i];
    }
  }
  return nullptr;
}

int16_t advance(const ColorEmojiGlyph &g, uint8_t bakedSize, int16_t drawPx) {
  if (bakedSize == 0) return 0;
  if (drawPx < 1) drawPx = 1;
  const uint8_t adv = pgm_read_byte(&g.xAdvance);
  return static_cast<int16_t>((static_cast<int32_t>(adv) * drawPx + bakedSize / 2) /
                              bakedSize);
}

static uint16_t readPixel(const ColorEmojiAtlas &atlas, const ColorEmojiGlyph &g,
                          int16_t x, int16_t y) {
  const uint8_t w = pgm_read_byte(&g.width);
  const uint32_t off = pgm_read_dword(&g.pixelsOffset);
  return pgm_read_word(&atlas.pixels[off + static_cast<uint32_t>(y) * w + x]);
}

static uint8_t readAlpha4(const ColorEmojiAtlas &atlas, const ColorEmojiGlyph &g,
                          int16_t x, int16_t y) {
  const uint8_t w = pgm_read_byte(&g.width);
  const uint32_t off = pgm_read_dword(&g.alphaOffset);
  const uint32_t i = static_cast<uint32_t>(y) * w + x;
  const uint8_t byte = pgm_read_byte(&atlas.alpha[off + (i >> 1)]);
  return (i & 1) ? (byte & 0x0F) : (byte >> 4);
}

void draw(Display &display, const ColorEmojiAtlas &atlas,
          const ColorEmojiGlyph &g, int16_t baselineX, int16_t baselineY,
          int16_t drawPx) {
  const uint8_t baked = atlas.bakedSize;
  if (baked == 0) return;
  if (drawPx < 1) drawPx = 1;

  const uint8_t srcW = pgm_read_byte(&g.width);
  const uint8_t srcH = pgm_read_byte(&g.height);
  const int8_t xOff = static_cast<int8_t>(pgm_read_byte(&g.xOffset));
  const int8_t yOff = static_cast<int8_t>(pgm_read_byte(&g.yOffset));

  const int16_t dstW = static_cast<int16_t>(
      (static_cast<int32_t>(srcW) * drawPx + baked / 2) / baked);
  const int16_t dstH = static_cast<int16_t>(
      (static_cast<int32_t>(srcH) * drawPx + baked / 2) / baked);
  if (dstW <= 0 || dstH <= 0) return;

  const int16_t x0 = static_cast<int16_t>(
      baselineX + (static_cast<int32_t>(xOff) * drawPx + baked / 2) / baked);
  const int16_t y0 = static_cast<int16_t>(
      baselineY + (static_cast<int32_t>(yOff) * drawPx + baked / 2) / baked);

  for (int16_t dy = 0; dy < dstH; dy++) {
    const int16_t sy0 = static_cast<int16_t>((static_cast<int32_t>(dy) * srcH) / dstH);
    const int16_t sy1 =
        static_cast<int16_t>((static_cast<int32_t>(dy + 1) * srcH) / dstH);
    const int16_t yStart = sy0;
    int16_t yEnd = sy1;
    if (yEnd <= yStart) yEnd = static_cast<int16_t>(yStart + 1);
    if (yEnd > srcH) yEnd = srcH;

    for (int16_t dx = 0; dx < dstW; dx++) {
      const int16_t sx0 = static_cast<int16_t>((static_cast<int32_t>(dx) * srcW) / dstW);
      const int16_t sx1 =
          static_cast<int16_t>((static_cast<int32_t>(dx + 1) * srcW) / dstW);
      const int16_t xStart = sx0;
      int16_t xEnd = sx1;
      if (xEnd <= xStart) xEnd = static_cast<int16_t>(xStart + 1);
      if (xEnd > srcW) xEnd = srcW;

      uint32_t rSum = 0, gSum = 0, bSum = 0, aSum = 0;
      uint16_t n = 0;
      for (int16_t sy = yStart; sy < yEnd; sy++) {
        for (int16_t sx = xStart; sx < xEnd; sx++) {
          const uint8_t a4 = readAlpha4(atlas, g, sx, sy);
          if (a4 == 0) {
            n++;
            continue;
          }
          const uint16_t c = readPixel(atlas, g, sx, sy);
          rSum += ((c >> 11) & 0x1F) * a4;
          gSum += ((c >> 5) & 0x3F) * a4;
          bSum += (c & 0x1F) * a4;
          aSum += a4;
          n++;
        }
      }
      if (aSum == 0 || n == 0) continue;

      const uint8_t cover =
          static_cast<uint8_t>((aSum + n / 2) / n); // 0..15 average
      if (cover == 0) continue;
      const uint8_t r = static_cast<uint8_t>(rSum / aSum);
      const uint8_t gch = static_cast<uint8_t>(gSum / aSum);
      const uint8_t b = static_cast<uint8_t>(bSum / aSum);
      const uint16_t fg =
          static_cast<uint16_t>((r << 11) | (gch << 5) | b);
      display.blendPixel(static_cast<int16_t>(x0 + dx),
                         static_cast<int16_t>(y0 + dy), fg, cover);
    }
  }
}

} // namespace ColorEmojiDraw
