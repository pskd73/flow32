#include "UIImage.h"
#include "../Canvas.h"

void UIImage::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  const int16_t w = style_.width.resolve(availW);
  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    int16_t contentH = 0;
    const int16_t innerW =
        static_cast<int16_t>(w - style_.padding.left - style_.padding.right);
    if (srcW_ > 0 && innerW > 0) {
      contentH = static_cast<int16_t>((int32_t)innerW * srcH_ / srcW_);
    }
    h = static_cast<int16_t>(contentH + style_.padding.top + style_.padding.bottom);
  } else {
    h = style_.height.resolve(0);
  }
  borderBox_ = Rect(x, y, w, h);
}

void UIImage::paintSelf(Canvas &canvas) {
  if (!pixels_) return;
  const Rect box = contentRect(borderBox_, style_.padding);
  canvas.drawImage(box, pixels_, srcW_, srcH_, style_.objectFit, false);
}
