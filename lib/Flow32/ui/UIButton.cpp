#include "UIButton.h"
#include "UIText.h"
#include "../Canvas.h"
#include "../IconSd.h"

#include <math.h>

namespace {
constexpr float kPressDipPx = 3.f;
constexpr float kPressDim = 0.28f;
constexpr float kPressEase = 22.f;
constexpr float kDisabledOpacity = 0.5f; // mix toward backdrop
constexpr int16_t kIconDesign = 16;
constexpr int16_t kIconGap = 6;
} // namespace

UIButton::UIButton() {
  highlightable_ = true;
  style_.padding = Edges(12, 18);
  style_.radius = Theme::active().radiusField;
  style_.width = Length::Pct(100);
  style_.outlineWidth = 2;
  style_.outlineOutside = true;
  disabledBackdrop_ = Theme::active().base100;
  applyChrome();
}

UIButton &UIButton::style(const Style &s) {
  const uint8_t keepRadius = style_.radius;
  UIDiv::style(s);
  if (style_.radius == 0 && keepRadius != 0) {
    style_.radius = keepRadius;
  }
  applyChrome();
  syncPressVisual();
  syncLabelColors();
  return *this;
}

UIButton &UIButton::add(UINode &child) {
  UIDiv::add(child);
  syncLabelColors();
  return *this;
}

UIButton &UIButton::color(ButtonColor c) {
  color_ = c;
  applyChrome();
  syncPressVisual();
  syncLabelColors();
  return *this;
}

UIButton &UIButton::variant(ButtonVariant v) {
  variant_ = v;
  applyChrome();
  syncPressVisual();
  syncLabelColors();
  return *this;
}

UIButton &UIButton::disabled(bool v) {
  disabled_ = v;
  highlightable_ = !v;
  if (v) {
    press_ = 0.f;
    setHighlighted(false);
  }
  applyChrome();
  syncPressVisual();
  syncLabelColors();
  return *this;
}

UIButton &UIButton::disabledBackdrop(uint16_t color) {
  disabledBackdrop_ = color;
  if (disabled_) {
    applyChrome();
    syncPressVisual();
    syncLabelColors();
  }
  return *this;
}

UIButton &UIButton::icon(const char *lucideName, const char *side) {
  iconName_ = lucideName;
  iconRight_ = false;
  if (side && side[0]) {
    if (side[0] == 'r' || side[0] == 'R') iconRight_ = true;
  }
  return *this;
}

int16_t UIButton::iconSlotPx() const {
  if (!iconName_ || !iconName_[0]) return 0;
  Canvas *host = layoutHost();
  const float s = layoutScale();
  const int16_t design =
      style_.iconSize > 0 ? static_cast<int16_t>(style_.iconSize) : kIconDesign;
  const int16_t slot = static_cast<int16_t>(design + kIconGap);
  return host ? host->sx(slot) : scalePx(slot, s);
}

void UIButton::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  const int16_t slot = iconSlotPx();
  const Edges saved = style_.padding;
  if (slot > 0) {
    if (iconRight_) {
      style_.padding.right = static_cast<int16_t>(saved.right + slot);
    } else {
      style_.padding.left = static_cast<int16_t>(saved.left + slot);
    }
  }
  UIDiv::layoutSelf(x, y, availW);
  style_.padding = saved;
}

void UIButton::applyChrome() {
  const Theme::ButtonChrome chrome = Theme::buttonChrome(color_, variant_);
  labelColor_ = chrome.label;
  style_.outlineColor = chrome.focus;

  if (chrome.hasFill) {
    style_.background = chrome.fill;
    style_.hasBackground = true;
  } else {
    style_.hasBackground = false;
  }

  if (chrome.hasBorder) {
    style_.borderColor = chrome.border;
    style_.borderWidth = chrome.borderWidth;
    style_.hasBorder = true;
  } else {
    style_.hasBorder = false;
    style_.borderWidth = 0;
  }
}

void UIButton::syncLabelColors() {
  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->style().setColor(labelColor_);
  }
}

void UIButton::syncPressVisual() {
  applyChrome();
  anim_.y = (!disabled_ && press_ > 0.f) ? press_ * kPressDipPx : 0.f;

  if (!disabled_ && press_ > 0.f) {
    if (style_.hasBackground) {
      style_.background = Theme::dim(style_.background, press_ * kPressDim);
    }
    if (style_.hasBorder) {
      style_.borderColor = Theme::dim(style_.borderColor, press_ * kPressDim);
    }
  }

  if (disabled_) {
    const float t = 1.f - kDisabledOpacity; // 0.5 → halfway to backdrop
    if (style_.hasBackground) {
      style_.background =
          Theme::lerp(style_.background, disabledBackdrop_, t);
    }
    if (style_.hasBorder) {
      style_.borderColor =
          Theme::lerp(style_.borderColor, disabledBackdrop_, t);
    }
    labelColor_ = Theme::lerp(labelColor_, disabledBackdrop_, t);
    style_.outlineColor =
        Theme::lerp(style_.outlineColor, disabledBackdrop_, t);
  }
}

void UIButton::triggerPressAnim() {
  if (disabled_) return;
  press_ = 1.f;
  syncPressVisual();
}

bool UIButton::visualAnimating() const {
  return !disabled_ && press_ > 0.001f;
}

void UIButton::tickSelf(float dt) {
  if (disabled_ || press_ <= 0.f) return;
  const float t = 1.f - expf(-kPressEase * dt);
  press_ += (0.f - press_) * t;
  if (press_ < 0.02f) press_ = 0.f;
  syncPressVisual();
}

bool UIButton::handleEvent(UIEvent &e) {
  if (e.key == UIKey::Select && e.phase == UIKeyPhase::Down) {
    if (disabled_) return true; // consume, do nothing
    triggerPressAnim();
    if (onPress_) {
      onPress_(*this);
    }
    return true;
  }
  return false;
}

void UIButton::paintSelf(Canvas &canvas) {
  if (!iconName_ || !iconName_[0]) return;
  IconSd *icons = canvas.iconSd();
  if (!icons || !icons->ready()) return;
  const uint32_t cp = icons->codepoint(iconName_);
  if (!cp) return;

  const int16_t design =
      style_.iconSize > 0 ? static_cast<int16_t>(style_.iconSize) : kIconDesign;
  const int16_t iconPx = canvas.sx(design);
  const Rect content = canvas.contentBox(borderBox_, style_.padding);
  const Point origin = canvas.origin();

  int16_t screenX;
  if (iconRight_) {
    screenX = static_cast<int16_t>(content.x + content.w - iconPx + origin.x);
  } else {
    screenX = static_cast<int16_t>(content.x + origin.x);
  }
  const int16_t top =
      static_cast<int16_t>(content.y + (content.h - iconPx) / 2);
  const int16_t baselineY = static_cast<int16_t>(top + iconPx + origin.y);
  icons->draw(canvas.display(), cp, screenX, baselineY, iconPx, labelColor_);
}
