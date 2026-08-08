#include "IconSd.h"
#include "Display.h"

#include <esp_heap_caps.h>
#include <string.h>

namespace {

constexpr char kMagic[4] = {'F', '3', '2', 'I'};
constexpr uint16_t kVersion = 1;
constexpr size_t kHeaderBytes = 4 + 2 + 2 + 2 + 2 + 4 + 4; // 20
constexpr size_t kGlyphRecBytes = 18;

struct __attribute__((packed)) GlyphRec {
  uint16_t id;
  uint16_t nameOffset;
  uint32_t alphaOffset;
  uint16_t width;
  uint16_t height;
  uint16_t xAdvance;
  int16_t xOffset;
  int16_t yOffset;
};
static_assert(sizeof(GlyphRec) == kGlyphRecBytes, "GlyphRec size");

void *allocPreferPsram(size_t bytes) {
  void *p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  return p;
}

} // namespace

bool IconSd::begin(Storage &storage, const char *relPath) {
  end();
  if (!storage.ready()) {
    Serial.println("IconSd: storage not ready");
    return false;
  }

  if (!storage.absPath(relPath ? relPath : kDefaultRelPath, path_,
                       sizeof(path_))) {
    Serial.println("IconSd: path too long");
    return false;
  }

  file_ = storage.open(path_, FILE_READ);
  if (!file_) {
    Serial.printf("IconSd: missing %s\n", path_);
    return false;
  }

  char magic[4];
  if (file_.read(reinterpret_cast<uint8_t *>(magic), 4) != 4 ||
      memcmp(magic, kMagic, 4) != 0) {
    Serial.println("IconSd: bad magic");
    end();
    return false;
  }

  uint16_t version = 0, baked = 0, count = 0, flags = 0;
  uint32_t nameBytes = 0, alphaBytes = 0;
  if (file_.read(reinterpret_cast<uint8_t *>(&version), 2) != 2 ||
      file_.read(reinterpret_cast<uint8_t *>(&baked), 2) != 2 ||
      file_.read(reinterpret_cast<uint8_t *>(&count), 2) != 2 ||
      file_.read(reinterpret_cast<uint8_t *>(&flags), 2) != 2 ||
      file_.read(reinterpret_cast<uint8_t *>(&nameBytes), 4) != 4 ||
      file_.read(reinterpret_cast<uint8_t *>(&alphaBytes), 4) != 4) {
    Serial.println("IconSd: truncated header");
    end();
    return false;
  }
  (void)flags;
  if (version != kVersion || count == 0 || baked == 0) {
    Serial.printf("IconSd: unsupported ver=%u count=%u baked=%u\n", version,
                  count, baked);
    end();
    return false;
  }

  names_ = static_cast<char *>(allocPreferPsram(nameBytes + 1));
  if (!names_) {
    Serial.println("IconSd: OOM name table");
    end();
    return false;
  }
  if (file_.read(reinterpret_cast<uint8_t *>(names_), nameBytes) !=
      static_cast<int>(nameBytes)) {
    Serial.println("IconSd: truncated names");
    end();
    return false;
  }
  names_[nameBytes] = '\0';

  const size_t glyphBytes = static_cast<size_t>(count) * sizeof(IconGlyph);
  glyphs_ = static_cast<IconGlyph *>(allocPreferPsram(glyphBytes));
  if (!glyphs_) {
    Serial.println("IconSd: OOM glyph index");
    end();
    return false;
  }

  for (uint16_t i = 0; i < count; i++) {
    GlyphRec rec{};
    if (file_.read(reinterpret_cast<uint8_t *>(&rec), sizeof(rec)) !=
        static_cast<int>(sizeof(rec))) {
      Serial.println("IconSd: truncated glyphs");
      end();
      return false;
    }
    glyphs_[i].id = rec.id;
    glyphs_[i].nameOffset = rec.nameOffset;
    glyphs_[i].alphaOffset = rec.alphaOffset;
    glyphs_[i].width = rec.width;
    glyphs_[i].height = rec.height;
    glyphs_[i].xAdvance = rec.xAdvance;
    glyphs_[i].xOffset = rec.xOffset;
    glyphs_[i].yOffset = rec.yOffset;
  }

  alphaFileOff_ = static_cast<uint32_t>(kHeaderBytes + nameBytes +
                                        static_cast<uint32_t>(count) *
                                            kGlyphRecBytes);
  bakedSize_ = baked;
  count_ = count;
  storage_ = &storage;
  ready_ = true;

  Serial.printf(
      "IconSd: %u icons @%upx from %s (%.1f KB names+index, %.1f MB alpha on SD)\n",
      count_, bakedSize_, path_,
      (nameBytes + glyphBytes) / 1024.0f, alphaBytes / (1024.0f * 1024.0f));
  // Keep file open for streaming (same as ColorEmojiSd).
  return true;
}

