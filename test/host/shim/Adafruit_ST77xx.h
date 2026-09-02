#pragma once

#include "Arduino.h"

#include <vector>

namespace st77xx_stub {
struct Window {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
};

extern int startWrites;
extern int endWrites;
extern std::vector<Window> windows;
extern std::vector<uint32_t> pixelCounts;
extern std::vector<uint16_t> firstPixels;
void reset();
} // namespace st77xx_stub

class Adafruit_ST77xx {
public:
  virtual ~Adafruit_ST77xx() = default;
  void setSPISpeed(uint32_t) {}
  void setRotation(uint8_t) {}
  void startWrite() { ++st77xx_stub::startWrites; }
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t width,
                     uint16_t height) {
    st77xx_stub::windows.push_back(st77xx_stub::Window{x, y, width, height});
  }
  void writePixels(uint16_t *pixels, uint32_t count) {
    st77xx_stub::pixelCounts.push_back(count);
    st77xx_stub::firstPixels.push_back(pixels && count ? pixels[0] : 0);
  }
  void endWrite() { ++st77xx_stub::endWrites; }
};
