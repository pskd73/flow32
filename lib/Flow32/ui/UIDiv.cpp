#include "UIDiv.h"
#include "../Canvas.h"

void UIDiv::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  const int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  const int16_t gap = host ? host->sx(style_.gap) : scalePx(style_.gap, s);
  const int16_t innerW = static_cast<int16_t>(w - pad.left - pad.right);

  int16_t cy = static_cast<int16_t>(y + pad.top);
  const int16_t cx = static_cast<int16_t>(x + pad.left);

  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->layout(cx, cy, innerW > 0 ? innerW : 0);
    cy = static_cast<int16_t>(cy + children_[i]->borderBox().h);
    if (i + 1 < childCount_) {
      cy = static_cast<int16_t>(cy + gap);
    }
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(cy - y + pad.bottom);
    if (h < pad.top + pad.bottom) {
      h = static_cast<int16_t>(pad.top + pad.bottom);
    }
  } else {
    h = host ? host->resolveLen(style_.height, 0)
             : style_.height.resolve(0, s);
  }

  borderBox_ = Rect(x, y, w, h);
}

void UIDiv::paintSelf(Canvas & /*canvas*/) {
  // Background handled in UINode::draw; div is a layout container.
}
