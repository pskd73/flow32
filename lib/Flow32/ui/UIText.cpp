#include "UIText.h"
#include "../Canvas.h"

void UIText::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  const int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  const int16_t innerW =
      static_cast<int16_t>(w - pad.left - pad.right);

  TextStyle ts;
  ts.font = style_.font;
  ts.color = style_.color;
  ts.align = style_.align;
  ts.lineHeight = style_.lineHeight;
  ts.lineGap = style_.lineGap;
  ts.emojiSize = style_.emojiSize;
  ts.iconSize = style_.iconSize;
  ts.paragraphGap = 0;

  int16_t textH = 0;
  if (host && text_ && innerW > 0) {
    textH = host->measureTextHeight(text_, innerW, ts);
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(textH + pad.top + pad.bottom);
  } else {
    h = host ? host->resolveLen(style_.height, 0)
             : style_.height.resolve(0, s);
  }

  borderBox_ = Rect(x, y, w, h);
}

void UIText::paintSelf(Canvas &canvas) {
  if (!text_) return;
  TextStyle ts;
  ts.font = style_.font;
  ts.color = style_.color;
  ts.align = style_.align;
  ts.lineHeight = style_.lineHeight;
  ts.lineGap = style_.lineGap;
  ts.emojiSize = style_.emojiSize;
  ts.iconSize = style_.iconSize;
  ts.paragraphGap = 0;

  const Rect box = canvas.contentBox(borderBox_, style_.padding);
  canvas.drawText(box, text_, ts, false);
}
