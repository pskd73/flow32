#include "ColorEmojiSd.h"
#include "Display.h"

#include <esp_heap_caps.h>
#include <string.h>

namespace {

constexpr char kMagic[4] = {'F', '3', '2', 'E'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderBytes = 4 + 2 + 1 + 1 + 2 + 2 + 4 + 4; // 20
constexpr size_t kGlyphRecBytes = 17;

struct __attribute__((packed)) GlyphRec {
  uint32_t codepoint;
  uint32_t pixelsOffset;
  uint32_t alphaOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset;
};
static_assert(sizeof(GlyphRec) == kGlyphRecBytes, "GlyphRec size");

void *allocPreferPsram(size_t bytes) {
  void *p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  return p;
}

} // namespace

bool ColorEmojiSd::begin(Storage &storage, const char *relPath) {
  end();
  if (!storage.ready()) {
    Serial.println("ColorEmojiSd: storage not ready");
    return false;
  }

  if (!storage.absPath(relPath ? relPath : kDefaultRelPath, path_,
                       sizeof(path_))) {
    Serial.println("ColorEmojiSd: path too long");
    return false;
  }

  File f = storage.open(path_, FILE_READ);
  if (!f) {
    Serial.printf("ColorEmojiSd: missing %s\n", path_);
    return false;
  }

  char magic[4];
  if (f.read(reinterpret_cast<uint8_t *>(magic), 4) != 4 ||
      memcmp(magic, kMagic, 4) != 0) {
    Serial.println("ColorEmojiSd: bad magic");
    f.close();
    return false;
  }

  uint16_t version = 0;
  uint8_t baked = 0;
  uint8_t reserved = 0;
  uint16_t count = 0;
  uint16_t reserved2 = 0;
  uint32_t pixelsBytes = 0;
  uint32_t alphaBytes = 0;

  if (f.read(reinterpret_cast<uint8_t *>(&version), 2) != 2 ||
      f.read(&baked, 1) != 1 || f.read(&reserved, 1) != 1 ||
      f.read(reinterpret_cast<uint8_t *>(&count), 2) != 2 ||
      f.read(reinterpret_cast<uint8_t *>(&reserved2), 2) != 2 ||
      f.read(reinterpret_cast<uint8_t *>(&pixelsBytes), 4) != 4 ||
      f.read(reinterpret_cast<uint8_t *>(&alphaBytes), 4) != 4) {
    Serial.println("ColorEmojiSd: truncated header");
    f.close();
    return false;
  }

  if (version != kVersion || count == 0 || baked == 0) {
    Serial.printf("ColorEmojiSd: unsupported ver=%u count=%u baked=%u\n",
                  version, count, baked);
    f.close();
    return false;
  }

  const size_t glyphBytes = static_cast<size_t>(count) * sizeof(ColorEmojiGlyph);
  glyphs_ = static_cast<ColorEmojiGlyph *>(allocPreferPsram(glyphBytes));
  if (!glyphs_) {
    Serial.println("ColorEmojiSd: OOM glyph index");
    f.close();
    return false;
  }

  for (uint16_t i = 0; i < count; i++) {
    GlyphRec rec{};
    if (f.read(reinterpret_cast<uint8_t *>(&rec), sizeof(rec)) !=
        static_cast<int>(sizeof(rec))) {
      Serial.println("ColorEmojiSd: truncated glyphs");
      f.close();
      end();
      return false;
    }
    glyphs_[i].codepoint = rec.codepoint;
    glyphs_[i].pixelsOffset = rec.pixelsOffset;
    glyphs_[i].alphaOffset = rec.alphaOffset;
    glyphs_[i].width = rec.width;
    glyphs_[i].height = rec.height;
    glyphs_[i].xAdvance = rec.xAdvance;
    glyphs_[i].xOffset = rec.xOffset;
    glyphs_[i].yOffset = rec.yOffset;
  }

  pixelsFileOff_ = kHeaderBytes + static_cast<uint32_t>(count) * kGlyphRecBytes;
  alphaFileOff_ = pixelsFileOff_ + pixelsBytes;

  // Keep the file open for cached seeks (open/close per glyph is too slow).
  file_ = f;
  storage_ = &storage;
  count_ = count;
  bakedSize_ = baked;
  pixelsBytes_ = pixelsBytes;
  alphaBytes_ = alphaBytes;
  ready_ = true;

  const size_t indexKb = glyphBytes / 1024;
  Serial.printf(
      "ColorEmojiSd: index %u glyphs @%upx from %s (%u KB index, %.1f MB on SD, "
      "%u-slot cache)\n",
      count, baked, path_, static_cast<unsigned>(indexKb),
      (pixelsBytes + alphaBytes) / (1024.0f * 1024.0f),
      static_cast<unsigned>(kCacheSlots));
  return true;
}

void ColorEmojiSd::end() {
  ready_ = false;
  storage_ = nullptr;
  path_[0] = '\0';
  if (file_) {
    file_.close();
  }
  count_ = 0;
  bakedSize_ = 0;
  pixelsBytes_ = 0;
  alphaBytes_ = 0;
  pixelsFileOff_ = 0;
  alphaFileOff_ = 0;
  useTick_ = 0;
  if (glyphs_) {
    free(glyphs_);
    glyphs_ = nullptr;
  }
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    CacheSlot &s = slots_[i];
    if (s.pixels) {
      free(s.pixels);
      s.pixels = nullptr;
    }
    if (s.alpha) {
      free(s.alpha);
      s.alpha = nullptr;
    }
    s = CacheSlot{};
  }
}

