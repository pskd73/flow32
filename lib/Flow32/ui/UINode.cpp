#include "UINode.h"
#include "UIDebug.h"
#include "../Canvas.h"

Canvas *UINode::layoutHost_ = nullptr;
float UINode::layoutScale_ = 1.0f;

void UINode::setLayoutHost(Canvas *c) {
  layoutHost_ = c;
  layoutScale_ = c ? c->uiScale() : 1.0f;
}

UINode &UINode::add(UINode &child) {
  if (!canHaveChildren_) return *this;
  if (childCount_ >= kMaxChildren) return *this;
  child.parent_ = this;
  children_[childCount_++] = &child;
  return *this;
}

void UINode::collectHighlightable(UINode **out, uint8_t &count, uint8_t max) {
  if (highlightable_ && count < max) {
    out[count++] = this;
  }
  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->collectHighlightable(out, count, max);
  }
}

void UINode::tickSelf(float /*dt*/) {}

void UINode::tick(float dt) {
  tickSelf(dt);
  if (onTick_) onTick_(*this, dt);
  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->tick(dt);
  }
}

void UINode::layout(int16_t x, int16_t y, int16_t availW) {
  layoutSelf(x, y, availW);
}

Rect UINode::paintBox() const {
  return borderBox_;
}

bool UINode::handleEvent(UIEvent & /*e*/) { return false; }

bool UINode::dispatch(UIEvent &e) {
  if (onEvent_ && onEvent_(*this, e)) {
    e.handled = true;
    return true;
  }
  if (handleEvent(e)) {
    e.handled = true;
    return true;
  }
  if (parent_) return parent_->dispatch(e);
  return false;
}

void UINode::draw(Canvas &canvas) {
  const int16_t ox = canvas.origin().x;
  const int16_t oy = canvas.origin().y;
  canvas.setOrigin(static_cast<int16_t>(ox + (int16_t)anim_.x),
                   static_cast<int16_t>(oy + (int16_t)anim_.y));

  // Pass design-px radius/widths — Canvas scales them at draw time.
  if (style_.hasBackground) {
    if (style_.radius > 0) {
      canvas.fillRoundRect(borderBox_, style_.radius, style_.background);
    } else {
      canvas.fillRect(borderBox_, style_.background);
    }
  }

  if (style_.hasBorder && style_.borderWidth > 0) {
    canvas.drawOutline(borderBox_, style_.borderWidth, style_.borderColor,
                       /*outside=*/false, style_.radius);
  }

  paintSelf(canvas);

  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->draw(canvas);
  }

  if (highlightable_ && highlighted_ && style_.outlineWidth > 0) {
    canvas.drawOutline(borderBox_, style_.outlineWidth, style_.outlineColor,
                       style_.outlineOutside, style_.radius);
  }

  if (UIDebug::borders) {
    // Hairline in physical px: width 1 with scale still yields ≥1.
    canvas.drawOutline(borderBox_, 1, UIDebug::borderColor, /*outside=*/false,
                       /*radius=*/0);
  }

  canvas.setOrigin(ox, oy);
}
