#include "UIIcon.h"
#include "../Canvas.h"
#include "../IconSd.h"

void UIIcon::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  const int16_t design =
      style_.iconSize > 0 ? static_cast<int16_t>(style_.iconSize)
                          : static_cast<int16_t>(16);

  int16_t w;
  if (style_.width.unit != Unit::Auto) {
    w = host ? host->resolveLen(style_.width, availW)
             : style_.width.resolve(availW, s);
  } else {
    w = host ? host->sx(design) : scalePx(design, s);
  }

  int16_t h;
  if (style_.height.unit != Unit::Auto) {
    h = host ? host->resolveLen(style_.height, 0)
             : style_.height.resolve(0, s);
  } else {
    h = host ? host->sx(design) : scalePx(design, s);
  }

  borderBox_ = Rect(x, y, w, h);
}

void UIIcon::paintSelf(Canvas &canvas) {
  if (!name_ || !name_[0]) return;
  IconSd *icons = canvas.iconSd();
  if (!icons || !icons->ready()) return;

  const int16_t design =
      style_.iconSize > 0 ? static_cast<int16_t>(style_.iconSize)
                          : (borderBox_.h > 0 ? borderBox_.h
                                              : static_cast<int16_t>(16));
  const int16_t iconPx = canvas.sx(design);
  const Rect content = canvas.contentBox(borderBox_, style_.padding);
  const Point origin = canvas.origin();
  const uint16_t color = style_.color;

  IconDraw::Align h = IconDraw::Align::Center;
  if (style_.alignH == Align::Start) h = IconDraw::Align::Start;
  else if (style_.alignH == Align::End) h = IconDraw::Align::End;

  IconDraw::Align v = IconDraw::Align::Center;
  if (style_.alignV == Align::Start) v = IconDraw::Align::Start;
  else if (style_.alignV == Align::End) v = IconDraw::Align::End;

  icons->drawInBox(canvas.display(), name_, content, origin.x, origin.y, iconPx,
                   color, h, v);
}