int ColorEmojiSd::findIndex(uint32_t cp) const {
  if (!glyphs_ || count_ == 0) return -1;
  int lo = 0;
  int hi = static_cast<int>(count_) - 1;
  while (lo <= hi) {
    const int mid = lo + (hi - lo) / 2;
    const uint32_t v = glyphs_[mid].codepoint;
    if (v == cp) return mid;
    if (v < cp) lo = mid + 1;
    else hi = mid - 1;
  }
  return -1;
}

const ColorEmojiGlyph *ColorEmojiSd::find(uint32_t cp) const {
  const int i = findIndex(cp);
  return i >= 0 ? &glyphs_[i] : nullptr;
}

int16_t ColorEmojiSd::advance(uint32_t cp, int16_t drawPx) const {
  const ColorEmojiGlyph *g = find(cp);
  if (!g || bakedSize_ == 0) return 0;
  return ColorEmojiDraw::advance(*g, bakedSize_, drawPx);
}

int ColorEmojiSd::findSlot(uint32_t cp) const {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (slots_[i].valid && slots_[i].cp == cp) return i;
  }
  return -1;
}

int ColorEmojiSd::pickVictim() const {
  int best = 0;
  uint32_t oldest = slots_[0].lastUsed;
  for (uint8_t i = 1; i < kCacheSlots; i++) {
    if (!slots_[i].valid) return i;
    if (slots_[i].lastUsed < oldest) {
      oldest = slots_[i].lastUsed;
      best = i;
    }
  }
  if (!slots_[0].valid) return 0;
  return best;
}

bool ColorEmojiSd::loadSlot(CacheSlot &slot, const ColorEmojiGlyph &g) {
  if (!storage_ || !storage_->ready()) return false;
  if (!file_) {
    file_ = storage_->open(path_, FILE_READ);
    if (!file_) return false;
  }

  const size_t pixCount = static_cast<size_t>(g.width) * g.height;
  const size_t pixBytes = pixCount * sizeof(uint16_t);
  const size_t alphaBytes = (pixCount + 1) / 2;

  if (pixBytes > slot.pixCap) {
    if (slot.pixels) free(slot.pixels);
    slot.pixels = static_cast<uint16_t *>(allocPreferPsram(pixBytes));
    slot.pixCap = slot.pixels ? pixBytes : 0;
  }
  if (alphaBytes > slot.alphaCap) {
    if (slot.alpha) free(slot.alpha);
    slot.alpha = static_cast<uint8_t *>(allocPreferPsram(alphaBytes));
    slot.alphaCap = slot.alpha ? alphaBytes : 0;
  }
  if (!slot.pixels || !slot.alpha) {
    Serial.println("ColorEmojiSd: OOM glyph cache");
    slot.valid = false;
    return false;
  }

  const uint32_t pixOff =
      pixelsFileOff_ + g.pixelsOffset * static_cast<uint32_t>(sizeof(uint16_t));
  const uint32_t aOff = alphaFileOff_ + g.alphaOffset;
  if (!file_.seek(pixOff)) {
    slot.valid = false;
    return false;
  }
  if (file_.read(reinterpret_cast<uint8_t *>(slot.pixels), pixBytes) !=
      static_cast<int>(pixBytes)) {
    slot.valid = false;
    return false;
  }
  if (!file_.seek(aOff)) {
    slot.valid = false;
    return false;
  }
  if (file_.read(slot.alpha, alphaBytes) != static_cast<int>(alphaBytes)) {
    slot.valid = false;
    return false;
  }

  slot.glyph = g;
  slot.glyph.pixelsOffset = 0;
  slot.glyph.alphaOffset = 0;
  slot.atlas.pixels = slot.pixels;
  slot.atlas.alpha = slot.alpha;
  slot.atlas.glyphs = &slot.glyph;
  slot.atlas.count = 1;
  slot.atlas.bakedSize = bakedSize_;
  slot.cp = g.codepoint;
  slot.valid = true;
  return true;
}

bool ColorEmojiSd::ensureCache(const ColorEmojiGlyph &g, CacheSlot *&out) {
  int idx = findSlot(g.codepoint);
  if (idx < 0) {
    idx = pickVictim();
    if (!loadSlot(slots_[idx], g)) {
      out = nullptr;
      return false;
    }
  }
  slots_[idx].lastUsed = ++useTick_;
  out = &slots_[idx];
  return true;
}

bool ColorEmojiSd::draw(Display &display, uint32_t cp, int16_t baselineX,
                        int16_t baselineY, int16_t drawPx) {
  const ColorEmojiGlyph *g = find(cp);
  if (!g) return false;
  CacheSlot *slot = nullptr;
  if (!ensureCache(*g, slot) || !slot) return false;
  ColorEmojiDraw::draw(display, slot->atlas, slot->glyph, baselineX, baselineY,
                       drawPx);
  return true;
}
