#include "UIRange.h"
#include "../Canvas.h"

#include <math.h>

namespace {
constexpr float kRangeEase = 16.f;
/** Match UIToggle height — active side is a stretchable toggle capsule. */
constexpr int16_t kDefaultH = 30;
constexpr int16_t kKnobInset = 3;
constexpr int16_t kTailH = 8;
} // namespace

UIRange::UIRange() {
  highlightable_ = true;
  style_.width = Length::Pct(100);
  style_.height = Length::Px(kDefaultH);
  style_.hasBackground = false;
  style_.hasBorder = false;
  style_.outlineWidth = 2;
  style_.outlineOutside = true;
  style_.radius = static_cast<uint8_t>(kDefaultH / 2);
  applyFocusChrome();
}

void UIRange::applyFocusChrome() {
  style_.outlineColor = Theme::active().focusRing;
}

UIRange &UIRange::style(const Style &s) {
  UINode::style(s);
  style_.hasBackground = false;
  style_.hasBorder = false;
  if (style_.outlineWidth == 0) style_.outlineWidth = 2;
  applyFocusChrome();
  return *this;
}

UIRange &UIRange::color(ButtonColor c) {
  color_ = c;
  return *this;
}

int16_t UIRange::clampValue(int16_t v) const {
  if (max_ < min_) return min_;
  if (v < min_) return min_;
  if (v > max_) return max_;
  return v;
}

UIRange &UIRange::min(int16_t v) {
  min_ = v;
  value_ = clampValue(value_);
  return *this;
}

UIRange &UIRange::max(int16_t v) {
  max_ = v;
  value_ = clampValue(value_);
  return *this;
}

UIRange &UIRange::step(int16_t v) {
  step_ = v < 1 ? 1 : v;
  return *this;
}

UIRange &UIRange::value(int16_t v) {
  value_ = clampValue(v);
  animV_ = static_cast<float>(value_);
  return *this;
}

float UIRange::tNorm(float v) const {
  const float span = static_cast<float>(max_ - min_);
  if (span <= 0.f) return 0.f;
  float t = (v - static_cast<float>(min_)) / span;
  if (t < 0.f) t = 0.f;
  if (t > 1.f) t = 1.f;
  return t;
}

bool UIRange::nudge(int8_t dir) {
  if (dir == 0) return false;
  const int16_t next =
      clampValue(static_cast<int16_t>(value_ + dir * step_));
  if (next == value_) return true; // consume key even at end
  value_ = next;
  // Leave animV_ — tick eases toward value_
  if (onChange_) onChange_(*this);
  return true;
}

bool UIRange::visualAnimating() const {
  const float d = animV_ - static_cast<float>(value_);
  return d > 0.15f || d < -0.15f;
}

void UIRange::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  int16_t h = host ? host->resolveLen(style_.height, 0) : style_.height.resolve(0, s);
  if (style_.width.unit == Unit::Auto || w <= 0) w = availW;
  if (style_.height.unit == Unit::Auto || h <= 0) {
    h = host ? host->sx(kDefaultH) : scalePx(kDefaultH, s);
  }
  style_.radius = static_cast<uint8_t>(h > 1 ? h / 2 : 1);
  borderBox_ = Rect(x, y, w, h);
}

void UIRange::paintSelf(Canvas &canvas) {
  const Theme::ThemeTokens &th = Theme::active();
  const float t = tNorm(animV_);
  const uint16_t brand = Theme::brand(color_);
  const uint16_t tailCol = th.base300;
  const uint16_t knob = th.base100;

  const Rect &bb = borderBox_;
  const int16_t radius = bb.h > 1 ? static_cast<int16_t>(bb.h / 2) : 1;

  // Thin inactive tail across the full width (shows on the right of the capsule).
  const int16_t tailH = canvas.sx(kTailH);
  int16_t tailY = static_cast<int16_t>(bb.y + (bb.h - tailH) / 2);
  if (tailY < bb.y) tailY = bb.y;
  canvas.fillRoundRect(Rect(bb.x, tailY, bb.w, tailH),
                       static_cast<int16_t>(tailH / 2), tailCol);

  // Brand capsule grows from min (toggle-sized pill) → full width.
  const int16_t minCapsuleW = bb.h;
  int16_t capsuleW = static_cast<int16_t>(
      static_cast<float>(minCapsuleW) +
      static_cast<float>(bb.w - minCapsuleW) * t + 0.5f);
  if (capsuleW < minCapsuleW) capsuleW = minCapsuleW;
  if (capsuleW > bb.w) capsuleW = bb.w;
  canvas.fillRoundRect(Rect(bb.x, bb.y, capsuleW, bb.h), radius, brand);

  // White knob inset at the right end of the capsule (same as UIToggle).
  const int16_t inset = canvas.sx(kKnobInset);
  int16_t knobD = static_cast<int16_t>(bb.h - 2 * inset);
  if (knobD < 1) knobD = 1;
  const int16_t kx =
      static_cast<int16_t>(bb.x + capsuleW - inset - knobD);
  const int16_t ky = static_cast<int16_t>(bb.y + inset);
  canvas.fillRoundRect(Rect(kx, ky, knobD, knobD),
                       static_cast<int16_t>(knobD / 2), knob);
}

void UIRange::tickSelf(float dt) {
  const float target = static_cast<float>(value_);
  if (animV_ == target) return;
  const float u = 1.f - expf(-kRangeEase * dt);
  animV_ += (target - animV_) * u;
  if (animV_ > target - 0.2f && animV_ < target + 0.2f) animV_ = target;
}

bool UIRange::handleEvent(UIEvent &e) {
  if (e.phase != UIKeyPhase::Down && e.phase != UIKeyPhase::Hold) {
    return false;
  }
  if (e.key == UIKey::Left) return nudge(-1);
  if (e.key == UIKey::Right) return nudge(+1);
  return false;
}
