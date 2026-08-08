#pragma once

#include "UIDiv.h"
#include "Style.h"
#include "Theme.h"
#include "UIEvent.h"

class UIText;

/** Highlightable button with color + variant chrome. */
class UIButton : public UIDiv {
public:
  using PressFn = void (*)(UIButton &self);

  UIButton();

  UIButton &style(const Style &s);
  UIButton &onTick(TickFn fn) {
    UIDiv::onTick(fn);
    return *this;
  }
  UIButton &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }
  UIButton &onPress(PressFn fn) {
    onPress_ = fn;
    return *this;
  }
  UIButton &add(UINode &child);

  UIButton &color(ButtonColor c);
  UIButton &variant(ButtonVariant v);
  ButtonColor color() const { return color_; }
  ButtonVariant variant() const { return variant_; }

  /**
   * Disabled: not focusable, ignores press, chrome at ~50% opacity over
   * `disabledBackdrop` (defaults to Theme::active().base100).
   */
  UIButton &disabled(bool v);
  bool disabled() const { return disabled_; }
  UIButton &disabledBackdrop(uint16_t color);

  /**
   * Lucide icon beside the label (via Canvas::iconSd).
   * `side`: "left" (default) or "right".
   * Size from style().iconSize, else 16 design px.
   */
  UIButton &icon(const char *lucideName, const char *side = "left");
  const char *iconName() const { return iconName_; }
  bool iconRight() const { return iconRight_; }

  /** Label color resolved from color/variant (for child UIText). */
  uint16_t labelColor() const { return labelColor_; }

  bool visualAnimating() const override;

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
  void tickSelf(float dt) override;
  bool handleEvent(UIEvent &e) override;

private:
  ButtonColor color_ = ButtonColor::Primary;
  ButtonVariant variant_ = ButtonVariant::Solid;
  uint16_t labelColor_ = 0xFFFF;
  PressFn onPress_ = nullptr;
  bool disabled_ = false;
  uint16_t disabledBackdrop_ = 0;
  const char *iconName_ = nullptr;
  bool iconRight_ = false;

  /** 1 → just tapped; eases to 0. Drives dip + dim. */
  float press_ = 0;

  void applyChrome();
  void syncLabelColors();
  void syncPressVisual();
  void triggerPressAnim();
  int16_t iconSlotPx() const;
};
