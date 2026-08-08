#pragma once

#include <Arduino.h>
#include "Style.h"

/**
 * DaisyUI-inspired theme tokens (RGB565).
 * Switch with Theme::setActive(Theme::WinterTheme()) etc.
 */
namespace Theme {

inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

inline uint16_t hex(uint32_t c) {
  return rgb(static_cast<uint8_t>((c >> 16) & 0xFF),
             static_cast<uint8_t>((c >> 8) & 0xFF),
             static_cast<uint8_t>(c & 0xFF));
}

struct ThemeTokens {
  const char *name;

  // Surfaces (daisy: base-100 / 200 / 300 / content)
  uint16_t base100;
  uint16_t base200;
  uint16_t base300;
  uint16_t baseContent;

  // Brand
  uint16_t primary;
  uint16_t primaryContent;
  uint16_t secondary;
  uint16_t secondaryContent;
  uint16_t accent;
  uint16_t accentContent;

  // Neutral + status
  uint16_t neutral;
  uint16_t neutralContent;
  uint16_t info;
  uint16_t infoContent;
  uint16_t success;
  uint16_t successContent;
  uint16_t warning;
  uint16_t warningContent;
  uint16_t error;
  uint16_t errorContent;

  // Focus ring (often near baseContent on light, cream on dark)
  uint16_t focusRing;

  // Shape — design px (daisy 1rem ≈ 16)
  uint8_t radiusSelector;
  uint8_t radiusField;
  uint8_t radiusBox;
};

const ThemeTokens &FlowTheme();
const ThemeTokens &WinterTheme();

const ThemeTokens &active();
void setActive(const ThemeTokens &t);

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

/** Linear mix: t=0 → a, t=1 → b. */
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
  const ThemeTokens &t = active();
  switch (c) {
  case ButtonColor::Secondary:
    return t.secondary;
  case ButtonColor::Accent:
    return t.accent;
  case ButtonColor::Primary:
  default:
    return t.primary;
  }
}

inline uint16_t brandContent(ButtonColor c) {
  const ThemeTokens &t = active();
  switch (c) {
  case ButtonColor::Secondary:
    return t.secondaryContent;
  case ButtonColor::Accent:
    return t.accentContent;
  case ButtonColor::Primary:
  default:
    return t.primaryContent;
  }
}

/** Soft fill: brand washed toward base-100. */
inline uint16_t soft(ButtonColor c) {
  return lerp(brand(c), active().base100, 0.75f);
}

inline ButtonChrome buttonChrome(ButtonColor color, ButtonVariant variant) {
  ButtonChrome out;
  const ThemeTokens &t = active();
  out.focus = t.focusRing;
  const uint16_t b = brand(color);
  switch (variant) {
  case ButtonVariant::Solid:
    out.fill = b;
    out.hasFill = true;
    out.label = brandContent(color);
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
