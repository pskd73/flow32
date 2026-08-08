#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Lucide (and similar) monochrome icons as tintable 4bpp alpha glyphs.
 * Codepoints live in BMP Private Use Area: U+E000 + id.
 */
struct IconGlyph {
  uint16_t id;
  uint16_t nameOffset; // into atlas name table (NUL-terminated)
  uint32_t alphaOffset;
  uint16_t width;
  uint16_t height;
  uint16_t xAdvance;
  int16_t xOffset;
  int16_t yOffset; // baseline → top (typically negative)
};

struct IconAtlas {
  const uint8_t *alpha;
  const char *names; // concatenated C strings
  const IconGlyph *glyphs;
  uint16_t count;
  uint16_t bakedSize;
};

class Display;

namespace IconDraw {

/** First / last inclusive PUA codepoint used for icons. */
constexpr uint32_t kCpBase = 0xE000u;
constexpr uint32_t kCpLast = 0xF8FFu;

inline bool isIconCp(uint32_t cp) {
  return cp >= kCpBase && cp <= kCpLast;
}

inline uint16_t idFromCp(uint32_t cp) {
  return static_cast<uint16_t>(cp - kCpBase);
}

inline uint32_t cpFromId(uint16_t id) { return kCpBase + id; }

/** Encode one Unicode scalar as UTF-8 into buf. Returns bytes written (0..4). */
size_t encodeUtf8(uint32_t cp, char *buf, size_t cap);

int16_t advance(const IconGlyph &g, uint16_t bakedSize, int16_t drawPx);

/**
 * Tint glyph with `color` (RGB565), blending 4bpp coverage via Display::blendPixel.
 * drawPx may be below bakedSize (preferred) or above (softer upscale).
 */
void draw(Display &display, const IconAtlas &atlas, const IconGlyph &g,
          int16_t baselineX, int16_t baselineY, int16_t drawPx, uint16_t color);

} // namespace IconDraw
