#pragma once

#include "Adafruit_ST77xx.h"
#include "SPI.h"

class Adafruit_ST7789 : public Adafruit_ST77xx {
public:
  Adafruit_ST7789(SPIClass *, int, int, int) {}
  void init(uint16_t, uint16_t) {}
};
