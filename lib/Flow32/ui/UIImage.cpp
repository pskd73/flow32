#include "UIImage.h"
#include "../Canvas.h"

void UIImage::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  const int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    int16_t contentH = 0;
    const int16_t innerW =
        static_cast<int16_t>(w - pad.left - pad.right);
    if (srcW_ > 0 && innerW > 0) {
      contentH = static_cast<int16_t>((int32_t)innerW * srcH_ / srcW_);
    }
    h = static_cast<int16_t>(contentH + pad.top + pad.bottom);
  } else {
    h = host ? host->resolveLen(style_.height, 0)
             : style_.height.resolve(0, s);
  }
  borderBox_ = Rect(x, y, w, h);
}

void UIImage::paintSelf(Canvas &canvas) {
  if (!pixels_) return;
  const Rect box = canvas.contentBox(borderBox_, style_.padding);
  canvas.drawImage(box, pixels_, srcW_, srcH_, style_.objectFit, false);
}
