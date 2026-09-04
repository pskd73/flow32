#pragma once

#include "UINode.h"

/**
 * Chrome Lucide icon — sized square, glyph centered via IconSd::drawInBox.
 * Prefer this over stuffing icon UTF-8 into UIText for nav/status/controls.
 */
class UIIcon : public UINode {
public:
  explicit UIIcon(const char *lucideName) : name_(lucideName) {
    highlightable_ = false;
    style_.width = Length::Px(16);
    style_.height = Length::Px(16);
    style_.iconSize = 16;
  }

  UIIcon &style(const Style &s) {
    UINode::style(s);
    return *this;
  }
  UIIcon &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }

  const char *name() const { return name_; }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;

private:
  const char *name_ = nullptr;
};
