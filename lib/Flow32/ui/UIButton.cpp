#include "UIButton.h"
#include "UIText.h"

UIButton::UIButton() {
  highlightable_ = true;
  style_.padding = Edges(12, 14);
  style_.radius = 12;
  style_.width = Length::Pct(100);
  style_.outlineWidth = 2;
  style_.outlineOutside = true;
  applyChrome();
}

UIButton &UIButton::style(const Style &s) {
  const uint8_t keepRadius = style_.radius;
  UIDiv::style(s);
  if (style_.radius == 0 && keepRadius != 0) {
    style_.radius = keepRadius;
  }
  applyChrome();
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
  syncLabelColors();
  return *this;
}

UIButton &UIButton::variant(ButtonVariant v) {
  variant_ = v;
  applyChrome();
  syncLabelColors();
  return *this;
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

bool UIButton::handleEvent(UIEvent &e) {
  if (e.key == UIKey::Select && e.phase == UIKeyPhase::Down) {
    if (onPress_) {
      onPress_(*this);
      return true;
    }
    return true; // consume Select even without a handler
  }
  return false;
}

void UIButton::paintSelf(Canvas & /*canvas*/) {}
