#pragma once

#include "FS.h"
#include "SPI.h"

#define CARD_NONE 0
#define CARD_MMC 1
#define CARD_SD 2
#define CARD_SDHC 3

class SDClass : public fs::FS {
public:
  bool begin(int, SPIClass &, uint32_t, const char *) { return true; }
  void end() {}
  uint8_t cardType() const { return CARD_SDHC; }
  uint64_t cardSize() const { return 0; }
  uint64_t usedBytes() const { return 0; }
};

extern SDClass SD;
