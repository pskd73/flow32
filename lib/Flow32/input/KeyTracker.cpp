#include "KeyTracker.h"

uint8_t KeyTracker::index(UIKey key) {
  return static_cast<uint8_t>(key);
}

bool KeyTracker::isPressed(UIKey key) const {
  const uint8_t i = index(key);
  if (i >= kCount) return false;
  return slots_[i].pressed;
}

void KeyTracker::fire(UIKey key, UIKeyPhase phase) {
  if (emit_) emit_(ctx_, key, phase);
}

void KeyTracker::setPressed(UIKey key, bool pressed, uint32_t nowMs) {
  const uint8_t i = index(key);
  if (i >= kCount) return;
  Slot &s = slots_[i];
  if (pressed == s.pressed) return;

  s.pressed = pressed;
  if (pressed) {
    s.sawDown = true;
    s.pressedAt = nowMs;
    s.lastHoldAt = nowMs;
    fire(key, UIKeyPhase::Down);
  } else {
    fire(key, UIKeyPhase::Up);
    s.sawDown = false;
  }
}

void KeyTracker::poll(uint32_t nowMs) {
  for (uint8_t i = 0; i < kCount; i++) {
    Slot &s = slots_[i];
    if (!s.pressed) continue;
    const UIKey key = static_cast<UIKey>(i);
    if (!s.sawDown) continue;
    if (nowMs - s.pressedAt < holdDelayMs) continue;
    if (nowMs - s.lastHoldAt < holdRepeatMs) continue;
    s.lastHoldAt = nowMs;
    fire(key, UIKeyPhase::Hold);
  }
}
