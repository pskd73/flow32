#pragma once

#include <Arduino.h>
#include "../ui/UIEvent.h"

/**
 * Converts pressed/released digital keys into Down / Hold / Up events.
 * Shared by SerialInput (press/release protocol) and future JoystickInput.
 */
class KeyTracker {
public:
  uint16_t holdDelayMs = 400;
  uint16_t holdRepeatMs = 120;

  using EmitFn = void (*)(void *ctx, UIKey key, UIKeyPhase phase);

  void setEmit(EmitFn fn, void *ctx) {
    emit_ = fn;
    ctx_ = ctx;
  }

  void setPressed(UIKey key, bool pressed, uint32_t nowMs);
  void poll(uint32_t nowMs);
  bool isPressed(UIKey key) const;

private:
  static constexpr uint8_t kCount = 6;

  struct Slot {
    bool pressed = false;
    bool sawDown = false;
    uint32_t pressedAt = 0;
    uint32_t lastHoldAt = 0;
  };

  Slot slots_[kCount];
  EmitFn emit_ = nullptr;
  void *ctx_ = nullptr;

  static uint8_t index(UIKey key);
  void fire(UIKey key, UIKeyPhase phase);
};
