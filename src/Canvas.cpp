#include "Canvas.h"
#include "GoogleSans16aa.h"
#include "GoogleSans22aa.h"
#include "GoogleSans34aa.h"
#include "GoogleSansBold22aa.h"
#include "ui/UIButton.h"
#include "ui/UIDiv.h"
#include "ui/UIImage.h"
#include "ui/UIText.h"

#include <string.h>

Canvas::Canvas(Display &display) : display_(display) {
  bounds_ = Rect::fromSize(display.panel().width, display.panel().height);
}

void Canvas::clear(uint16_t color) { display_.clear(color); }

void Canvas::present() { display_.present(); }

void Canvas::present(const Rect &r) { display_.present(r.x, r.y, r.w, r.h); }

void Canvas::setOrigin(int16_t x, int16_t y) {
  originX_ = x;
  originY_ = y;
}

void Canvas::setClip(const Rect &r) { display_.setClip(r.x, r.y, r.w, r.h); }

void Canvas::clearClip() { display_.clearClip(); }

void Canvas::setBounds(const Rect &r) { bounds_ = r; }

void Canvas::moveTo(int16_t x, int16_t y) {
  cx_ = x;
  cy_ = y;
}

void Canvas::moveBy(int16_t dx, int16_t dy) {
  cx_ = static_cast<int16_t>(cx_ + dx);
  cy_ = static_cast<int16_t>(cy_ + dy);
}

void Canvas::resetFlow() {
  cx_ = bounds_.x;
  cy_ = bounds_.y;
  lastLineH_ = 0;
}

void Canvas::newLine(int16_t extra) {
  cx_ = bounds_.x;
  cy_ = static_cast<int16_t>(cy_ + lastLineH_ + extra);
}

void Canvas::gap(int16_t dy) { cy_ = static_cast<int16_t>(cy_ + dy); }

void Canvas::applyFont(FontRole role) {
  aaFont_ = nullptr;
  switch (role) {
  case FontRole::Small:
    aaFont_ = &GoogleSans16aa;
    display_.useFontSmall();
    break;
  case FontRole::Body:
    aaFont_ = &GoogleSans22aa;
    display_.useFontBody();
    break;
  case FontRole::BodyBold:
    aaFont_ = &GoogleSansBold22aa;
    display_.useFontBodyBold();
    break;
  case FontRole::BodyLarge:
    aaFont_ = &GoogleSans34aa;
    display_.useFontBodyLarge();
    break;
  case FontRole::Playful:
    display_.useFontPlayful();
    break;
  case FontRole::PlayfulLarge:
    display_.useFontPlayfulLarge();
    break;
  case FontRole::Childlike:
    display_.useFontChildlike();
    break;
  case FontRole::ChildlikeLarge:
    display_.useFontChildlikeLarge();
    break;
  case FontRole::Default:
    display_.useFontDefault();
    break;
  }
}

int16_t Canvas::fontLineHeight() const {
  if (aaFont_) return aaFont_->yAdvance;
  const GFXfont *font = display_.font();
  if (!font) {
    return static_cast<int16_t>(8 * display_.textSize());
  }
  return font->yAdvance;
}

int16_t Canvas::fontBaseline() const {
  if (aaFont_) return aaFont_->baseline;
  const GFXfont *font = display_.font();
  if (!font) return static_cast<int16_t>(7);
  return static_cast<int16_t>(font->yAdvance - 2);
}

int16_t Canvas::measureCharWidth(char c) const {
  if (aaFont_) return AAFontDraw::charWidth(*aaFont_, c);
  const GFXfont *font = display_.font();
  if (!font) {
    return static_cast<int16_t>(6 * display_.textSize());
  }
  if (c < font->first || c > font->last) {
    c = '?';
    if (c < font->first || c > font->last) return 0;
  }
  const GFXglyph *g = &font->glyph[c - font->first];
  return g->xAdvance;
}

int16_t Canvas::measureTextWidth(const char *text, size_t len) const {
  int16_t w = 0;
  for (size_t i = 0; i < len; i++) {
    w = static_cast<int16_t>(w + measureCharWidth(text[i]));
  }
  return w;
}

void Canvas::contentToScreen(int16_t cx, int16_t cy, int16_t &sx,
                             int16_t &sy) const {
  sx = static_cast<int16_t>(cx + originX_);
  sy = static_cast<int16_t>(cy + originY_);
}

