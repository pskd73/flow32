#pragma once

#include <Arduino.h>
#include "Style.h"

/** Shared palette for button (and later chip/badge) chrome. */
namespace Theme {

inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

inline uint16_t primary() { return rgb(255, 160, 90); }   // warm accent orange
inline uint16_t secondary() { return rgb(120, 140, 160); } // cool gray-blue
inline uint16_t accent() { return rgb(90, 200, 170); }     // mint

inline uint16_t onPrimary() { return rgb(40, 24, 20); }
inline uint16_t onSecondary() { return rgb(245, 248, 252); }
inline uint16_t onAccent() { return rgb(20, 36, 32); }

inline uint16_t softPrimary() { return rgb(72, 48, 40); }
inline uint16_t softSecondary() { return rgb(48, 56, 68); }
inline uint16_t softAccent() { return rgb(36, 64, 56); }

inline uint16_t focusRing() { return rgb(255, 248, 235); }

/** Typical page / content backdrop (for disabled 50% opacity mix). */
inline uint16_t surface() { return rgb(48, 32, 28); }

/** Blend RGB565 `c` toward black by `amount` (0 = unchanged, 1 = black). */
inline uint16_t dim(uint16_t c, float amount) {
  if (amount <= 0.f) return c;
  if (amount >= 1.f) return 0;
  const float keep = 1.f - amount;
  const uint8_t r = static_cast<uint8_t>((((c >> 11) & 0x1F) << 3) * keep);
  const uint8_t g = static_cast<uint8_t>((((c >> 5) & 0x3F) << 2) * keep);
  const uint8_t b = static_cast<uint8_t>(((c & 0x1F) << 3) * keep);
  return rgb(r, g, b);
}

/** Linear mix: t=0 → a, t=1 → b. t=0.5 ≈ 50% opacity of `a` over solid `b`. */
inline uint16_t lerp(uint16_t a, uint16_t b, float t) {
  if (t <= 0.f) return a;
  if (t >= 1.f) return b;
  const float ia = 1.f - t;
  const uint8_t ar = ((a >> 11) & 0x1F) << 3;
  const uint8_t ag = ((a >> 5) & 0x3F) << 2;
  const uint8_t ab = (a & 0x1F) << 3;
  const uint8_t br = ((b >> 11) & 0x1F) << 3;
  const uint8_t bg = ((b >> 5) & 0x3F) << 2;
  const uint8_t bb = (b & 0x1F) << 3;
  return rgb(static_cast<uint8_t>(ar * ia + br * t),
             static_cast<uint8_t>(ag * ia + bg * t),
             static_cast<uint8_t>(ab * ia + bb * t));
}

struct ButtonChrome {
  uint16_t fill = 0;
  bool hasFill = false;
  uint16_t border = 0;
  uint8_t borderWidth = 0;
  bool hasBorder = false;
  uint16_t label = 0xFFFF;
  uint16_t focus = 0xFFFF;
};

inline uint16_t brand(ButtonColor c) {
  switch (c) {
  case ButtonColor::Secondary:
    return secondary();
  case ButtonColor::Accent:
    return accent();
  case ButtonColor::Primary:
  default:
    return primary();
  }
}

inline uint16_t soft(ButtonColor c) {
  switch (c) {
  case ButtonColor::Secondary:
    return softSecondary();
  case ButtonColor::Accent:
    return softAccent();
  case ButtonColor::Primary:
  default:
    return softPrimary();
  }
}

inline uint16_t onSolid(ButtonColor c) {
  switch (c) {
  case ButtonColor::Secondary:
    return onSecondary();
  case ButtonColor::Accent:
    return onAccent();
  case ButtonColor::Primary:
  default:
    return onPrimary();
  }
}

inline ButtonChrome buttonChrome(ButtonColor color, ButtonVariant variant) {
  ButtonChrome out;
  out.focus = focusRing();
  const uint16_t b = brand(color);
  switch (variant) {
  case ButtonVariant::Solid:
    out.fill = b;
    out.hasFill = true;
    out.label = onSolid(color);
    out.hasBorder = false;
    break;
  case ButtonVariant::Outline:
    out.hasFill = false;
    out.border = b;
    out.borderWidth = 2;
    out.hasBorder = true;
    out.label = b;
    break;
  case ButtonVariant::Soft:
    out.fill = soft(color);
    out.hasFill = true;
    out.label = b;
    out.hasBorder = false;
    break;
  case ButtonVariant::Ghost:
    out.hasFill = false;
    out.hasBorder = false;
    out.label = b;
    break;
  }
  return out;
}

} // namespace Theme
