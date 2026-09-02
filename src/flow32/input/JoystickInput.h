#pragma once

#include "flow32/input/InputSource.h"
#include "flow32/input/KeyTracker.h"
#include "flow32/input/ButtonDebouncer.h"
#include "Flow32Config.h"

/** Wiring and calibration for a two-axis analog joystick. */
struct JoystickConfig {
  int pinX = -1;
  int pinY = -1;
  int pinSelect = -1;
  int pinBack = -1;
  int centerX = 2048;
  int centerY = 2048;
  int deadZoneEnter = 700;
  int deadZoneExit = 500;
  bool invertX = false;
  bool invertY = false;
  bool buttonsActiveLow = true;
  uint8_t buttonMode = INPUT_PULLUP;
  uint16_t buttonDebounceMs = FLOW32_BUTTON_DEBOUNCE_MS;
};

/** Analog joystick and optional Select/Back buttons → UI events. */
class JoystickInput final : public InputSource {
public:
  explicit JoystickInput(const JoystickConfig &config) : config_(config) {}

  void begin() override;
  void poll(uint32_t nowMs) override;
  bool validConfig() const;

  KeyTracker &tracker() { return tracker_; }

private:
  JoystickConfig config_;
  KeyTracker tracker_;
  ButtonDebouncer selectButton_;
  ButtonDebouncer backButton_;
  int8_t xDirection_ = 0;
  int8_t yDirection_ = 0;

  bool buttonPressed(int pin) const;
  int8_t updateAxis(int value, int center, int8_t direction) const;
  void updateButton(ButtonDebouncer &button, UIKey key, int pin,
                    uint32_t nowMs);
  static void onEmit(void *ctx, UIKey key, UIKeyPhase phase);
};
