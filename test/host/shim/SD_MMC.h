#pragma once

#include "SD.h"

class SDMMCClass : public fs::FS {
public:
  void setPins(int, int, int) {}
  void setPins(int, int, int, int, int, int) {}
  bool begin(const char *, bool) { return true; }
  void end() {}
  uint8_t cardType() const { return CARD_MMC; }
  uint64_t cardSize() const { return 0; }
  uint64_t usedBytes() const { return 0; }
};

extern SDMMCClass SD_MMC;
