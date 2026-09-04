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
  if (!marquee_ || box.w <= 0 || box.h <= 0) {
    overflowing_ = false;
    canvas.drawText(box, text_, ts, false);
    return;
  }

  textW_ = canvas.measureTextWidth(text_, ts);
  overflowing_ = textW_ > box.w;
  if (!overflowing_) {
    offset_ = 0;
    canvas.drawText(box, text_, ts, false);
    return;
  }

  ts.align = Align::Start;
  const int16_t ox = canvas.origin().x;
  const int16_t oy = canvas.origin().y;
  Display &disp = canvas.display();
  const bool hadClip = disp.clipEnabled();
  const Rect prevClip = disp.clipRect();
  canvas.setClip(
      Rect(static_cast<int16_t>(box.x + ox), static_cast<int16_t>(box.y + oy),
           box.w, box.h));
  // Wide enough that drawText will not wrap; clip does the cropping.
  const Rect line(static_cast<int16_t>(box.x - (int16_t)offset_), box.y, 10000,
                  box.h);
  canvas.drawText(line, text_, ts, false);
  if (hadClip) {
    canvas.setClip(prevClip);
  } else {
    canvas.clearClip();
  }
  boxW_ = box.w;
}

void UIText::tickSelf(float dt) {
  if (!marquee_ || !overflowing_ || textW_ <= 0 || boxW_ <= 0) return;
  if (pause_ > 0.f) {
    pause_ -= dt;
    return;
  }
  offset_ += kPxPerSec * dt;
  // Last character is on screen once offset reaches textW - boxW. Keep going
  // a bit so the ending isn't clipped at the right edge when we loop.
  const float shown = static_cast<float>(textW_ - boxW_);
  const float loop = (shown > 0.f ? shown : 0.f) + static_cast<float>(kGapPx);
  if (offset_ >= loop) {
    offset_ = 0;
    pause_ = kPauseSec;
  }
}
