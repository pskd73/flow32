#include "flow32/assets/StreamedIconAtlas.h"
#include "flow32/graphics/Display.h"

#include <esp_heap_caps.h>
#include <string.h>

namespace {

constexpr char kMagic[4] = {'F', '3', '2', 'I'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderBytes = 4 + 2 + 2 + 2 + 2 + 4 + 4; // 20
constexpr size_t kGlyphRecBytes = 18;
constexpr uint16_t kMaxGlyphs = 4096;
constexpr uint16_t kMaxBakedSize = 256;
constexpr uint32_t kMaxNameBytes = 256U * 1024U;

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

bool StreamedIconAtlas::begin(AssetStore &storage, const char *relPath) {
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
  const uint16_t baked = readU16(header + 6);
  const uint16_t count = readU16(header + 8);
  const uint16_t flags = readU16(header + 10);
  const uint32_t nameBytes = readU32(header + 12);
  const uint32_t alphaBytes = readU32(header + 16);
  const uint64_t alphaOffset =
      kHeaderBytes + static_cast<uint64_t>(nameBytes) +
      static_cast<uint64_t>(count) * kGlyphRecBytes;
  const uint64_t expectedBytes = alphaOffset + alphaBytes;
  if (version != kVersion || count == 0 || count > kMaxGlyphs || baked == 0 ||
      baked > kMaxBakedSize ||
      flags != 0 || nameBytes == 0 || nameBytes > kMaxNameBytes ||
      alphaBytes == 0 || expectedBytes != sourceBytes) {
    end();
    return false;
  }

  names_ = static_cast<char *>(allocPreferPsram(nameBytes + 1));
  if (!names_) {
    end();
    return false;
  }
  if (!readExact(file_, names_, nameBytes)) {
    end();
    return false;
  }
  names_[nameBytes] = '\0';

  const size_t glyphBytes = static_cast<size_t>(count) * sizeof(IconGlyph);
  glyphs_ = static_cast<IconGlyph *>(allocPreferPsram(glyphBytes));
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
    IconGlyph &glyph = glyphs_[i];
    glyph.id = readU16(raw);
    glyph.nameOffset = readU16(raw + 2);
    glyph.alphaOffset = readU32(raw + 4);
    glyph.width = readU16(raw + 8);
    glyph.height = readU16(raw + 10);
    glyph.xAdvance = readU16(raw + 12);
    glyph.xOffset = static_cast<int16_t>(readU16(raw + 14));
    glyph.yOffset = static_cast<int16_t>(readU16(raw + 16));
    const uint64_t pixels =
        static_cast<uint64_t>(glyph.width) * glyph.height;
    const uint64_t requiredAlpha = (pixels + 1) / 2;
    if (glyph.id != i || glyph.nameOffset >= nameBytes || glyph.width == 0 ||
        glyph.height == 0 || glyph.width > baked || glyph.height > baked ||
        requiredAlpha > alphaBytes ||
        glyph.alphaOffset > alphaBytes - requiredAlpha ||
        !memchr(names_ + glyph.nameOffset, '\0',
                nameBytes - glyph.nameOffset)) {
      end();
      return false;
    }
  }

  fileBytes_ = static_cast<uint32_t>(sourceBytes);
  nameBytes_ = nameBytes;
  alphaBytes_ = alphaBytes;
  alphaFileOff_ = static_cast<uint32_t>(alphaOffset);
  bakedSize_ = baked;
  count_ = count;
  storage_ = &storage;
  ready_ = true;

  // Keep file open for streaming (same as StreamedEmojiAtlas).
  return true;
}

void StreamedIconAtlas::end() {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (slots_[i].alpha) {
      free(slots_[i].alpha);
      slots_[i].alpha = nullptr;
    }
    slots_[i].alphaCap = 0;
    slots_[i].valid = false;
  }
  if (file_) file_.close();
  if (names_) {
    free(names_);
    names_ = nullptr;
  }
  if (glyphs_) {
    free(glyphs_);
    glyphs_ = nullptr;
  }
  ready_ = false;
  storage_ = nullptr;
  bakedSize_ = 0;
  count_ = 0;
  fileBytes_ = 0;
  nameBytes_ = 0;
  alphaBytes_ = 0;
  alphaFileOff_ = 0;
  path_[0] = '\0';
  useTick_ = 0;
}

const char *StreamedIconAtlas::nameOf(const IconGlyph &g) const {
  if (!names_ || g.nameOffset >= nameBytes_) return "";
  return names_ + g.nameOffset;
}

