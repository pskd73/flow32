#pragma once

#include <Arduino.h>

/** One color emoji glyph (RGB565 + 4bpp alpha), baked at atlas.bakedSize. */
struct ColorEmojiGlyph {
  uint32_t codepoint;
  uint32_t pixelsOffset; // index into atlas.pixels (RGB565)
  uint32_t alphaOffset;  // byte offset into atlas.alpha (4bpp, high nibble first)
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset; // baseline → top (typically negative)
};

struct ColorEmojiAtlas {
  const uint16_t *pixels;
  const uint8_t *alpha;
  const ColorEmojiGlyph *glyphs;
  uint16_t count;
  uint8_t bakedSize; // never upscale past this
};

class Display;

namespace ColorEmojiDraw {

/** Decode one UTF-8 scalar; advances `p`. Returns false at end / invalid. */
bool nextUtf8(const char *&p, uint32_t &cp);

const ColorEmojiGlyph *find(const ColorEmojiAtlas &atlas, uint32_t cp);

/** Advance width at the requested draw size (downscale only). */
int16_t advance(const ColorEmojiGlyph &g, uint8_t bakedSize, int16_t drawPx);

/**
 * Blit glyph at baseline (screen px). drawPx may be below or above bakedSize
 * (downscale / upscale). Prefer drawPx <= bakedSize for sharpness.
 */
void draw(Display &display, const ColorEmojiAtlas &atlas,
          const ColorEmojiGlyph &g, int16_t baselineX, int16_t baselineY,
          int16_t drawPx);

} // namespace ColorEmojiDraw
