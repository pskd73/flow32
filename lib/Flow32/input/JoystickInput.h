#pragma once

#include "InputSource.h"
#include "KeyTracker.h"

/**
 * Future: analog/digital joystick → same UIEvents via KeyTracker.
 *
 * Example wiring later:
 *   JoystickInput stick(pinX, pinY, pinSelect);
 *   input.add(stick);
 *
 * Read axes each poll(), apply deadzone, call
 *   tracker_.setPressed(UIKey::Up/Down/Left/Right/Select/Back, …)
 * Hold repeats come free from KeyTracker.
 */
class JoystickInput : public InputSource {
public:
  void begin() override {}
  void poll(uint32_t nowMs) override { tracker_.poll(nowMs); }

  KeyTracker &tracker() { return tracker_; }

private:
  KeyTracker tracker_;
};
