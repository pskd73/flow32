#pragma once

#include "UINode.h"

#include <string.h>

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
    if (text_ == text) return *this;
    if (text_ && text && strcmp(text_, text) == 0) {
      text_ = text;
      return *this;
    }
    text_ = text;
    offset_ = 0;
    pause_ = kPauseSec;
    overflowing_ = false;
    return *this;
  }
  const char *text() const { return text_; }

  /**
   * Single-line scroll when the string is wider than the box. No-ops when it
   * fits. The page must redraw while this is moving — visualAnimating() is
   * true only during overflow.
   */
  UIText &marquee(bool v = true) {
    marquee_ = v;
    return *this;
  }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
  void tickSelf(float dt) override;
  bool visualAnimating() const override { return marquee_ && overflowing_; }

private:
  static constexpr float kPxPerSec = 32.f;
  static constexpr float kPauseSec = 1.0f;
  static constexpr int16_t kGapPx = 32;

  const char *text_ = "";
  bool marquee_ = false;
  bool overflowing_ = false;
  float offset_ = 0;
  float pause_ = kPauseSec;
  int16_t textW_ = 0;
  int16_t boxW_ = 0;
};
