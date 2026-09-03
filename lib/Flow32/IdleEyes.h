#pragma once

#include "Canvas.h"
#include "Display.h"
#include "Rect.h"

#include <stdint.h>

/**
 * Idle screensaver — a pair of mono green rounded-box eyes that blink on a
 * black field.
 *
 * Deliberately not an App: it must not take a launcher slot, own a page, or
 * disturb the foreground app's state while it shows. Flow32 drives it from
 * tick() once FlowConfig::idleEyes() sets a timeout, and the foreground app
 * keeps running underneath — it is only painted over.
 */
class IdleEyes {
public:
  /** Re-arm the blink timer. Call each time the screensaver appears. */
  void reset();

  /** Animate and paint the whole panel. */
  void frame(Canvas &canvas, const Rect &panel, float dt);

private:
  void updateBlink(float dt);
  void draw(Display &d, const Rect &vp) const;

  float blinkAmt_ = 0.f;
  float nextBlinkIn_ = 1.2f;
  int8_t blinkDir_ = 0;
  uint8_t blinksLeft_ = 0;
};
