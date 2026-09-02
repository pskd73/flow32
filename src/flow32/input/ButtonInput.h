#pragma once

#include "flow32/input/InputSource.h"
#include "flow32/input/KeyTracker.h"
#include "flow32/input/ButtonDebouncer.h"
#include "Flow32Config.h"

/** Six optional digital navigation buttons. Use -1 for an unwired action. */
struct ButtonInputConfig {
  int up = -1;
  int down = -1;
  int left = -1;
  int right = -1;
  int select = -1;
  int back = -1;
  bool activeLow = true;
  uint8_t pinModeValue = INPUT_PULLUP;
  uint16_t debounceMs = FLOW32_BUTTON_DEBOUNCE_MS;
};

class ButtonInput final : public InputSource {
public:
  explicit ButtonInput(const ButtonInputConfig &config) : config_(config) {}

  void begin() override;
  void poll(uint32_t nowMs) override;
  KeyTracker &tracker() { return tracker_; }

private:
  ButtonInputConfig config_;
  KeyTracker tracker_;
  ButtonDebouncer buttons_[6];

  bool pressed(int pin) const;
  void configure(int pin) const;
  void update(uint8_t index, UIKey key, int pin, uint32_t nowMs);
  static void onEmit(void *ctx, UIKey key, UIKeyPhase phase);
};
