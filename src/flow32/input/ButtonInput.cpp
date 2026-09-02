#include "flow32/input/ButtonInput.h"

void ButtonInput::onEmit(void *ctx, UIKey key, UIKeyPhase phase) {
  static_cast<ButtonInput *>(ctx)->emit(key, phase);
}

void ButtonInput::configure(int pin) const {
  if (pin >= 0) pinMode(pin, config_.pinModeValue);
}

void ButtonInput::begin() {
  tracker_.setEmit(onEmit, this);
  configure(config_.up);
  configure(config_.down);
  configure(config_.left);
  configure(config_.right);
  configure(config_.select);
  configure(config_.back);
  const uint32_t now = millis();
  for (uint8_t i = 0; i < 6; ++i) buttons_[i].reset(false, now);
}

bool ButtonInput::pressed(int pin) const {
  if (pin < 0) return false;
  const bool high = digitalRead(pin) == HIGH;
  return config_.activeLow ? !high : high;
}

void ButtonInput::update(uint8_t index, UIKey key, int pin, uint32_t nowMs) {
  const bool raw = pressed(pin);
  if (buttons_[index].update(raw, nowMs, config_.debounceMs)) {
    tracker_.setPressed(key, buttons_[index].stablePressed(), nowMs);
  }
}

void ButtonInput::poll(uint32_t nowMs) {
  update(0, UIKey::Up, config_.up, nowMs);
  update(1, UIKey::Down, config_.down, nowMs);
  update(2, UIKey::Left, config_.left, nowMs);
  update(3, UIKey::Right, config_.right, nowMs);
  update(4, UIKey::Select, config_.select, nowMs);
  update(5, UIKey::Back, config_.back, nowMs);
  tracker_.poll(nowMs);
}
