#include "Canvas.h"
#include "GoogleSans16aa.h"
#include "GoogleSans22aa.h"
#include "GoogleSans34aa.h"
#include "GoogleSansBold22aa.h"
#include "ui/UIButton.h"
#include "ui/UIDiv.h"
#include "ui/UIImage.h"
#include "ui/UIText.h"
#include "ui/UIToggle.h"
#include "ui/UIRange.h"
#include "ui/UISelect.h"

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
  cy_ = static_cast<int16_t>(cy_ + lastLineH_ + sx(extra));
}

void Canvas::gap(int16_t dy) { cy_ = static_cast<int16_t>(cy_ + sx(dy)); }

void Canvas::applyFont(FontRole role) {
  aaFont_ = nullptr;
  const float s = uiScale();
  if (s < 0.05f) {
    display_.useFontDefault();
    return;
  }

  // AA GoogleSans roles: pick nearest baked size — never stretch bitmaps.
  // Below the smallest AA face (~16), fall back to GFX 9pt / default so
  // uiScale < ~0.5 still shrinks type (spacing already scales continuously).
  const bool aaRole = role == FontRole::Small || role == FontRole::Body ||
                      role == FontRole::BodyBold || role == FontRole::BodyLarge;
  if (aaRole) {
    int16_t design = 22;
    bool wantBold = false;
    switch (role) {
    case FontRole::Small:
      design = 16;
      break;
    case FontRole::Body:
      design = 22;
      break;
    case FontRole::BodyBold:
      design = 22;
      wantBold = true;
      break;
    case FontRole::BodyLarge:
      design = 34;
      break;
    default:
      break;
    }
    const int16_t target = scalePx(design, s);

    if (target < 10) {
      display_.useFontDefault();
      return;
    }
    if (target < 14) {
      display_.useFontSmall(); // GFX 9pt, no AA
      return;
    }

    auto dist = [](int16_t a, int16_t b) -> int16_t {
      return a > b ? static_cast<int16_t>(a - b) : static_cast<int16_t>(b - a);
    };
    const int16_t d16 = dist(target, 16);
    const int16_t d22 = dist(target, 22);
    const int16_t d34 = dist(target, 34);

    if (d16 <= d22 && d16 <= d34) {
      aaFont_ = &GoogleSans16aa;
      display_.useFontSmall();
    } else if (d34 < d22) {
      aaFont_ = &GoogleSans34aa;
      display_.useFontBodyLarge();
    } else if (wantBold) {
      aaFont_ = &GoogleSansBold22aa;
      display_.useFontBodyBold();
    } else {
      aaFont_ = &GoogleSans22aa;
      display_.useFontBody();
    }
    return;
  }

  // GFX decorative fonts: step down a size tier when scale is small.
  FontRole effective = role;
  if (s < 0.85f) {
    if (role == FontRole::PlayfulLarge) effective = FontRole::Playful;
    else if (role == FontRole::ChildlikeLarge) effective = FontRole::Childlike;
  }
  if (s < 0.45f) {
    // No smaller decorative faces — use compact GFX body.
    display_.useFontSmall();
    return;
  }

  switch (effective) {
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
  default:
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

void Canvas::syncEmojiDrawSize(uint8_t overridePx) {
  emojiDrawPx_ = 0;
  const bool haveEmoji =
      (emojiSd_ && emojiSd_->ready()) ||
      (emojiAtlas_ && emojiAtlas_->bakedSize > 0);
  const bool haveIcons = iconSd_ && iconSd_->ready();
  if (!haveEmoji && !haveIcons) return;
  int16_t px;
  if (overridePx > 0) {
    px = sx(static_cast<int16_t>(overridePx));
  } else {
    px = static_cast<int16_t>(fontLineHeight() - 2);
  }
  if (px < 1) px = 1;
  emojiDrawPx_ = px;
}

int16_t Canvas::measureCodeWidth(uint32_t cp) const {
  if (emojiDrawPx_ > 0 && IconDraw::isIconCp(cp) && iconSd_ &&
      iconSd_->ready()) {
    const int16_t adv = iconSd_->advance(cp, emojiDrawPx_);
    if (adv > 0) return adv;
  }
  if (emojiDrawPx_ > 0) {
    if (emojiSd_ && emojiSd_->ready()) {
      const int16_t adv = emojiSd_->advance(cp, emojiDrawPx_);
      if (adv > 0) return adv;
    } else if (emojiAtlas_) {
      const ColorEmojiGlyph *eg = ColorEmojiDraw::find(*emojiAtlas_, cp);
      if (eg) {
        return ColorEmojiDraw::advance(*eg, emojiAtlas_->bakedSize, emojiDrawPx_);
      }
    }
  }
  if (aaFont_) {
    const uint32_t folded = AAFontDraw::foldCodepoint(cp);
    if (folded <= 0xFF) {
      const int16_t w = AAFontDraw::charWidth(*aaFont_, folded);
      // Contiguous Latin-1 holes (C1) have zero advance — treat as missing.
      if (w > 0 || folded == static_cast<uint32_t>(' ')) return w;
    }
    return AAFontDraw::charWidth(*aaFont_, static_cast<uint32_t>('?'));
  }
  if (cp < 0x80) {
    return measureCharWidth(static_cast<char>(cp));
  }
  return measureCharWidth('?');
}

int16_t Canvas::measureUtf8Width(const char *start, const char *end) const {
  int16_t w = 0;
  const char *p = start;
  while (p < end) {
    uint32_t cp = 0;
    const char *before = p;
    if (!ColorEmojiDraw::nextUtf8(p, cp) || p > end) break;
    if (before == p) break;
    w = static_cast<int16_t>(w + measureCodeWidth(cp));
  }
  return w;
}

void Canvas::drawUtf8Span(int16_t baselineScreenX, int16_t baselineScreenY,
                          const char *start, const char *end, uint16_t color) {
  int16_t penX = baselineScreenX;
  const char *p = start;
  while (p < end) {
    uint32_t cp = 0;
    const char *before = p;
    if (!ColorEmojiDraw::nextUtf8(p, cp) || p > end) break;
    if (before == p) break;

    if (emojiDrawPx_ > 0 && IconDraw::isIconCp(cp) && iconSd_ &&
        iconSd_->ready() &&
        iconSd_->draw(display_, cp, penX, baselineScreenY, emojiDrawPx_,
                      color)) {
      penX = static_cast<int16_t>(penX + iconSd_->advance(cp, emojiDrawPx_));
      continue;
    }

    if (emojiDrawPx_ > 0) {
      if (emojiSd_ && emojiSd_->ready() &&
          emojiSd_->draw(display_, cp, penX, baselineScreenY, emojiDrawPx_)) {
        penX = static_cast<int16_t>(penX + emojiSd_->advance(cp, emojiDrawPx_));
        continue;
      }
      if (emojiAtlas_) {
        const ColorEmojiGlyph *eg = ColorEmojiDraw::find(*emojiAtlas_, cp);
        if (eg) {
          ColorEmojiDraw::draw(display_, *emojiAtlas_, *eg, penX,
                               baselineScreenY, emojiDrawPx_);
          penX = static_cast<int16_t>(
              penX + ColorEmojiDraw::advance(*eg, emojiAtlas_->bakedSize,
                                             emojiDrawPx_));
          continue;
        }
      }
    }

    if (aaFont_) {
      const uint32_t folded = AAFontDraw::foldCodepoint(cp);
      AAFontDraw::drawChar(display_, *aaFont_, penX, baselineScreenY, folded,
                           color);
      penX = static_cast<int16_t>(penX +
                                  AAFontDraw::charWidth(*aaFont_, folded));
      continue;
    }

    if (cp < 0x80) {
      const char ch = static_cast<char>(cp);
      display_.setCursor(penX, baselineScreenY);
      display_.write(static_cast<uint8_t>(ch));
      penX = static_cast<int16_t>(penX + measureCharWidth(ch));
    }
  }
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
  int16_t sx0, sy0;
  contentToScreen(box.x, box.y, sx0, sy0);
  int16_t r = sx(radius);
  if (r <= 0) {
    display_.fillRect(sx0, sy0, box.w, box.h, color);
    return;
  }
  if (r * 2 > box.w) r = box.w / 2;
  if (r * 2 > box.h) r = box.h / 2;
  display_.fillRoundRect(sx0, sy0, box.w, box.h, r, color);
}

void Canvas::drawOutline(const Rect &box, uint8_t width, uint16_t color,
                         bool outside, int16_t radius) {
  const uint8_t wPx = su8(width);
  int16_t r0 = sx(radius);
  if (wPx == 0 || box.w <= 0 || box.h <= 0) return;
  int16_t sx0, sy0;
  contentToScreen(box.x, box.y, sx0, sy0);
  if (r0 * 2 > box.w) r0 = box.w / 2;
  if (r0 * 2 > box.h) r0 = box.h / 2;
  display_.strokeRoundRect(sx0, sy0, box.w, box.h, r0, wPx, color, outside);
}

DrawResult Canvas::drawText(const char *text, const TextStyle &style,
                            bool advance) {
  const int16_t boxW =
      static_cast<int16_t>(bounds_.x + bounds_.w - cx_);
  const DrawResult r =
      drawTextInBox(cx_, cy_, boxW, 10000, text, style, true);
  if (advance) {
    cx_ = bounds_.x;
    cy_ = static_cast<int16_t>(cy_ + r.h + sx(style.paragraphGap));
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
    cy_ = static_cast<int16_t>(box.y + r.h + sx(style.paragraphGap));
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
  const uint8_t mediaSize =
      style.iconSize > 0 ? style.iconSize : style.emojiSize;
  syncEmojiDrawSize(mediaSize);
  display_.setTextColor(style.color);
  display_.setTextWrap(false);

  const int16_t fontH = fontLineHeight();
  const int16_t rowH =
      (emojiDrawPx_ > fontH) ? emojiDrawPx_ : fontH;
  int16_t lineH;
  if (style.lineHeight > 0) {
    lineH = sx(static_cast<int16_t>(style.lineHeight));
    if (lineH < 1) lineH = 1;
  } else {
    // AA/GFX yAdvance is generous; default wrap a bit tighter for UI density.
    int16_t stride = static_cast<int16_t>((rowH * 4) / 5); // 80%
    const int16_t floor =
        static_cast<int16_t>(fontBaseline() + sx(3)); // keep room for descenders
    if (stride < floor) stride = floor;
    if (emojiDrawPx_ > stride) stride = emojiDrawPx_;
    lineH = static_cast<int16_t>(stride + sx(static_cast<int16_t>(style.lineGap)));
    if (lineH < 1) lineH = 1;
  }
  const int16_t baselineOffset = fontBaseline();

  int16_t penY = boxY;
  const char *p = text;
  int16_t maxLineW = 0;
  int16_t lines = 0;

  while (*p) {
    // When lineHeight is set, allow a line if the absolute line box fits;
    // otherwise require the full font metrics box (rowH).
    const int16_t needH = (style.lineHeight > 0) ? lineH : rowH;
    if ((penY - boxY) + needH > boxH) break;

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
      uint32_t cp = 0;
      const char *cpStart = p;
      if (!ColorEmojiDraw::nextUtf8(p, cp)) break;
      const int16_t cw = measureCodeWidth(cp);
      if (width + cw > boxW && cpStart > lineStart) {
        p = cpStart;
        break;
      }
      width = static_cast<int16_t>(width + cw);
      if (cp == ' ') {
        breakAt = cpStart;
        widthBeforeBreak = static_cast<int16_t>(width - cw);
      }
    }

    const char *lineEnd = p;
    int16_t lineW = width;
    if (breakAt && *p && *p != '\n') {
      lineEnd = breakAt;
      lineW = widthBeforeBreak;
      p = breakAt;
      uint32_t spaceCp = 0;
      ColorEmojiDraw::nextUtf8(p, spaceCp); // skip the space
    }

    if (lineEnd == lineStart) {
      if (!*p) break;
      uint32_t cp = 0;
      const char *cpStart = p;
      if (!ColorEmojiDraw::nextUtf8(p, cp)) break;
      lineEnd = p;
      lineW = measureCodeWidth(cp);
      if (lineEnd == cpStart) break;
    }

    int16_t drawX = boxX;
    if (style.align == Align::Center) {
      drawX = static_cast<int16_t>(boxX + (boxW - lineW) / 2);
    } else if (style.align == Align::End) {
      drawX = static_cast<int16_t>(boxX + boxW - lineW);
    }

    if (paint) {
      int16_t screenX, screenY;
      contentToScreen(drawX, static_cast<int16_t>(penY + baselineOffset),
                      screenX, screenY);
      drawUtf8Span(screenX, screenY, lineStart, lineEnd, style.color);
    }

    if (lineW > maxLineW) maxLineW = lineW;
    penY = static_cast<int16_t>(penY + lineH);
    lines++;

    if (*p == '\n') p++;
  }

  lastLineH_ = lineH;
  result.w = maxLineW;
  if (lines <= 0) {
    result.h = 0;
  } else if (style.lineHeight > 0) {
    // Absolute line box: every line (including the last) uses lineH.
    result.h = static_cast<int16_t>(lines * lineH);
  } else {
    result.h =
        static_cast<int16_t>((lines - 1) * lineH + rowH);
  }
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

UIToggle &Canvas::toggle() { return arena_.create<UIToggle>(); }

UIRange &Canvas::range() { return arena_.create<UIRange>(); }

UISelect &Canvas::select() { return arena_.create<UISelect>(); }

UISelectOption &Canvas::selectOption() {
  return arena_.create<UISelectOption>();
}

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
  UINode::setLayoutHost(this);
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->layout(0, 0, availW);
  }
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->draw(*this);
  }
  UINode::setLayoutHost(nullptr);
}
