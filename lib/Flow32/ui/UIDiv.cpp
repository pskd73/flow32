#include "UIDiv.h"
#include "../Canvas.h"

namespace {

int16_t axisAlign(int16_t space, int16_t size, Align a) {
  if (size >= space || space <= 0) return 0;
  switch (a) {
  case Align::Center:
    return static_cast<int16_t>((space - size) / 2);
  case Align::End:
    return static_cast<int16_t>(space - size);
  case Align::Start:
  default:
    return 0;
  }
}

uint8_t clampColumns(uint8_t c) {
  if (c < 1) return 1;
  if (c > 3) return 3;
  return c;
}

} // namespace

void UIDiv::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  const int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  const int16_t gap = host ? host->sx(style_.gap) : scalePx(style_.gap, s);
  const int16_t innerW = static_cast<int16_t>(w - pad.left - pad.right);
  const int16_t contentLeft = static_cast<int16_t>(x + pad.left);
  const int16_t contentTop = static_cast<int16_t>(y + pad.top);
  const uint8_t cols = clampColumns(style_.columns);

  if (cols == 1) {
    // --- Column stack ---
    int16_t cy = contentTop;
    for (uint8_t i = 0; i < childCount_; i++) {
      children_[i]->layout(contentLeft, cy, innerW > 0 ? innerW : 0);
      const Rect &cb = children_[i]->borderBox();
      const int16_t ox = axisAlign(innerW, cb.w, style_.alignH);
      if (ox != 0) {
        children_[i]->layout(static_cast<int16_t>(contentLeft + ox), cy,
                             innerW > 0 ? innerW : 0);
      }
      cy = static_cast<int16_t>(children_[i]->borderBox().y +
                               children_[i]->borderBox().h);
      if (i + 1 < childCount_) cy = static_cast<int16_t>(cy + gap);
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
      const int16_t innerH =
          static_cast<int16_t>(h - pad.top - pad.bottom);
      const int16_t contentH = static_cast<int16_t>(cy - contentTop);
      const int16_t oy = axisAlign(innerH, contentH, style_.alignV);
      if (oy != 0 && childCount_ > 0) {
        for (uint8_t i = 0; i < childCount_; i++) {
          const Rect &cb = children_[i]->borderBox();
          children_[i]->layout(cb.x, static_cast<int16_t>(cb.y + oy),
                               innerW > 0 ? innerW : 0);
        }
      }
    }

    borderBox_ = Rect(x, y, w, h);
    return;
  }

  // --- Grid: 2 or 3 equal columns, wrap ---
  const int16_t gapsW = static_cast<int16_t>(gap * (cols - 1));
  int16_t cellW = innerW > gapsW
                      ? static_cast<int16_t>((innerW - gapsW) / cols)
                      : 0;
  if (cellW < 0) cellW = 0;

  int16_t rowY = contentTop;
  int16_t maxBottom = contentTop;
  uint8_t i = 0;
  while (i < childCount_) {
    const uint8_t rowStart = i;
    uint8_t rowCount = 0;
    int16_t rowH = 0;

    // Place up to `cols` children for this row (first pass: top-left of cell).
    for (uint8_t c = 0; c < cols && i < childCount_; c++, i++) {
      const int16_t cellX =
          static_cast<int16_t>(contentLeft + c * (cellW + gap));
      children_[i]->layout(cellX, rowY, cellW);
      const int16_t ch = children_[i]->borderBox().h;
      if (ch > rowH) rowH = ch;
      rowCount++;
    }

    // Second pass: align each child inside its cell.
    for (uint8_t c = 0; c < rowCount; c++) {
      UINode *child = children_[rowStart + c];
      const int16_t cellX =
          static_cast<int16_t>(contentLeft + c * (cellW + gap));
      const Rect &cb = child->borderBox();
      const int16_t ox = axisAlign(cellW, cb.w, style_.alignH);
      const int16_t oy = axisAlign(rowH, cb.h, style_.alignV);
      if (ox != 0 || oy != 0 || cb.x != cellX || cb.y != rowY) {
        child->layout(static_cast<int16_t>(cellX + ox),
                      static_cast<int16_t>(rowY + oy), cellW);
      }
    }

    maxBottom = static_cast<int16_t>(rowY + rowH);
    rowY = static_cast<int16_t>(maxBottom + gap);
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(maxBottom - y + pad.bottom);
    if (childCount_ == 0) {
      h = static_cast<int16_t>(pad.top + pad.bottom);
    }
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
