#include "UIText.h"
#include "../Canvas.h"

Canvas *UIText::layoutCanvas_ = nullptr;

void UIText::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  const int16_t w = style_.width.resolve(availW);
  const int16_t innerW =
      static_cast<int16_t>(w - style_.padding.left - style_.padding.right);

  TextStyle ts;
  ts.font = style_.font;
  ts.color = style_.color;
  ts.align = style_.align;
  ts.lineGap = style_.lineGap;
  ts.paragraphGap = 0;

  int16_t textH = 0;
  if (layoutCanvas_ && text_ && innerW > 0) {
    textH = layoutCanvas_->measureTextHeight(text_, innerW, ts);
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(textH + style_.padding.top + style_.padding.bottom);
  } else {
    h = style_.height.resolve(0);
  }

  borderBox_ = Rect(x, y, w, h);
}

void UIText::paintSelf(Canvas &canvas) {
  if (!text_) return;
  TextStyle ts;
  ts.font = style_.font;
  ts.color = style_.color;
  ts.align = style_.align;
  ts.lineGap = style_.lineGap;
  ts.paragraphGap = 0;

  const Rect box = contentRect(borderBox_, style_.padding);
  canvas.drawText(box, text_, ts, false);
}
