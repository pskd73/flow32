#include "UIDiv.h"
#include "../Canvas.h"

void UIDiv::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  const int16_t w = style_.width.resolve(availW);
  const int16_t innerW =
      static_cast<int16_t>(w - style_.padding.left - style_.padding.right);

  int16_t cy =
      static_cast<int16_t>(y + style_.padding.top);
  const int16_t cx =
      static_cast<int16_t>(x + style_.padding.left);

  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->layout(cx, cy, innerW > 0 ? innerW : 0);
    cy = static_cast<int16_t>(cy + children_[i]->borderBox().h);
    if (i + 1 < childCount_) {
      cy = static_cast<int16_t>(cy + style_.gap);
    }
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(cy - y + style_.padding.bottom);
    if (h < style_.padding.top + style_.padding.bottom) {
      h = static_cast<int16_t>(style_.padding.top + style_.padding.bottom);
    }
  } else {
    h = style_.height.resolve(0);
  }

  borderBox_ = Rect(x, y, w, h);
}

void UIDiv::paintSelf(Canvas & /*canvas*/) {
  // Background handled in UINode::draw; div is a layout container.
}
