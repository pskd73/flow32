#pragma once

#include "UINode.h"
#include "Style.h"
#include "Theme.h"
#include "UIEvent.h"

/**
 * Animated on/off switch. Theme brand colors; knob slides when toggled.
 *
 * Persist checked state in app variables — UI nodes are rebuilt each frame.
 * While the knob animates, Page skips rebuild (visualAnimating).
 */
class UIToggle : public UINode {
public:
  using ChangeFn = void (*)(UIToggle &self);

  UIToggle();

  UIToggle &style(const Style &s);
  UIToggle &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UIToggle &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }
  UIToggle &onChange(ChangeFn fn) {
    onChange_ = fn;
    return *this;
  }

  UIToggle &color(ButtonColor c);
  ButtonColor color() const { return color_; }

  /** Snap visual to `v` (use when rebuilding from app state). */
  UIToggle &checked(bool v);
  bool checked() const { return checked_; }

  /** Flip and start animation (same as Select). */
  void toggle();

  bool visualAnimating() const override;

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
  void tickSelf(float dt) override;
  bool handleEvent(UIEvent &e) override;

private:
  ButtonColor color_ = ButtonColor::Primary;
  bool checked_ = false;
  /** 0 = off, 1 = on — eases toward checked_. */
  float animT_ = 0.f;
  ChangeFn onChange_ = nullptr;

  void applyFocusChrome();
};
