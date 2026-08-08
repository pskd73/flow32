#pragma once

#include "Icon.h"
#include "Storage.h"

class Display;

/**
 * SD-backed Lucide (monochrome) icon atlas.
 *
 * begin() loads the glyph index + name table into RAM/PSRAM. Alpha stays on
 * the card and streams into an LRU cache when drawn — tinted with the current
 * text color at blit time.
 *
 * File format (little-endian), magic "F32I":
 *   char     magic[4]      "F32I"
 *   uint16_t version       1
 *   uint16_t bakedSize     e.g. 96 (downscale for UI; avoid upscale)
 *   uint16_t glyphCount
 *   uint16_t flags         0
 *   uint32_t nameBytes
 *   uint32_t alphaBytes
 *   char     names[nameBytes]          // "wifi\0heart\0..."
 *   GlyphRec glyphs[glyphCount]        // 18 bytes, id order = PUA order
 *   uint8_t  alpha[alphaBytes]         // 4bpp, high nibble first
 *
 * GlyphRec:
 *   uint16 id, nameOffset
 *   uint32 alphaOffset
 *   uint16 width, height, xAdvance
 *   int16  xOffset, yOffset
 *
 * Build (curated / all):
 *   python3 tools/lucide_icon_atlas.py -o sd/flow32/icons.atlas --baked 96
 *   python3 tools/lucide_icon_atlas.py --all -o sd/flow32/icons.atlas --baked 96
 */
class IconSd {
public:
  static constexpr const char *kDefaultRelPath = "flow32/icons.atlas";
  static constexpr uint8_t kCacheSlots = 64;

  IconSd() = default;
  ~IconSd() { end(); }

  IconSd(const IconSd &) = delete;
  IconSd &operator=(const IconSd &) = delete;

  bool begin(Storage &storage, const char *relPath = kDefaultRelPath);
  void end();

  bool ready() const { return ready_; }
  uint16_t glyphCount() const { return count_; }
  uint16_t bakedSize() const { return bakedSize_; }

  /** Index lookup by Lucide name ("wifi") — no SD I/O. */
  const IconGlyph *findByName(const char *name) const;
  const IconGlyph *findById(uint16_t id) const;
  const IconGlyph *findByCp(uint32_t cp) const;

  const char *nameOf(const IconGlyph &g) const;

  int16_t advance(uint32_t cp, int16_t drawPx) const;

  /**
   * Write UTF-8 for icon `name` into buf (no trailing NUL required beyond
   * return length). Returns bytes written, or 0 if missing / too small.
   */
  size_t utf8(const char *name, char *buf, size_t cap) const;

  /** PUA codepoint for name, or 0 if missing. */
  uint32_t codepoint(const char *name) const;

  bool draw(Display &display, uint32_t cp, int16_t baselineX, int16_t baselineY,
            int16_t drawPx, uint16_t color);

private:
  struct CacheSlot {
    uint16_t id = 0;
    bool valid = false;
    uint32_t lastUsed = 0;
    uint8_t *alpha = nullptr;
    size_t alphaCap = 0;
    IconGlyph glyph{};
    IconAtlas atlas{};
  };

  bool ready_ = false;
  Storage *storage_ = nullptr;
  char path_[96] = {};
  File file_;

  uint16_t bakedSize_ = 0;
  uint16_t count_ = 0;
  uint32_t alphaFileOff_ = 0;

  char *names_ = nullptr;
  IconGlyph *glyphs_ = nullptr;
  CacheSlot slots_[kCacheSlots] = {};
  uint32_t useTick_ = 0;

  int findNameIndex(const char *name) const;
  int findSlot(uint16_t id) const;
  int pickVictim() const;
  bool loadSlot(CacheSlot &slot, const IconGlyph &g);
  bool ensureCache(const IconGlyph &g, CacheSlot *&out);
};