void Canvas::fillRect(const Rect &box, uint16_t color) {
  int16_t sx, sy;
  contentToScreen(box.x, box.y, sx, sy);
  display_.fillRect(sx, sy, box.w, box.h, color);
}

void Canvas::fillRoundRect(const Rect &box, int16_t radius, uint16_t color) {
  int16_t sx, sy;
  contentToScreen(box.x, box.y, sx, sy);
  if (radius <= 0) {
    display_.fillRect(sx, sy, box.w, box.h, color);
    return;
  }
  int16_t r = radius;
  if (r * 2 > box.w) r = box.w / 2;
  if (r * 2 > box.h) r = box.h / 2;
  display_.fillRoundRect(sx, sy, box.w, box.h, r, color);
}

void Canvas::drawOutline(const Rect &box, uint8_t width, uint16_t color,
                         bool outside, int16_t radius) {
  if (width == 0 || box.w <= 0 || box.h <= 0) return;
  int16_t sx, sy;
  contentToScreen(box.x, box.y, sx, sy);
  for (uint8_t i = 0; i < width; i++) {
    int16_t x, y, w, h;
    if (outside) {
      x = static_cast<int16_t>(sx - (i + 1));
      y = static_cast<int16_t>(sy - (i + 1));
      w = static_cast<int16_t>(box.w + 2 * (i + 1));
      h = static_cast<int16_t>(box.h + 2 * (i + 1));
    } else {
      x = static_cast<int16_t>(sx + i);
      y = static_cast<int16_t>(sy + i);
      w = static_cast<int16_t>(box.w - 2 * i);
      h = static_cast<int16_t>(box.h - 2 * i);
      if (w <= 0 || h <= 0) break;
    }
    if (radius > 0) {
      int16_t rr = static_cast<int16_t>(radius + (outside ? (i + 1) : -static_cast<int16_t>(i)));
      if (rr < 0) rr = 0;
      if (rr * 2 > w) rr = w / 2;
      if (rr * 2 > h) rr = h / 2;
      display_.drawRoundRect(x, y, w, h, rr, color);
    } else {
      display_.drawRect(x, y, w, h, color);
    }
  }
}

DrawResult Canvas::drawText(const char *text, const TextStyle &style,
                            bool advance) {
  const int16_t boxW =
      static_cast<int16_t>(bounds_.x + bounds_.w - cx_);
  const DrawResult r =
      drawTextInBox(cx_, cy_, boxW, 10000, text, style, true);
  if (advance) {
    cx_ = bounds_.x;
    cy_ = static_cast<int16_t>(cy_ + r.h + style.paragraphGap);
    lastLineH_ = fontLineHeight();
  }
  return r;
}

DrawResult Canvas::drawText(const Rect &box, const char *text,
                            const TextStyle &style, bool advance) {
  const DrawResult r =
      drawTextInBox(box.x, box.y, box.w, box.h, text, style, true);
  if (advance) {
    cx_ = bounds_.x;
    cy_ = static_cast<int16_t>(box.y + r.h + style.paragraphGap);
    lastLineH_ = fontLineHeight();
  }
  return r;
}

int16_t Canvas::measureTextHeight(const char *text, int16_t maxW,
                                  const TextStyle &style) {
  return drawTextInBox(0, 0, maxW, 10000, text, style, false).h;
}

