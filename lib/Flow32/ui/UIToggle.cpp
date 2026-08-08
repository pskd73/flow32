#include "UIToggle.h"
#include "../Canvas.h"

#include <math.h>

namespace {
constexpr float kToggleEase = 14.f;
constexpr int16_t kDefaultW = 52;
constexpr int16_t kDefaultH = 30;
constexpr int16_t kKnobInset = 3;
} // namespace

UIToggle::UIToggle() {
  highlightable_ = true;
  style_.width = Length::Px(kDefaultW);
  style_.height = Length::Px(kDefaultH);
  style_.hasBackground = false;
  style_.hasBorder = false;
  style_.outlineWidth = 2;
  style_.outlineOutside = true;
  style_.radius = static_cast<uint8_t>(kDefaultH / 2);
  applyFocusChrome();
}

void UIToggle::applyFocusChrome() {
  style_.outlineColor = Theme::active().focusRing;
}

UIToggle &UIToggle::style(const Style &s) {
  UINode::style(s);
  // Toggle paints its own chrome; keep focus ring settings.
  style_.hasBackground = false;
  style_.hasBorder = false;
  if (style_.outlineWidth == 0) style_.outlineWidth = 2;
  applyFocusChrome();
  return *this;
}

UIToggle &UIToggle::color(ButtonColor c) {
  color_ = c;
  return *this;
}

UIToggle &UIToggle::checked(bool v) {
  checked_ = v;
  animT_ = v ? 1.f : 0.f;
  return *this;
}

void UIToggle::toggle() {
  checked_ = !checked_;
  // Leave animT_ where it is — tickSelf eases toward the new target.
  if (onChange_) onChange_(*this);
}

bool UIToggle::visualAnimating() const {
  const float target = checked_ ? 1.f : 0.f;
  const float d = animT_ - target;
  return d > 0.01f || d < -0.01f;
}

void UIToggle::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  int16_t h = host ? host->resolveLen(style_.height, 0) : style_.height.resolve(0, s);
  if (style_.width.unit == Unit::Auto || w <= 0) {
    w = host ? host->sx(kDefaultW) : scalePx(kDefaultW, s);
  }
  if (style_.height.unit == Unit::Auto || h <= 0) {
    h = host ? host->sx(kDefaultH) : scalePx(kDefaultH, s);
  }
  style_.radius = static_cast<uint8_t>(h > 1 ? h / 2 : 1);
  borderBox_ = Rect(x, y, w, h);
}

void UIToggle::paintSelf(Canvas &canvas) {
  const Theme::ThemeTokens &th = Theme::active();
  const float t = animT_ < 0.f ? 0.f : (animT_ > 1.f ? 1.f : animT_);

  const uint16_t trackOff = th.base300;
  const uint16_t trackOn = Theme::brand(color_);
  const uint16_t track = Theme::lerp(trackOff, trackOn, t);

  const uint16_t knobOff = th.base100;
  const uint16_t knobOn = Theme::brandContent(color_);
  const uint16_t knob = Theme::lerp(knobOff, knobOn, t);

  const Rect &bb = borderBox_;
  const int16_t radius = bb.h > 1 ? static_cast<int16_t>(bb.h / 2) : 1;
  canvas.fillRoundRect(bb, radius, track);

  const int16_t inset = canvas.sx(kKnobInset);
  int16_t knobD = static_cast<int16_t>(bb.h - 2 * inset);
  if (knobD < 1) knobD = 1;
  const int16_t travel = static_cast<int16_t>(bb.w - 2 * inset - knobD);
  const int16_t kx =
      static_cast<int16_t>(bb.x + inset + static_cast<float>(travel) * t + 0.5f);
  const int16_t ky = static_cast<int16_t>(bb.y + inset);
  const Rect knobBox(kx, ky, knobD, knobD);
  canvas.fillRoundRect(knobBox, static_cast<int16_t>(knobD / 2), knob);
}

void UIToggle::tickSelf(float dt) {
  const float target = checked_ ? 1.f : 0.f;
  if (animT_ == target) return;
  const float u = 1.f - expf(-kToggleEase * dt);
  animT_ += (target - animT_) * u;
  if (animT_ > target - 0.01f && animT_ < target + 0.01f) animT_ = target;
}

bool UIToggle::handleEvent(UIEvent &e) {
  if (e.key == UIKey::Select && e.phase == UIKeyPhase::Down) {
    toggle();
    return true;
  }
  return false;
}
