#include "Storage.h"

#include <SD.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <string.h>

Storage::Storage(const StorageConfig &cfg) : cfg_(cfg) {}

bool Storage::begin() {
  if (ready_) return true;
  if (!cfg_.mountPoint || !cfg_.mountPoint[0]) return false;

  if (cfg_.bus == SdBus::Spi) {
    if (cfg_.pinCs < 0 || cfg_.pinSclk < 0 || cfg_.pinMosi < 0 ||
        cfg_.pinMiso < 0) {
      Serial.println("Storage: SPI pins incomplete");
      return false;
    }
    SPI.begin(cfg_.pinSclk, cfg_.pinMiso, cfg_.pinMosi, -1);
    if (!SD.begin(cfg_.pinCs, SPI, cfg_.spiHz, cfg_.mountPoint)) {
      Serial.printf("Storage: SD SPI mount failed (%s)\n", cfg_.id);
      return false;
    }
  } else {
    if (cfg_.pinClk < 0 || cfg_.pinCmd < 0 || cfg_.pinD0 < 0) {
      Serial.println("Storage: SDMMC pins incomplete");
      return false;
    }
    if (cfg_.sdmmc1bit || cfg_.pinD1 < 0) {
      SD_MMC.setPins(cfg_.pinClk, cfg_.pinCmd, cfg_.pinD0);
    } else {
      SD_MMC.setPins(cfg_.pinClk, cfg_.pinCmd, cfg_.pinD0, cfg_.pinD1,
                     cfg_.pinD2, cfg_.pinD3);
    }
    // mode1bit = true → 1-bit SDMMC
    if (!SD_MMC.begin(cfg_.mountPoint, cfg_.sdmmc1bit)) {
      Serial.printf("Storage: SDMMC mount failed (%s) clk=%d cmd=%d d0=%d\n",
                    cfg_.id, cfg_.pinClk, cfg_.pinCmd, cfg_.pinD0);
      return false;
    }
  }

  ready_ = true;
  Serial.printf("Storage: mounted %s at %s\n", cfg_.id, cfg_.mountPoint);
  return true;
}

void Storage::end() {
  if (!ready_) return;
  if (cfg_.bus == SdBus::Spi) {
    SD.end();
  } else {
    SD_MMC.end();
  }
  ready_ = false;
}

uint8_t Storage::cardType() const {
  if (!ready_) return CARD_NONE;
  return (cfg_.bus == SdBus::Spi) ? SD.cardType() : SD_MMC.cardType();
}

uint64_t Storage::cardSizeBytes() const {
  if (!ready_) return 0;
  return (cfg_.bus == SdBus::Spi) ? SD.cardSize() : SD_MMC.cardSize();
}

uint64_t Storage::usedBytes() const {
  if (!ready_) return 0;
  return (cfg_.bus == SdBus::Spi) ? SD.usedBytes() : SD_MMC.usedBytes();
}

bool Storage::exists(const char *path) const {
  if (!ready_ || !path) return false;
  return (cfg_.bus == SdBus::Spi) ? SD.exists(path) : SD_MMC.exists(path);
}

File Storage::open(const char *path, const char *mode) const {
  if (!ready_ || !path) return File();
  return (cfg_.bus == SdBus::Spi) ? SD.open(path, mode) : SD_MMC.open(path, mode);
}

bool Storage::absPath(const char *rel, char *out, size_t outLen) const {
  if (!out || outLen == 0) return false;
  const char *r = rel ? rel : "";
  while (*r == '/') r++;
  // Card-rooted path for SD / SD_MMC (mount point is only for begin()).
  const int n = snprintf(out, outLen, "/%s", r);
  return n > 0 && static_cast<size_t>(n) < outLen;
}

void Storage::printInfo(Stream &out) const {
  if (!ready_) {
    out.println("Storage: not mounted");
    return;
  }
  const uint8_t t = cardType();
  const char *type = "NONE";
  if (t == CARD_MMC) type = "MMC";
  else if (t == CARD_SD) type = "SD";
  else if (t == CARD_SDHC) type = "SDHC";

  out.printf("Storage[%s] type=%s size=%.1f MB used=%.1f MB mount=%s\n", cfg_.id,
             type, cardSizeBytes() / (1024.0 * 1024.0),
             usedBytes() / (1024.0 * 1024.0), cfg_.mountPoint);

  File root = open("/");
  if (!root || !root.isDirectory()) {
    out.println("Storage: cannot open root");
    return;
  }
  out.println("Storage: root listing");
  File f = root.openNextFile();
  int n = 0;
  while (f && n < 32) {
    out.printf("  %s%s  %u\n", f.name(), f.isDirectory() ? "/" : "",
               static_cast<unsigned>(f.size()));
    f = root.openNextFile();
    n++;
  }
  if (f) out.println("  …");
}
