#pragma once

#include <stdint.h>

enum class UIKey : uint8_t {
  Up,
  Down,
  Left,
  Right,
  Select,
  Back,
};

enum class UIKeyPhase : uint8_t {
  Down,  // first press
  Hold,  // repeat while held
  Up,    // release
};

struct UIEvent {
  UIKey key = UIKey::Select;
  UIKeyPhase phase = UIKeyPhase::Down;
  bool handled = false;

  UIEvent() = default;
  UIEvent(UIKey k, UIKeyPhase p) : key(k), phase(p) {}
};
