#include "flow32/input/JoystickInput.h"
#include "flow32/core/FlowError.h"

void JoystickInput::onEmit(void *ctx, UIKey key, UIKeyPhase phase) {
  static_cast<JoystickInput *>(ctx)->emit(key, phase);
}

void JoystickInput::begin() {
  if (!validConfig()) flowFatal(FlowError::InvalidInputConfiguration);
  tracker_.setEmit(onEmit, this);
  if (config_.pinSelect >= 0) pinMode(config_.pinSelect, config_.buttonMode);
  if (config_.pinBack >= 0) pinMode(config_.pinBack, config_.buttonMode);
  const uint32_t now = millis();
  selectButton_.reset(false, now);
  backButton_.reset(false, now);
  xDirection_ = 0;
  yDirection_ = 0;
}

bool JoystickInput::validConfig() const {
  return config_.centerX >= 0 && config_.centerY >= 0 &&
         config_.deadZoneEnter > 0 && config_.deadZoneExit >= 0 &&
         config_.deadZoneExit < config_.deadZoneEnter;
}

bool JoystickInput::buttonPressed(int pin) const {
  if (pin < 0) return false;
  const bool high = digitalRead(pin) == HIGH;
  return config_.buttonsActiveLow ? !high : high;
}

int8_t JoystickInput::updateAxis(int value, int center,
                                 int8_t direction) const {
  const int enter = config_.deadZoneEnter;
  const int exit = config_.deadZoneExit;
  if (direction < 0) {
    if (value <= center - exit) return -1;
  } else if (direction > 0) {
    if (value >= center + exit) return 1;
  }
  if (value < center - enter) return -1;
  if (value > center + enter) return 1;
  return 0;
}

void JoystickInput::updateButton(ButtonDebouncer &button, UIKey key, int pin,
                                 uint32_t nowMs) {
  if (button.update(buttonPressed(pin), nowMs, config_.buttonDebounceMs)) {
    tracker_.setPressed(key, button.stablePressed(), nowMs);
  }
}

void JoystickInput::poll(uint32_t nowMs) {
  int x = config_.pinX >= 0 ? analogRead(config_.pinX) : config_.centerX;
  int y = config_.pinY >= 0 ? analogRead(config_.pinY) : config_.centerY;
  if (config_.invertX) x = 2 * config_.centerX - x;
  if (config_.invertY) y = 2 * config_.centerY - y;

  xDirection_ = updateAxis(x, config_.centerX, xDirection_);
  yDirection_ = updateAxis(y, config_.centerY, yDirection_);
  tracker_.setPressed(UIKey::Left, xDirection_ < 0, nowMs);
  tracker_.setPressed(UIKey::Right, xDirection_ > 0, nowMs);
  tracker_.setPressed(UIKey::Up, yDirection_ < 0, nowMs);
  tracker_.setPressed(UIKey::Down, yDirection_ > 0, nowMs);
  updateButton(selectButton_, UIKey::Select, config_.pinSelect, nowMs);
  updateButton(backButton_, UIKey::Back, config_.pinBack, nowMs);
  tracker_.poll(nowMs);
}
