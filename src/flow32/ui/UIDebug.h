#pragma once

#include <stdint.h>

/** Global UI debug toggles. */
struct UIDebug {
  static bool borders;
  static uint16_t borderColor; // RGB565
};