DrawResult Canvas::drawTextInBox(int16_t boxX, int16_t boxY, int16_t boxW,
                                 int16_t boxH, const char *text,
                                 const TextStyle &style, bool paint) {
  DrawResult result{};
  if (!text || boxW <= 0 || boxH <= 0) return result;

  applyFont(style.font);
  display_.setTextColor(style.color);
  display_.setTextWrap(false);

  const int16_t lineH =
      static_cast<int16_t>(fontLineHeight() + style.lineGap);
  const int16_t baselineOffset = fontBaseline();

  int16_t penY = boxY;
  const char *p = text;
  int16_t maxLineW = 0;
  int16_t lines = 0;

  while (*p) {
    if ((penY - boxY) + fontLineHeight() > boxH) break;

    while (*p == ' ') p++;
    if (*p == '\n') {
      p++;
      penY = static_cast<int16_t>(penY + lineH);
      lines++;
      continue;
    }
    if (!*p) break;

    const char *lineStart = p;
    const char *breakAt = nullptr;
    int16_t width = 0;
    int16_t widthBeforeBreak = 0;

    while (*p && *p != '\n') {
      const int16_t cw = measureCharWidth(*p);
      if (width + cw > boxW && p > lineStart) break;
      width = static_cast<int16_t>(width + cw);
      if (*p == ' ') {
        breakAt = p;
        widthBeforeBreak = static_cast<int16_t>(width - cw);
      }
      p++;
    }

    const char *lineEnd = p;
    int16_t lineW = width;
    if (breakAt && *p && *p != '\n') {
      lineEnd = breakAt;
      lineW = widthBeforeBreak;
      p = breakAt + 1;
    }

    if (lineEnd == lineStart) {
      if (!*p) break;
      if (p == lineStart) {
        p++;
        lineW = measureCharWidth(*lineStart);
      }
      lineEnd = p;
      lineW = measureTextWidth(lineStart, static_cast<size_t>(lineEnd - lineStart));
    }

    int16_t drawX = boxX;
    if (style.align == Align::Center) {
      drawX = static_cast<int16_t>(boxX + (boxW - lineW) / 2);
    } else if (style.align == Align::End) {
      drawX = static_cast<int16_t>(boxX + boxW - lineW);
    }

    if (paint) {
      int16_t sx, sy;
      contentToScreen(drawX, static_cast<int16_t>(penY + baselineOffset), sx, sy);

      if (aaFont_) {
        int16_t penX = sx;
        for (const char *c = lineStart; c < lineEnd; c++) {
          AAFontDraw::drawChar(display_, *aaFont_, penX, sy, *c, style.color);
          penX = static_cast<int16_t>(penX + AAFontDraw::charWidth(*aaFont_, *c));
        }
      } else {
        display_.setCursor(sx, sy);
        for (const char *c = lineStart; c < lineEnd; c++) {
          display_.write(static_cast<uint8_t>(*c));
        }
      }
    }

    if (lineW > maxLineW) maxLineW = lineW;
    penY = static_cast<int16_t>(penY + lineH);
    lines++;

    if (*p == '\n') p++;
  }

  lastLineH_ = lineH;
  result.w = maxLineW;
  result.h =
      static_cast<int16_t>(lines > 0 ? (lines * lineH - style.lineGap) : 0);
  result.endX = static_cast<int16_t>(boxX + maxLineW);
  result.endY = static_cast<int16_t>(boxY + result.h);
  return result;
}

static void computeFit(int16_t boxW, int16_t boxH, int16_t srcW, int16_t srcH,
                       ImageFit fit, int16_t &dstX, int16_t &dstY, int16_t &dstW,
                       int16_t &dstH, int32_t &srcX0, int32_t &srcY0,
                       int32_t &srcX1, int32_t &srcY1) {
  dstX = 0;
  dstY = 0;
  srcX0 = 0;
  srcY0 = 0;
  srcX1 = srcW;
  srcY1 = srcH;

  if (srcW <= 0 || srcH <= 0 || boxW <= 0 || boxH <= 0) {
    dstW = 0;
    dstH = 0;
    return;
  }

  switch (fit) {
  case ImageFit::Fill:
    dstW = boxW;
    dstH = boxH;
    break;
  case ImageFit::Center:
    dstW = min(boxW, srcW);
    dstH = min(boxH, srcH);
    dstX = static_cast<int16_t>((boxW - dstW) / 2);
    dstY = static_cast<int16_t>((boxH - dstH) / 2);
    srcX0 = (srcW - dstW) / 2;
    srcY0 = (srcH - dstH) / 2;
    srcX1 = srcX0 + dstW;
    srcY1 = srcY0 + dstH;
    break;
  case ImageFit::Contain: {
    const float sx = (float)boxW / (float)srcW;
    const float sy = (float)boxH / (float)srcH;
    const float s = sx < sy ? sx : sy;
    dstW = static_cast<int16_t>(srcW * s + 0.5f);
    dstH = static_cast<int16_t>(srcH * s + 0.5f);
    if (dstW < 1) dstW = 1;
    if (dstH < 1) dstH = 1;
    dstX = static_cast<int16_t>((boxW - dstW) / 2);
    dstY = static_cast<int16_t>((boxH - dstH) / 2);
    break;
  }
  case ImageFit::Cover: {
    const float sx = (float)boxW / (float)srcW;
    const float sy = (float)boxH / (float)srcH;
    const float s = sx > sy ? sx : sy;
    const int32_t fullW = static_cast<int32_t>(srcW * s + 0.5f);
    const int32_t fullH = static_cast<int32_t>(srcH * s + 0.5f);
    dstW = boxW;
    dstH = boxH;
    // Visible window in source space
    const float inv = 1.0f / s;
    srcX0 = static_cast<int32_t>(((fullW - boxW) / 2) * inv);
    srcY0 = static_cast<int32_t>(((fullH - boxH) / 2) * inv);
    srcX1 = srcX0 + static_cast<int32_t>(boxW * inv);
    srcY1 = srcY0 + static_cast<int32_t>(boxH * inv);
    if (srcX0 < 0) srcX0 = 0;
    if (srcY0 < 0) srcY0 = 0;
    if (srcX1 > srcW) srcX1 = srcW;
    if (srcY1 > srcH) srcY1 = srcH;
    break;
  }
  }
}

