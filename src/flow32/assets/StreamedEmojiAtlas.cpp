#include "flow32/assets/StreamedEmojiAtlas.h"
#include "flow32/graphics/Display.h"

#include <esp_heap_caps.h>
#include <string.h>

namespace {

constexpr char kMagic[4] = {'F', '3', '2', 'E'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderBytes = 4 + 2 + 1 + 1 + 2 + 2 + 4 + 4; // 20
constexpr size_t kGlyphRecBytes = 17;
constexpr uint16_t kMaxGlyphs = 4096;

uint16_t readU16(const uint8_t *p) {
  return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t readU32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

bool readExact(File &file, void *out, size_t bytes) {
  return file.read(static_cast<uint8_t *>(out), bytes) == bytes;
}

void *allocPreferPsram(size_t bytes) {
  void *p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  return p;
}

} // namespace

bool StreamedEmojiAtlas::begin(AssetStore &storage, const char *relPath) {
  end();
  if (!storage.ready()) {
    return false;
  }

  if (!storage.resolve(relPath ? relPath : kDefaultRelPath, path_,
                       sizeof(path_))) {
    return false;
  }

  file_ = storage.open(path_, FILE_READ);
  if (!file_) {
    return false;
  }
  const size_t sourceBytes = file_.size();
  if (sourceBytes < kHeaderBytes || sourceBytes > UINT32_MAX) {
    end();
    return false;
  }
  uint8_t header[kHeaderBytes];
  if (!readExact(file_, header, sizeof(header)) ||
      memcmp(header, kMagic, 4) != 0) {
    end();
    return false;
  }
  const uint16_t version = readU16(header + 4);
  const uint8_t baked = header[6];
  const uint8_t reserved = header[7];
  const uint16_t count = readU16(header + 8);
  const uint16_t reserved2 = readU16(header + 10);
  const uint32_t pixelsBytes = readU32(header + 12);
  const uint32_t alphaBytes = readU32(header + 16);
  const uint64_t pixelsOffset =
      kHeaderBytes + static_cast<uint64_t>(count) * kGlyphRecBytes;
  const uint64_t alphaOffset = pixelsOffset + pixelsBytes;
  const uint64_t expectedBytes = alphaOffset + alphaBytes;
  if (version != kVersion || count == 0 || count > kMaxGlyphs || baked == 0 ||
      reserved != 0 || reserved2 != 0 || pixelsBytes == 0 ||
      (pixelsBytes & 1U) != 0 || alphaBytes == 0 ||
      expectedBytes != sourceBytes) {
    end();
    return false;
  }

  const size_t glyphBytes = static_cast<size_t>(count) * sizeof(ColorEmojiGlyph);
  glyphs_ = static_cast<ColorEmojiGlyph *>(allocPreferPsram(glyphBytes));
  if (!glyphs_) {
    end();
    return false;
  }

  for (uint16_t i = 0; i < count; i++) {
    uint8_t raw[kGlyphRecBytes];
    if (!readExact(file_, raw, sizeof(raw))) {
      end();
      return false;
    }
    ColorEmojiGlyph &glyph = glyphs_[i];
    glyph.codepoint = readU32(raw);
    glyph.pixelsOffset = readU32(raw + 4);
    glyph.alphaOffset = readU32(raw + 8);
    glyph.width = raw[12];
    glyph.height = raw[13];
    glyph.xAdvance = raw[14];
    glyph.xOffset = static_cast<int8_t>(raw[15]);
    glyph.yOffset = static_cast<int8_t>(raw[16]);
    const uint64_t pixelCount =
        static_cast<uint64_t>(glyph.width) * glyph.height;
    const uint64_t pixelByteOffset =
        static_cast<uint64_t>(glyph.pixelsOffset) * sizeof(uint16_t);
    const uint64_t pixelByteCount = pixelCount * sizeof(uint16_t);
    const uint64_t alphaByteCount = (pixelCount + 1) / 2;
    if (glyph.codepoint == 0 || (i > 0 && glyph.codepoint <= glyphs_[i - 1].codepoint) ||
        glyph.width == 0 || glyph.height == 0 || glyph.width > baked ||
        glyph.height > baked ||
        pixelByteOffset > pixelsBytes ||
        pixelByteCount > pixelsBytes - pixelByteOffset ||
        glyph.alphaOffset > alphaBytes ||
        alphaByteCount > alphaBytes - glyph.alphaOffset) {
      end();
      return false;
    }
  }

  fileBytes_ = static_cast<uint32_t>(sourceBytes);
  pixelsFileOff_ = static_cast<uint32_t>(pixelsOffset);
  alphaFileOff_ = static_cast<uint32_t>(alphaOffset);

  // Keep the file open for cached seeks (open/close per glyph is too slow).
  storage_ = &storage;
  count_ = count;
  bakedSize_ = baked;
  pixelsBytes_ = pixelsBytes;
  alphaBytes_ = alphaBytes;
  ready_ = true;

  return true;
}

void StreamedEmojiAtlas::end() {
  ready_ = false;
  storage_ = nullptr;
  path_[0] = '\0';
  if (file_) {
    file_.close();
  }
  count_ = 0;
  bakedSize_ = 0;
  fileBytes_ = 0;
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

int StreamedEmojiAtlas::findIndex(uint32_t cp) const {
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

const ColorEmojiGlyph *StreamedEmojiAtlas::find(uint32_t cp) const {
  const int i = findIndex(cp);
  return i >= 0 ? &glyphs_[i] : nullptr;
}

int16_t StreamedEmojiAtlas::advance(uint32_t cp, int16_t drawPx) const {
  const ColorEmojiGlyph *g = find(cp);
  if (!g || bakedSize_ == 0) return 0;
  return ColorEmojiDraw::advance(*g, bakedSize_, drawPx);
}

int StreamedEmojiAtlas::findSlot(uint32_t cp) const {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (slots_[i].valid && slots_[i].cp == cp) return i;
  }
  return -1;
}

int StreamedEmojiAtlas::pickVictim() const {
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

bool StreamedEmojiAtlas::loadSlot(CacheSlot &slot, const ColorEmojiGlyph &g) {
  if (!storage_ || !storage_->ready()) return false;
  if (!file_) {
    file_ = storage_->open(path_, FILE_READ);
    if (!file_) return false;
  }

  const size_t pixCount = static_cast<size_t>(g.width) * g.height;
  const size_t pixBytes = pixCount * sizeof(uint16_t);
  const size_t alphaBytes = (pixCount + 1) / 2;
  const uint64_t relativePixels =
      static_cast<uint64_t>(g.pixelsOffset) * sizeof(uint16_t);
  if (relativePixels > pixelsBytes_ ||
      pixBytes > pixelsBytes_ - relativePixels ||
      g.alphaOffset > alphaBytes_ ||
      alphaBytes > alphaBytes_ - g.alphaOffset) {
    return false;
  }

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
    slot.valid = false;
    return false;
  }

  const uint64_t absolutePixels = pixelsFileOff_ + relativePixels;
  const uint64_t absoluteAlpha =
      static_cast<uint64_t>(alphaFileOff_) + g.alphaOffset;
  if (absolutePixels + pixBytes > fileBytes_ ||
      absoluteAlpha + alphaBytes > fileBytes_) {
    return false;
  }
  const uint32_t pixOff = static_cast<uint32_t>(absolutePixels);
  const uint32_t aOff = static_cast<uint32_t>(absoluteAlpha);
  if (!file_.seek(pixOff)) {
    slot.valid = false;
    return false;
  }
  if (file_.read(reinterpret_cast<uint8_t *>(slot.pixels), pixBytes) !=
      pixBytes) {
    slot.valid = false;
    return false;
  }
  if (!file_.seek(aOff)) {
    slot.valid = false;
    return false;
  }
  if (file_.read(slot.alpha, alphaBytes) != alphaBytes) {
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

bool StreamedEmojiAtlas::ensureCache(const ColorEmojiGlyph &g, CacheSlot *&out) {
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

bool StreamedEmojiAtlas::draw(Display &display, uint32_t cp, int16_t baselineX,
                        int16_t baselineY, int16_t drawPx) {
  const ColorEmojiGlyph *g = find(cp);
  if (!g) return false;
  CacheSlot *slot = nullptr;
  if (!ensureCache(*g, slot) || !slot) return false;
  ColorEmojiDraw::draw(display, slot->atlas, slot->glyph, baselineX, baselineY,
                       drawPx);
  return true;
}
