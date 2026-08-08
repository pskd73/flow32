#pragma once

#include <Flow32.h>

/** App-side panel profiles — not part of the Flow32 library. */

/** Flow32 1.83" rounded IPS — 240×284 visible. */
inline DisplayPanel Panel183() {
  DisplayPanel p;
  p.id = "Panel183";
  p.chip = PanelChip::ST7789;
  p.width = 240;
  p.height = 284;
  p.rotation = 0;
  p.cornerRadius = 44;
  p.gramWidth = 240;
  p.gramHeight = 320;
  p.panelYOffset = 36;
  p.spiHz = 80000000;
  p.uiScale = 1.0f;
  return p;
}

/**
 * 1.8" ST7735 SPI module — landscape 160×128 (1:1 FB).
 *
 * Wiring: VCC→3V3, GND→GND, CS→10, RESET→14, AO→9, SDA→11, SCK→12, LED→13
 */
inline DisplayPanel Panel18() {
  DisplayPanel p;
  p.id = "Panel18";
  p.chip = PanelChip::ST7735;
  p.width = 160;
  p.height = 128;
  p.rotation = 3; // landscape
  p.cornerRadius = 0;
  p.gramWidth = 128;
  p.gramHeight = 160;
  p.panelYOffset = 0;
  p.spiHz = 40000000;
  p.blActiveHigh = true;
  p.uiScale = 0.5f;
  return p;
}
