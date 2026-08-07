#pragma once

#include "UINode.h"

class UIDiv : public UINode {
public:
  UIDiv() { canHaveChildren_ = true; }

  UIDiv &style(const Style &s) {
    UINode::style(s);
    return *this;
  }
  UIDiv &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UIDiv &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }
  UIDiv &add(UINode &child) {
    UINode::add(child);
    return *this;
  }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
};
