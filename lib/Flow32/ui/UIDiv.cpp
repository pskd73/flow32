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

bool hasInset(const Length &l) { return l.unit != Unit::Auto; }

bool isAbsolute(const UINode &n) {
  return n.style().position == Position::Absolute;
}

int16_t resolveInset(const Length &l, int16_t parent, Canvas *host, float s) {
  return host ? host->resolveLen(l, parent) : l.resolve(parent, s);
}

/**
 * Place an absolute child in the parent's content box.
 * left/right/top/bottom use Unit::Auto as "unset".
 * If top & bottom unset → vertically centered; if left & right unset → start.
 */
void layoutAbsoluteChild(UINode *child, int16_t contentLeft, int16_t contentTop,
                         int16_t innerW, int16_t innerH, Canvas *host,
                         float s) {
  const Style &st = child->style();
  const bool hasL = hasInset(st.left);
  const bool hasR = hasInset(st.right);
  const bool hasT = hasInset(st.top);
  const bool hasB = hasInset(st.bottom);

  const int16_t leftPx =
      hasL ? resolveInset(st.left, innerW, host, s) : 0;
  const int16_t rightPx =
      hasR ? resolveInset(st.right, innerW, host, s) : 0;
  const int16_t topPx =
      hasT ? resolveInset(st.top, innerH, host, s) : 0;
  const int16_t bottomPx =
      hasB ? resolveInset(st.bottom, innerH, host, s) : 0;

  int16_t availW = innerW;
  if (st.width.unit != Unit::Auto) {
    availW =
        host ? host->resolveLen(st.width, innerW) : st.width.resolve(innerW, s);
  } else if (hasL && hasR) {
    availW = static_cast<int16_t>(innerW - leftPx - rightPx);
    if (availW < 0) availW = 0;
  }

  int16_t x = static_cast<int16_t>(contentLeft + leftPx);
  int16_t y = static_cast<int16_t>(contentTop + topPx);

  // Measure at a tentative origin (needed when right/bottom/center apply).
  child->layout(x, y, availW);
  const int16_t cw = child->borderBox().w;
  const int16_t ch = child->borderBox().h;

  if (!hasL && hasR) {
    x = static_cast<int16_t>(contentLeft + innerW - rightPx - cw);
  }

  if (!hasT && hasB) {
    y = static_cast<int16_t>(contentTop + innerH - bottomPx - ch);
  } else if (!hasT && !hasB) {
    y = static_cast<int16_t>(contentTop + axisAlign(innerH, ch, Align::Center));
  }

  if (x != child->borderBox().x || y != child->borderBox().y) {
    child->layout(x, y, availW);
  }
}

void layoutAbsoluteChildren(UINode **children, uint8_t count, int16_t contentLeft,
                            int16_t contentTop, int16_t innerW, int16_t innerH,
                            Canvas *host, float s) {
  for (uint8_t i = 0; i < count; i++) {
    if (!isAbsolute(*children[i])) continue;
    layoutAbsoluteChild(children[i], contentLeft, contentTop, innerW, innerH,
                        host, s);
  }
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
    // --- Column stack (in-flow children only) ---
    int16_t cy = contentTop;
    uint8_t flowCount = 0;
    for (uint8_t i = 0; i < childCount_; i++) {
      if (isAbsolute(*children_[i])) continue;
      if (flowCount > 0) cy = static_cast<int16_t>(cy + gap);
      children_[i]->layout(contentLeft, cy, innerW > 0 ? innerW : 0);
      const Rect &cb = children_[i]->borderBox();
      const int16_t ox = axisAlign(innerW, cb.w, style_.alignH);
      if (ox != 0) {
        children_[i]->layout(static_cast<int16_t>(contentLeft + ox), cy,
                             innerW > 0 ? innerW : 0);
      }
      cy = static_cast<int16_t>(children_[i]->borderBox().y +
                               children_[i]->borderBox().h);
      flowCount++;
    }

    int16_t h;
    if (style_.height.unit == Unit::Auto) {
      h = static_cast<int16_t>(cy - y + pad.bottom);
      if (flowCount == 0) {
        h = static_cast<int16_t>(pad.top + pad.bottom);
      }
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
      if (oy != 0 && flowCount > 0) {
        for (uint8_t i = 0; i < childCount_; i++) {
          if (isAbsolute(*children_[i])) continue;
          const Rect &cb = children_[i]->borderBox();
          children_[i]->layout(cb.x, static_cast<int16_t>(cb.y + oy),
                               innerW > 0 ? innerW : 0);
        }
      }
    }

    borderBox_ = Rect(x, y, w, h);
    const int16_t innerH =
        static_cast<int16_t>(h - pad.top - pad.bottom);
    layoutAbsoluteChildren(children_, childCount_, contentLeft, contentTop,
                           innerW > 0 ? innerW : 0, innerH > 0 ? innerH : 0,
                           host, s);
    return;
  }

  // --- Grid: 2 or 3 equal columns, wrap (in-flow only) ---
  const int16_t gapsW = static_cast<int16_t>(gap * (cols - 1));
  int16_t cellW = innerW > gapsW
                      ? static_cast<int16_t>((innerW - gapsW) / cols)
                      : 0;
  if (cellW < 0) cellW = 0;

  // Compact in-flow children into a temp list for row packing.
  UINode *flow[UINode::kMaxChildren];
  uint8_t flowCount = 0;
  for (uint8_t i = 0; i < childCount_; i++) {
    if (isAbsolute(*children_[i])) continue;
    if (flowCount < UINode::kMaxChildren) flow[flowCount++] = children_[i];
  }

  int16_t rowY = contentTop;
  int16_t maxBottom = contentTop;
  uint8_t i = 0;
  while (i < flowCount) {
    const uint8_t rowStart = i;
    uint8_t rowCount = 0;
    int16_t rowH = 0;

    for (uint8_t c = 0; c < cols && i < flowCount; c++, i++) {
      const int16_t cellX =
          static_cast<int16_t>(contentLeft + c * (cellW + gap));
      flow[i]->layout(cellX, rowY, cellW);
      const int16_t ch = flow[i]->borderBox().h;
      if (ch > rowH) rowH = ch;
      rowCount++;
    }

    for (uint8_t c = 0; c < rowCount; c++) {
      UINode *child = flow[rowStart + c];
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
    if (flowCount == 0) {
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
  const int16_t innerH = static_cast<int16_t>(h - pad.top - pad.bottom);
  layoutAbsoluteChildren(children_, childCount_, contentLeft, contentTop,
                         innerW > 0 ? innerW : 0, innerH > 0 ? innerH : 0, host,
                         s);
}

void UIDiv::paintSelf(Canvas & /*canvas*/) {
  // Background handled in UINode::draw; div is a layout container.
}