void Canvas::blitScaled(const Rect &dstScreen, const uint16_t *pixels,
                        int16_t srcW, int16_t srcH, int32_t srcX0,
                        int32_t srcY0, int32_t srcX1, int32_t srcY1) {
  if (!pixels || dstScreen.empty() || srcX1 <= srcX0 || srcY1 <= srcY0) return;

  const int32_t srcSpanW = srcX1 - srcX0;
  const int32_t srcSpanH = srcY1 - srcY0;

  for (int16_t row = 0; row < dstScreen.h; row++) {
    const int32_t sy =
        srcY0 + (int32_t)row * srcSpanH / dstScreen.h;
    if (sy < 0 || sy >= srcH) continue;
    const int16_t dy = static_cast<int16_t>(dstScreen.y + row);
    for (int16_t col = 0; col < dstScreen.w; col++) {
      const int32_t sx =
          srcX0 + (int32_t)col * srcSpanW / dstScreen.w;
      if (sx < 0 || sx >= srcW) continue;
      const int16_t dx = static_cast<int16_t>(dstScreen.x + col);
      display_.drawPixel(dx, dy, pixels[(int32_t)sy * srcW + sx]);
    }
  }
}

DrawResult Canvas::drawImage(const uint16_t *pixels, int16_t srcW, int16_t srcH,
                             int16_t boxW, int16_t boxH, ImageFit fit,
                             bool advance) {
  const Rect box(cx_, cy_, boxW, boxH);
  const DrawResult r = drawImage(box, pixels, srcW, srcH, fit, false);
  if (advance) {
    cx_ = bounds_.x;
    cy_ = static_cast<int16_t>(cy_ + boxH);
    lastLineH_ = boxH;
  }
  return r;
}

DrawResult Canvas::drawImage(const Rect &box, const uint16_t *pixels,
                             int16_t srcW, int16_t srcH, ImageFit fit,
                             bool advance) {
  DrawResult result{};
  if (!pixels || box.empty()) return result;

  int16_t odx, ody, odw, odh;
  int32_t sx0, sy0, sx1, sy1;
  computeFit(box.w, box.h, srcW, srcH, fit, odx, ody, odw, odh, sx0, sy0, sx1,
             sy1);

  int16_t screenX, screenY;
  contentToScreen(static_cast<int16_t>(box.x + odx),
                  static_cast<int16_t>(box.y + ody), screenX, screenY);
  const Rect dst(screenX, screenY, odw, odh);
  blitScaled(dst, pixels, srcW, srcH, sx0, sy0, sx1, sy1);

  result.w = box.w;
  result.h = box.h;
  result.endX = static_cast<int16_t>(box.x + box.w);
  result.endY = static_cast<int16_t>(box.y + box.h);

  if (advance) {
    cx_ = bounds_.x;
    cy_ = static_cast<int16_t>(box.y + box.h);
    lastLineH_ = box.h;
  }
  return result;
}

void Canvas::beginUI() {
  arena_.reset();
  rootCount_ = 0;
}

UIDiv &Canvas::div() { return arena_.create<UIDiv>(); }

UIButton &Canvas::button() { return arena_.create<UIButton>(); }

UIText &Canvas::text(const char *s) { return arena_.create<UIText>(s); }

UIImage &Canvas::image(const uint16_t *pixels, int16_t srcW, int16_t srcH) {
  return arena_.create<UIImage>(pixels, srcW, srcH);
}

void Canvas::add(UINode &node) {
  if (rootCount_ >= kMaxRoots) return;
  roots_[rootCount_++] = &node;
}

void Canvas::tick(float dt) {
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->tick(dt);
  }
}

void Canvas::drawUI() {
  drawUI(display_.panel().width);
}

void Canvas::drawUI(int16_t availW) {
  UIText::setLayoutCanvas(this);
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->layout(0, 0, availW);
  }
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->draw(*this);
  }
  UIText::setLayoutCanvas(nullptr);
}
