#pragma once

#include "ColorEmoji.h"
#include "Storage.h"

class Display;

/**
 * SD-backed color emoji atlas.
 *
 * begin() loads only the glyph index into RAM/PSRAM. Pixel/alpha data stay on
 * the card and are streamed into a small LRU cache when drawn — so a full Noto
 * set can live on SD without fitting in PSRAM.
 *
 * File format (little-endian), magic "F32E":
 *   char     magic[4]     "F32E"
 *   uint16_t version      1
 *   uint8_t  bakedSize
 *   uint8_t  reserved
 *   uint16_t glyphCount
 *   uint16_t reserved2
 *   uint32_t pixelsBytes
 *   uint32_t alphaBytes
 *   GlyphRec glyphs[glyphCount]  // 17 bytes, sorted by codepoint
 *   uint16_t pixels[...]
 *   uint8_t  alpha[...]
 *
 * Build (full set):
 *   python3 tools/noto_emoji_atlas.py --all --format bin \
 *       -o sd/flow32/emoji.atlas --baked 64
 */
class ColorEmojiSd {
public:
  static constexpr const char *kDefaultRelPath = "flow32/emoji.atlas";
  /** Keep a large working set in PSRAM so on-screen emoji don't re-hit SD. */
  static constexpr uint8_t kCacheSlots = 100;

  ColorEmojiSd() = default;
  ~ColorEmojiSd() { end(); }

  ColorEmojiSd(const ColorEmojiSd &) = delete;
  ColorEmojiSd &operator=(const ColorEmojiSd &) = delete;

  bool begin(Storage &storage, const char *relPath = kDefaultRelPath);
  void end();

  bool ready() const { return ready_; }
  uint16_t glyphCount() const { return count_; }
  uint8_t bakedSize() const { return bakedSize_; }

  /** Index lookup only (no SD I/O). */
  const ColorEmojiGlyph *find(uint32_t cp) const;

  int16_t advance(uint32_t cp, int16_t drawPx) const;

  /**
   * Load glyph pixels from SD into the LRU cache (if needed) and blit.
   * Returns false if codepoint missing or I/O fails.
   */
  bool draw(Display &display, uint32_t cp, int16_t baselineX, int16_t baselineY,
            int16_t drawPx);

private:
  struct CacheSlot {
    uint32_t cp = 0;
    bool valid = false;
    uint32_t lastUsed = 0;
    uint16_t *pixels = nullptr;
    uint8_t *alpha = nullptr;
    size_t pixCap = 0;
    size_t alphaCap = 0;
    ColorEmojiGlyph glyph{};
    ColorEmojiAtlas atlas{};
  };

  bool ready_ = false;
  Storage *storage_ = nullptr;
  char path_[96] = {};
  File file_;

  uint16_t count_ = 0;
  uint8_t bakedSize_ = 0;
  uint32_t pixelsBytes_ = 0;
  uint32_t alphaBytes_ = 0;
  uint32_t pixelsFileOff_ = 0;
  uint32_t alphaFileOff_ = 0;
  uint32_t useTick_ = 0;

  ColorEmojiGlyph *glyphs_ = nullptr;
  CacheSlot slots_[kCacheSlots];

  bool ensureCache(const ColorEmojiGlyph &g, CacheSlot *&out);
  bool loadSlot(CacheSlot &slot, const ColorEmojiGlyph &g);
  int findIndex(uint32_t cp) const;
  int findSlot(uint32_t cp) const;
  int pickVictim() const;
};
