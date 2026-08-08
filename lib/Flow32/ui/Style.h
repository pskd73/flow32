#pragma once

#include <Arduino.h>
#include <math.h>
#include "Rect.h"

/** Scale design px by uiScale. Positive values keep at least 1px when scaled. */
inline int16_t scalePx(int16_t v, float s) {
  if (v == 0 || s == 1.0f) return v;
  const int32_t out = (int32_t)lroundf((float)v * s);
  if (v > 0 && out < 1) return 1;
  if (v < 0 && out > -1) return -1;
  if (out > 32767) return 32767;
  if (out < -32768) return -32768;
  return static_cast<int16_t>(out);
}

inline uint8_t scaleU8(uint8_t v, float s) {
  if (v == 0 || s == 1.0f) return v;
  const int32_t out = (int32_t)lroundf((float)v * s);
  if (out < 1) return 1;
  if (out > 255) return 255;
  return static_cast<uint8_t>(out);
}

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

/** Div packing: 1 = column stack, 2–3 = wrapping grid. */
enum class Unit : uint8_t { Auto, Px, Percent };

/** CSS-like positioning. Absolute children are taken out of flow. */
enum class Position : uint8_t { Relative, Absolute };

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

  int16_t resolve(int16_t parent, float scale = 1.0f) const {
    switch (unit) {
    case Unit::Px:
      return scalePx(value, scale);
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

inline Edges scaleEdges(const Edges &e, float s) {
  return Edges(scalePx(e.top, s), scalePx(e.right, s), scalePx(e.bottom, s),
               scalePx(e.left, s));
}

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
  /** Text horizontal align (UIText). */
  Align align = Align::Start;
  /**
   * Div layout: 1 = vertical stack, 2–3 = equal-width wrapping columns.
   * Clamped to 1..3 at layout time.
   */
  uint8_t columns = 1;
  /** Div: horizontal place of children / cell content (Left/Center/Right). */
  Align alignH = Align::Start;
  /** Div: vertical place of stack / cell content (Top/Center/Bottom). */
  Align alignV = Align::Start;
  ImageFit objectFit = ImageFit::Cover;
  /**
   * Absolute → out of flow; placed via left/top/right/bottom relative to the
   * parent's content box. Unset insets use Unit::Auto.
   */
  Position position = Position::Relative;
  Length left = Length::Auto();
  Length top = Length::Auto();
  Length right = Length::Auto();
  Length bottom = Length::Auto();
  /**
   * Absolute line box height in design px (0 = normal).
   * When 0, stride is font metrics + lineGap. When set, stride is lineHeight
   * (lineGap ignored) — controls space between wrapped lines.
   */
  uint8_t lineHeight = 0;
  /** Extra leading when lineHeight is 0 (design px). */
  uint8_t lineGap = 0;
  /**
   * Emoji / Lucide icon raster size in design px (0 = derive from font).
   * Prefer ≤ atlas bakedSize (icons default bake 96) to avoid soft upscales.
   */
  uint8_t emojiSize = 0;
  /** When set, overrides emojiSize for icon sizing (0 = use emojiSize). */
  uint8_t iconSize = 0;
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
  Style &setColumns(uint8_t v) {
    columns = v;
    return *this;
  }
  Style &setAlignH(Align v) {
    alignH = v;
    return *this;
  }
  Style &setAlignV(Align v) {
    alignV = v;
    return *this;
  }
  Style &setFit(ImageFit v) {
    objectFit = v;
    return *this;
  }
  Style &setPosition(Position v) {
    position = v;
    return *this;
  }
  Style &setLeft(Length v) {
    left = v;
    return *this;
  }
  Style &setTop(Length v) {
    top = v;
    return *this;
  }
  Style &setRight(Length v) {
    right = v;
    return *this;
  }
  Style &setBottom(Length v) {
    bottom = v;
    return *this;
  }
  Style &setLineHeight(uint8_t v) {
    lineHeight = v;
    return *this;
  }
  Style &setLineGap(uint8_t v) {
    lineGap = v;
    return *this;
  }
  Style &setEmojiSize(uint8_t v) {
    emojiSize = v;
    return *this;
  }
  Style &setIconSize(uint8_t v) {
    iconSize = v;
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