void IconSd::end() {
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
  alphaFileOff_ = 0;
  path_[0] = '\0';
  useTick_ = 0;
}

const char *IconSd::nameOf(const IconGlyph &g) const {
  if (!names_) return "";
  return names_ + g.nameOffset;
}

int IconSd::findNameIndex(const char *name) const {
  if (!ready_ || !name || !names_ || !glyphs_) return -1;
  // Linear scan is fine for curated sets; --all still ~1.5k names.
  for (uint16_t i = 0; i < count_; i++) {
    if (strcmp(names_ + glyphs_[i].nameOffset, name) == 0) return i;
  }
  return -1;
}

const IconGlyph *IconSd::findByName(const char *name) const {
  const int i = findNameIndex(name);
  return i >= 0 ? &glyphs_[i] : nullptr;
}

const IconGlyph *IconSd::findById(uint16_t id) const {
  if (!ready_ || !glyphs_) return nullptr;
  // Packer assigns id == index.
  if (id < count_ && glyphs_[id].id == id) return &glyphs_[id];
  for (uint16_t i = 0; i < count_; i++) {
    if (glyphs_[i].id == id) return &glyphs_[i];
  }
  return nullptr;
}

const IconGlyph *IconSd::findByCp(uint32_t cp) const {
  if (!IconDraw::isIconCp(cp)) return nullptr;
  return findById(IconDraw::idFromCp(cp));
}

int16_t IconSd::advance(uint32_t cp, int16_t drawPx) const {
  const IconGlyph *g = findByCp(cp);
  if (!g || bakedSize_ == 0) return 0;
  return IconDraw::advance(*g, bakedSize_, drawPx);
}

size_t IconSd::utf8(const char *name, char *buf, size_t cap) const {
  const IconGlyph *g = findByName(name);
  if (!g) return 0;
  return IconDraw::encodeUtf8(IconDraw::cpFromId(g->id), buf, cap);
}

uint32_t IconSd::codepoint(const char *name) const {
  const IconGlyph *g = findByName(name);
  if (!g) return 0;
  return IconDraw::cpFromId(g->id);
}

int IconSd::findSlot(uint16_t id) const {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (slots_[i].valid && slots_[i].id == id) return i;
  }
  return -1;
}

int IconSd::pickVictim() const {
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

bool IconSd::loadSlot(CacheSlot &slot, const IconGlyph &g) {
  if (!storage_ || !storage_->ready()) return false;
  if (!file_) {
    file_ = storage_->open(path_, FILE_READ);
    if (!file_) return false;
  }

  const size_t pixCount = static_cast<size_t>(g.width) * g.height;
  const size_t alphaBytes = (pixCount + 1) / 2;

  if (alphaBytes > slot.alphaCap) {
    if (slot.alpha) free(slot.alpha);
    slot.alpha = static_cast<uint8_t *>(allocPreferPsram(alphaBytes));
    slot.alphaCap = slot.alpha ? alphaBytes : 0;
  }
  if (!slot.alpha) {
    Serial.println("IconSd: OOM glyph cache");
    slot.valid = false;
    return false;
  }

  const uint32_t aOff = alphaFileOff_ + g.alphaOffset;
  if (!file_.seek(aOff)) {
    slot.valid = false;
    return false;
  }
  if (file_.read(slot.alpha, alphaBytes) != static_cast<int>(alphaBytes)) {
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

bool IconSd::ensureCache(const IconGlyph &g, CacheSlot *&out) {
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

bool IconSd::draw(Display &display, uint32_t cp, int16_t baselineX,
                  int16_t baselineY, int16_t drawPx, uint16_t color) {
  const IconGlyph *g = findByCp(cp);
  if (!g) return false;
  CacheSlot *slot = nullptr;
  if (!ensureCache(*g, slot) || !slot) return false;
  IconDraw::draw(display, slot->atlas, slot->glyph, baselineX, baselineY,
                 drawPx, color);
  return true;
}
