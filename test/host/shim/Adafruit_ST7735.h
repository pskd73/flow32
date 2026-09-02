#pragma once

#include "Adafruit_ST77xx.h"
#include "SPI.h"

#define INITR_18BLACKTAB 2

class Adafruit_ST7735 : public Adafruit_ST77xx {
public:
  Adafruit_ST7735(SPIClass *, int, int, int) {}
  void initR(uint8_t) {}
};
