#pragma once

#include "UINode.h"

class UIImage : public UINode {
public:
  UIImage(const uint16_t *pixels, int16_t srcW, int16_t srcH)
      : pixels_(pixels), srcW_(srcW), srcH_(srcH) {}

  UIImage &style(const Style &s) {
    UINode::style(s);
    return *this;
  }
  UIImage &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;

private:
  const uint16_t *pixels_ = nullptr;
  int16_t srcW_ = 0;
  int16_t srcH_ = 0;
};
