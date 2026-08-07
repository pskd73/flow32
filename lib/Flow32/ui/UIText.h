#pragma once

#include "UINode.h"

class UIText : public UINode {
public:
  explicit UIText(const char *text = "") : text_(text) {}

  UIText &style(const Style &s) {
    UINode::style(s);
    return *this;
  }
  UIText &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UIText &setText(const char *text) {
    text_ = text;
    return *this;
  }
  const char *text() const { return text_; }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;

private:
  const char *text_ = "";
  // Canvas pointer set during layout/paint via thread-local-ish: we need Canvas
  // for measure. Pass through static during layout tree from host.
public:
  static void setLayoutCanvas(Canvas *c) { layoutCanvas_ = c; }

private:
  static Canvas *layoutCanvas_;
};