int StreamedIconAtlas::findNameIndex(const char *name) const {
  if (!ready_ || !name || !names_ || !glyphs_) return -1;
  // Linear scan is fine for curated sets; --all still ~1.5k names.
  for (uint16_t i = 0; i < count_; i++) {
    if (strcmp(names_ + glyphs_[i].nameOffset, name) == 0) return i;
  }
  return -1;
}

const IconGlyph *StreamedIconAtlas::findByName(const char *name) const {
  const int i = findNameIndex(name);
  return i >= 0 ? &glyphs_[i] : nullptr;
}

const IconGlyph *StreamedIconAtlas::findById(uint16_t id) const {
  if (!ready_ || !glyphs_) return nullptr;
  // Packer assigns id == index.
  if (id < count_ && glyphs_[id].id == id) return &glyphs_[id];
  for (uint16_t i = 0; i < count_; i++) {
    if (glyphs_[i].id == id) return &glyphs_[i];
  }
  return nullptr;
}

const IconGlyph *StreamedIconAtlas::findByCp(uint32_t cp) const {
  if (!IconDraw::isIconCp(cp)) return nullptr;
  return findById(IconDraw::idFromCp(cp));
}

int16_t StreamedIconAtlas::advance(uint32_t cp, int16_t drawPx) const {
  const IconGlyph *g = findByCp(cp);
  if (!g || bakedSize_ == 0) return 0;
  return IconDraw::advance(*g, bakedSize_, drawPx);
}

size_t StreamedIconAtlas::utf8(const char *name, char *buf, size_t cap) const {
  const IconGlyph *g = findByName(name);
  if (!g) return 0;
  return IconDraw::encodeUtf8(IconDraw::cpFromId(g->id), buf, cap);
}

uint32_t StreamedIconAtlas::codepoint(const char *name) const {
  const IconGlyph *g = findByName(name);
  if (!g) return 0;
  return IconDraw::cpFromId(g->id);
}

int StreamedIconAtlas::findSlot(uint16_t id) const {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (slots_[i].valid && slots_[i].id == id) return i;
  }
  return -1;
}

int StreamedIconAtlas::pickVictim() const {
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

bool StreamedIconAtlas::loadSlot(CacheSlot &slot, const IconGlyph &g) {
  if (!storage_ || !storage_->ready()) return false;
  if (!file_) {
    file_ = storage_->open(path_, FILE_READ);
    if (!file_) return false;
  }

  const size_t pixCount = static_cast<size_t>(g.width) * g.height;
  const size_t alphaBytes = (pixCount + 1) / 2;
  if (alphaBytes > alphaBytes_ || g.alphaOffset > alphaBytes_ - alphaBytes) {
    return false;
  }

  if (alphaBytes > slot.alphaCap) {
    if (slot.alpha) free(slot.alpha);
    slot.alpha = static_cast<uint8_t *>(allocPreferPsram(alphaBytes));
    slot.alphaCap = slot.alpha ? alphaBytes : 0;
  }
  if (!slot.alpha) {
    slot.valid = false;
    return false;
  }

  const uint64_t absolute = static_cast<uint64_t>(alphaFileOff_) + g.alphaOffset;
  if (absolute + alphaBytes > fileBytes_) return false;
  const uint32_t aOff = static_cast<uint32_t>(absolute);
  if (!file_.seek(aOff)) {
    slot.valid = false;
    return false;
  }
  if (file_.read(slot.alpha, alphaBytes) != alphaBytes) {
    slot.valid = false;
    return false;
  }

  slot.glyph = g;
  slot.glyph.alphaOffset = 0;
  slot.atlas.alpha = slot.alpha;
  slot.atlas.names = names_;
  slot.atlas.glyphs = &slot.glyph;
  slot.atlas.count = 1;
  slot.atlas.bakedSize = bakedSize_;
  slot.id = g.id;
  slot.valid = true;
  return true;
}

bool StreamedIconAtlas::ensureCache(const IconGlyph &g, CacheSlot *&out) {
  int idx = findSlot(g.id);
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

bool StreamedIconAtlas::draw(Display &display, uint32_t cp, int16_t baselineX,
                  int16_t baselineY, int16_t drawPx, uint16_t color) {
  const IconGlyph *g = findByCp(cp);
  if (!g) return false;
  CacheSlot *slot = nullptr;
  if (!ensureCache(*g, slot) || !slot) return false;
  IconDraw::draw(display, slot->atlas, slot->glyph, baselineX, baselineY,
                 drawPx, color);
  return true;
}
