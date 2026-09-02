#include "Arduino.h"
#include "Adafruit_ST77xx.h"
#include "SD.h"
#include "SD_MMC.h"
#include "SPI.h"
#include "LittleFS.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"

HardwareSerial Serial;
SPIClass SPI;
SDClass SD;
SDMMCClass SD_MMC;
LittleFSClass LittleFS;
const GFXfont FreeSans9pt7b{};
const GFXfont FreeSansBold12pt7b{};

namespace arduino_stub {
int digitalValues[64] = {};
int analogValues[64] = {};
uint32_t nowMs = 0;
} // namespace arduino_stub

namespace st77xx_stub {
int startWrites = 0;
int endWrites = 0;
std::vector<Window> windows;
std::vector<uint32_t> pixelCounts;
std::vector<uint16_t> firstPixels;

void reset() {
  startWrites = 0;
  endWrites = 0;
  windows.clear();
  pixelCounts.clear();
  firstPixels.clear();
}
} // namespace st77xx_stub
