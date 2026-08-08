#include "Icon.h"
#include "Display.h"

namespace IconDraw {

size_t encodeUtf8(uint32_t cp, char *buf, size_t cap) {
  if (!buf || cap == 0) return 0;
  if (cp < 0x80) {
    if (cap < 1) return 0;
    buf[0] = static_cast<char>(cp);
    return 1;
  }
  if (cp < 0x800) {
    if (cap < 2) return 0;
    buf[0] = static_cast<char>(0xC0 | (cp >> 6));
    buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    if (cap < 3) return 0;
    buf[0] = static_cast<char>(0xE0 | (cp >> 12));
    buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
    return 3;
  }
  if (cap < 4) return 0;
  buf[0] = static_cast<char>(0xF0 | (cp >> 18));
  buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
  buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
  buf[3] = static_cast<char>(0x80 | (cp & 0x3F));
  return 4;
}

int16_t advance(const IconGlyph &g, uint16_t bakedSize, int16_t drawPx) {
  if (bakedSize == 0) return 0;
  if (drawPx < 1) drawPx = 1;
  return static_cast<int16_t>(
      (static_cast<int32_t>(g.xAdvance) * drawPx + bakedSize / 2) / bakedSize);
}

static uint8_t readAlpha4(const IconAtlas &atlas, const IconGlyph &g, int16_t x,
                          int16_t y) {
  const uint32_t i =
      static_cast<uint32_t>(y) * g.width + static_cast<uint32_t>(x);
  const uint8_t byte = atlas.alpha[g.alphaOffset + (i >> 1)];
  return (i & 1) ? (byte & 0x0F) : (byte >> 4);
}

void draw(Display &display, const IconAtlas &atlas, const IconGlyph &g,
          int16_t baselineX, int16_t baselineY, int16_t drawPx,
          uint16_t color) {
  const uint16_t baked = atlas.bakedSize;
  if (baked == 0 || !atlas.alpha) return;
  if (drawPx < 1) drawPx = 1;

  const int16_t srcW = static_cast<int16_t>(g.width);
  const int16_t srcH = static_cast<int16_t>(g.height);
  if (srcW <= 0 || srcH <= 0) return;

  const int16_t dstW = static_cast<int16_t>(
      (static_cast<int32_t>(srcW) * drawPx + baked / 2) / baked);
  const int16_t dstH = static_cast<int16_t>(
      (static_cast<int32_t>(srcH) * drawPx + baked / 2) / baked);
  if (dstW <= 0 || dstH <= 0) return;

  const int16_t x0 = static_cast<int16_t>(
      baselineX +
      (static_cast<int32_t>(g.xOffset) * drawPx + baked / 2) / baked);
  const int16_t y0 = static_cast<int16_t>(
      baselineY +
      (static_cast<int32_t>(g.yOffset) * drawPx + baked / 2) / baked);

  for (int16_t dy = 0; dy < dstH; dy++) {
    const int16_t sy0 =
        static_cast<int16_t>((static_cast<int32_t>(dy) * srcH) / dstH);
    int16_t sy1 =
        static_cast<int16_t>((static_cast<int32_t>(dy + 1) * srcH) / dstH);
    if (sy1 <= sy0) sy1 = static_cast<int16_t>(sy0 + 1);
    if (sy1 > srcH) sy1 = srcH;

    for (int16_t dx = 0; dx < dstW; dx++) {
      const int16_t sx0 =
          static_cast<int16_t>((static_cast<int32_t>(dx) * srcW) / dstW);
      int16_t sx1 =
          static_cast<int16_t>((static_cast<int32_t>(dx + 1) * srcW) / dstW);
      if (sx1 <= sx0) sx1 = static_cast<int16_t>(sx0 + 1);
      if (sx1 > srcW) sx1 = srcW;

      uint32_t aSum = 0;
      uint16_t n = 0;
      for (int16_t sy = sy0; sy < sy1; sy++) {
        for (int16_t sx = sx0; sx < sx1; sx++) {
          aSum += readAlpha4(atlas, g, sx, sy);
          n++;
        }
      }
      if (n == 0) continue;
      const uint8_t cover = static_cast<uint8_t>((aSum + n / 2) / n);
      if (cover == 0) continue;
      display.blendPixel(static_cast<int16_t>(x0 + dx),
                         static_cast<int16_t>(y0 + dy), color, cover);
    }
  }
}

} // namespace IconDraw
