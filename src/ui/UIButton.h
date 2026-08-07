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

  /** Label color resolved from color/variant (for child UIText). */
  uint16_t labelColor() const { return labelColor_; }

protected:
  void paintSelf(Canvas &canvas) override;
  bool handleEvent(UIEvent &e) override;

private:
  ButtonColor color_ = ButtonColor::Primary;
  ButtonVariant variant_ = ButtonVariant::Solid;
  uint16_t labelColor_ = 0xFFFF;
  PressFn onPress_ = nullptr;

  void applyChrome();
  void syncLabelColors();
};
