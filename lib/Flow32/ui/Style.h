#pragma once

#include <Arduino.h>
#include "Rect.h"

enum class FontRole : uint8_t {
  Small,
  Body,
  BodyBold,
  BodyLarge,
  Playful,
  PlayfulLarge,
  Childlike,
  ChildlikeLarge,
  Default,
};

enum class ImageFit : uint8_t { Fill, Contain, Cover, Center };

enum class Align : uint8_t { Start, Center, End };

enum class Unit : uint8_t { Auto, Px, Percent };

enum class ButtonColor : uint8_t { Primary, Secondary, Accent };

enum class ButtonVariant : uint8_t { Solid, Outline, Soft, Ghost };

struct Length {
  Unit unit = Unit::Auto;
  int16_t value = 0;

  static Length Auto() { return Length{}; }
  static Length Px(int16_t v) {
    Length l;
    l.unit = Unit::Px;
    l.value = v;
    return l;
  }
  static Length Pct(int16_t v) {
    Length l;
    l.unit = Unit::Percent;
    l.value = v;
    return l;
  }

  int16_t resolve(int16_t parent) const {
    switch (unit) {
    case Unit::Px:
      return value;
    case Unit::Percent:
      return static_cast<int16_t>((static_cast<int32_t>(parent) * value) / 100);
    case Unit::Auto:
    default:
      return parent;
    }
  }
};

struct Edges {
  int16_t top = 0;
  int16_t right = 0;
  int16_t bottom = 0;
  int16_t left = 0;

  Edges() = default;
  Edges(int16_t all) : top(all), right(all), bottom(all), left(all) {}
  Edges(int16_t v, int16_t h) : top(v), right(h), bottom(v), left(h) {}
  Edges(int16_t t, int16_t r, int16_t b, int16_t l)
      : top(t), right(r), bottom(b), left(l) {}
};

/** CSS-inspired style for UI nodes. */
class Style {
public:
  Length width = Length::Auto();
  Length height = Length::Auto();
  Edges padding;
  int16_t gap = 0;
  uint16_t color = 0xFFFF;
  uint16_t background = 0;
  bool hasBackground = false;
  uint8_t radius = 0; // border-radius px
  uint16_t borderColor = 0;
  uint8_t borderWidth = 0;
  bool hasBorder = false;
  FontRole font = FontRole::Body;
  Align align = Align::Start;
  ImageFit objectFit = ImageFit::Cover;
  uint8_t lineGap = 4;
  uint16_t outlineColor = 0xFFFF; // focus ring
  uint8_t outlineWidth = 2;
  bool outlineOutside = true;

  Style &setWidth(Length v) {
    width = v;
    return *this;
  }
  Style &setHeight(Length v) {
    height = v;
    return *this;
  }
  Style &setPadding(Edges e) {
    padding = e;
    return *this;
  }
  Style &setPadding(int16_t all) {
    padding = Edges(all);
    return *this;
  }
  Style &setPadding(int16_t v, int16_t h) {
    padding = Edges(v, h);
    return *this;
  }
  Style &setGap(int16_t v) {
    gap = v;
    return *this;
  }
  Style &setColor(uint16_t v) {
    color = v;
    return *this;
  }
  Style &setBackground(uint16_t v) {
    background = v;
    hasBackground = true;
    return *this;
  }
  Style &clearBackground() {
    hasBackground = false;
    return *this;
  }
  Style &setRadius(uint8_t v) {
    radius = v;
    return *this;
  }
  Style &setBorder(uint16_t color, uint8_t width = 1) {
    borderColor = color;
    borderWidth = width;
    hasBorder = width > 0;
    return *this;
  }
  Style &clearBorder() {
    hasBorder = false;
    borderWidth = 0;
    return *this;
  }
  Style &setFont(FontRole v) {
    font = v;
    return *this;
  }
  Style &setAlign(Align v) {
    align = v;
    return *this;
  }
  Style &setFit(ImageFit v) {
    objectFit = v;
    return *this;
  }
  Style &setLineGap(uint8_t v) {
    lineGap = v;
    return *this;
  }
  Style &setOutlineColor(uint16_t v) {
    outlineColor = v;
    return *this;
  }
  Style &setOutlineWidth(uint8_t v) {
    outlineWidth = v;
    return *this;
  }
  Style &setOutlineOutside(bool v) {
    outlineOutside = v;
    return *this;
  }
};

inline int16_t contentWidth(const Rect &border, const Edges &pad) {
  return static_cast<int16_t>(border.w - pad.left - pad.right);
}

inline Rect contentRect(const Rect &border, const Edges &pad) {
  return Rect(static_cast<int16_t>(border.x + pad.left),
              static_cast<int16_t>(border.y + pad.top),
              static_cast<int16_t>(border.w - pad.left - pad.right),
              static_cast<int16_t>(border.h - pad.top - pad.bottom));
}
