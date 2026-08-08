#pragma once

#include "UINode.h"
#include "Style.h"
#include "Theme.h"
#include "UIEvent.h"

/**
 * Horizontal range: toggle-like brand capsule + thin right-side tail.
 * Left/Right (and Hold) nudge the value.
 *
 * Persist `value` in app state — UI nodes rebuild each frame.
 * Capsule/knob ease while visualAnimating (Page skips rebuild).
 */
class UIRange : public UINode {
public:
  using ChangeFn = void (*)(UIRange &self);

  UIRange();

  UIRange &style(const Style &s);
  UIRange &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UIRange &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }
  UIRange &onChange(ChangeFn fn) {
    onChange_ = fn;
    return *this;
  }

  UIRange &color(ButtonColor c);
  ButtonColor color() const { return color_; }

  UIRange &min(int16_t v);
  UIRange &max(int16_t v);
  UIRange &step(int16_t v);
  int16_t min() const { return min_; }
  int16_t max() const { return max_; }
  int16_t step() const { return step_; }

  /** Snap visual to `v` (use when rebuilding from app state). */
  UIRange &value(int16_t v);
  int16_t value() const { return value_; }

  /** Nudge by ±step (same as Left/Right). */
  bool nudge(int8_t dir);

  bool visualAnimating() const override;

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
  void tickSelf(float dt) override;
  bool handleEvent(UIEvent &e) override;

private:
  ButtonColor color_ = ButtonColor::Primary;
  int16_t min_ = 0;
  int16_t max_ = 100;
  int16_t step_ = 5;
  int16_t value_ = 0;
  /** Eased display value for fill/knob. */
  float animV_ = 0.f;
  ChangeFn onChange_ = nullptr;

  void applyFocusChrome();
  int16_t clampValue(int16_t v) const;
  float tNorm(float v) const;
};
