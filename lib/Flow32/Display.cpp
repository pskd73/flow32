#include "Display.h"
#include "AAFont.h"

#include <SPI.h>
#include <esp_heap_caps.h>
#include <string.h>

Display::Display(const DisplayPanel &panel)
    : Adafruit_GFX(panel.width, panel.height), panel_(panel) {
  panel_.finalize();
}

bool Display::begin() {
  // Drive LED/BL immediately — many 1.8" modules need this pin HIGH to light.
  pinMode(panel_.pinBl, OUTPUT);
  digitalWrite(panel_.pinBl, panel_.blActiveHigh ? HIGH : LOW);

  SPI.begin(panel_.pinSclk, -1 /* MISO unused */, panel_.pinMosi, panel_.pinCs);

  if (panel_.chip == PanelChip::ST7735) {
    // Explicit MOSI/SCLK helps on ESP32 when not using the default VSPI mapping.
    st7735_ = new Adafruit_ST7735(panel_.pinCs, panel_.pinDc, panel_.pinMosi,
                                  panel_.pinSclk, panel_.pinRst);
    st7735_->initR(INITR_18BLACKTAB);
    st7735_->setSPISpeed(panel_.spiHz);
    st7735_->setRotation(panel_.rotation);
    tft_ = st7735_;
  } else {
    st7789_ = new Adafruit_ST7789(panel_.pinCs, panel_.pinDc, panel_.pinRst);
    st7789_->init(panel_.gramWidth, panel_.gramHeight);
    st7789_->setSPISpeed(panel_.spiHz);
    st7789_->setRotation(panel_.rotation);
    tft_ = st7789_;
  }

  // Re-assert after SPI init in case the bus touched nearby pins.
  setBacklight(true);

  const size_t bytes = bufferBytes();
  fb_ = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!fb_) {
    fb_ = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (!fb_ || !tft_) {
    return false;
  }
  memset(fb_, 0, bytes);

  if (panel_.needsDownscale()) {
    const size_t lineBytes =
        (size_t)panel_.nativeWidth * sizeof(uint16_t);
    presentLine_ = (uint16_t *)heap_caps_malloc(
        lineBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!presentLine_) {
      return false;
    }
  }

  useFontBody();
  setTextColor(0xFFFF);
  setTextWrap(false);
  return true;
}

void Display::setBacklight(bool on) {
  pinMode(panel_.pinBl, OUTPUT);
  const bool level = panel_.blActiveHigh ? on : !on;
  digitalWrite(panel_.pinBl, level ? HIGH : LOW);
}

void Display::useFontSmall() { setFont(&GoogleSans_Regular9pt7b); }
void Display::useFontBody() { setFont(&GoogleSans_Regular12pt7b); }
void Display::useFontBodyBold() { setFont(&GoogleSans_Bold12pt7b); }
void Display::useFontBodyLarge() { setFont(&GoogleSans_Regular18pt7b); }
void Display::useFontPlayful() { setFont(&DynaPuff_Regular12pt7b); }
void Display::useFontPlayfulLarge() { setFont(&DynaPuff_Regular18pt7b); }
void Display::useFontChildlike() { setFont(&ShortStack_Regular12pt7b); }
void Display::useFontChildlikeLarge() { setFont(&ShortStack_Regular18pt7b); }
void Display::useFontDefault() { setFont(nullptr); }

void Display::clear(uint16_t color) {
  if (!fb_) return;
  if (color == 0) {
    memset(fb_, 0, bufferBytes());
    return;
  }
  const size_t n = (size_t)panel_.width * (size_t)panel_.height;
  for (size_t i = 0; i < n; i++) fb_[i] = color;
}

void Display::setClip(int16_t x, int16_t y, int16_t w, int16_t h) {
  clipToPanel(x, y, w, h);
  clipX_ = x;
  clipY_ = y;
  clipW_ = w;
  clipH_ = h;
  clipEnabled_ = !((w <= 0) || (h <= 0));
}

void Display::clearClip() {
  clipEnabled_ = false;
  clipX_ = 0;
  clipY_ = 0;
  clipW_ = panel_.width;
  clipH_ = panel_.height;
}

bool Display::inClip(int16_t x, int16_t y) const {
  if (!clipEnabled_) return true;
  return x >= clipX_ && y >= clipY_ && x < clipX_ + clipW_ && y < clipY_ + clipH_;
}

void Display::clipToPanel(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > panel_.width) w = panel_.width - x;
  if (y + h > panel_.height) h = panel_.height - y;
  if (w < 0) w = 0;
  if (h < 0) h = 0;
}

void Display::clipToDraw(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const {
  clipToPanel(x, y, w, h);
  if (!clipEnabled_) return;
  if (x < clipX_) {
    w -= (clipX_ - x);
    x = clipX_;
  }
  if (y < clipY_) {
    h -= (clipY_ - y);
    y = clipY_;
  }
  if (x + w > clipX_ + clipW_) w = clipX_ + clipW_ - x;
  if (y + h > clipY_ + clipH_) h = clipY_ + clipH_ - y;
  if (w < 0) w = 0;
  if (h < 0) h = 0;
}

void Display::present() { present(0, 0, panel_.width, panel_.height); }

void Display::present(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!fb_ || !tft_ || w <= 0 || h <= 0) return;
  clipToPanel(x, y, w, h);
  if (w <= 0 || h <= 0) return;

  if (panel_.needsDownscale()) {
    presentDownscaled(x, y, w, h);
  } else {
    presentDirect(x, y, w, h);
  }
}

void Display::presentDirect(int16_t x, int16_t y, int16_t w, int16_t h) {
  tft_->startWrite();
  if (x == 0 && w == panel_.width) {
    tft_->setAddrWindow(0, panel_.panelYOffset + y, (uint16_t)w, (uint16_t)h);
    tft_->writePixels(fb_ + (int32_t)y * panel_.width, (uint32_t)w * h);
  } else {
    for (int16_t row = 0; row < h; row++) {
      tft_->setAddrWindow(x, panel_.panelYOffset + y + row, (uint16_t)w, 1);
      tft_->writePixels(fb_ + (int32_t)(y + row) * panel_.width + x,
                        (uint32_t)w);
    }
  }
  tft_->endWrite();
}

void Display::presentDownscaled(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!presentLine_) return;

  const int16_t sx = panel_.scaleX();
  const int16_t sy = panel_.scaleY();
  if (sx < 1 || sy < 1) return;
  if (panel_.width != panel_.nativeWidth * sx ||
      panel_.height != panel_.nativeHeight * sy) {
    // Non-integer scale not supported yet — fall back to full-frame integer path
    // only when dimensions divide evenly (Panel18).
    return;
  }

  // Expand dirty rect to whole source blocks.
  const int16_t x0 = (x / sx) * sx;
  const int16_t y0 = (y / sy) * sy;
  const int16_t x1 = ((x + w + sx - 1) / sx) * sx;
  const int16_t y1 = ((y + h + sy - 1) / sy) * sy;

  const int16_t nx = x0 / sx;
  const int16_t ny = y0 / sy;
  const int16_t nw = (x1 - x0) / sx;
  const int16_t nh = (y1 - y0) / sy;
  if (nw <= 0 || nh <= 0) return;

  const int32_t samples = (int32_t)sx * (int32_t)sy;
  const int16_t fbW = panel_.width;

  tft_->startWrite();
  for (int16_t row = 0; row < nh; row++) {
    const int16_t srcY0 = y0 + row * sy;
    for (int16_t col = 0; col < nw; col++) {
      const int16_t srcX0 = x0 + col * sx;
      uint32_t rSum = 0, gSum = 0, bSum = 0;
      for (int16_t dy = 0; dy < sy; dy++) {
        const uint16_t *src =
            fb_ + (int32_t)(srcY0 + dy) * fbW + srcX0;
        for (int16_t dx = 0; dx < sx; dx++) {
          const uint16_t c = src[dx];
          rSum += (c >> 11) & 0x1F;
          gSum += (c >> 5) & 0x3F;
          bSum += c & 0x1F;
        }
      }
      presentLine_[col] = (uint16_t)(((rSum / samples) << 11) |
                                     ((gSum / samples) << 5) |
                                     (bSum / samples));
    }
    tft_->setAddrWindow(nx, panel_.panelYOffset + ny + row, (uint16_t)nw, 1);
    tft_->writePixels(presentLine_, (uint32_t)nw);
  }
  tft_->endWrite();
}

void Display::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!fb_ || x < 0 || y < 0 || x >= panel_.width || y >= panel_.height) return;
  if (!inClip(x, y)) return;
  fb_[(int32_t)y * panel_.width + x] = color;
}

uint16_t Display::getPixel(int16_t x, int16_t y) const {
  if (!fb_ || x < 0 || y < 0 || x >= panel_.width || y >= panel_.height) return 0;
  return fb_[(int32_t)y * panel_.width + x];
}

void Display::blendPixel(int16_t x, int16_t y, uint16_t fg, uint8_t cover4) {
  if (!fb_ || cover4 == 0) return;
  if (x < 0 || y < 0 || x >= panel_.width || y >= panel_.height) return;
  if (!inClip(x, y)) return;
  if (cover4 >= 15) {
    fb_[(int32_t)y * panel_.width + x] = fg;
    return;
  }
  uint16_t *p = &fb_[(int32_t)y * panel_.width + x];
  *p = AAFontDraw::blend565(fg, *p, cover4);
}

void Display::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if (!fb_ || y < 0 || y >= panel_.height) return;
  if (clipEnabled_ && (y < clipY_ || y >= clipY_ + clipH_)) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (x + w > panel_.width) w = panel_.width - x;
  if (clipEnabled_) {
    if (x < clipX_) {
      w -= (clipX_ - x);
      x = clipX_;
    }
    if (x + w > clipX_ + clipW_) w = clipX_ + clipW_ - x;
  }
  if (w <= 0) return;
  uint16_t *row = fb_ + (int32_t)y * panel_.width + x;
  for (int16_t i = 0; i < w; i++) row[i] = color;
}

void Display::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if (!fb_ || x < 0 || x >= panel_.width) return;
  if (clipEnabled_ && (x < clipX_ || x >= clipX_ + clipW_)) return;
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (y + h > panel_.height) h = panel_.height - y;
  if (clipEnabled_) {
    if (y < clipY_) {
      h -= (clipY_ - y);
      y = clipY_;
    }
    if (y + h > clipY_ + clipH_) h = clipY_ + clipH_ - y;
  }
  if (h <= 0) return;
  for (int16_t i = 0; i < h; i++) {
    fb_[(int32_t)(y + i) * panel_.width + x] = color;
  }
}

void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       uint16_t color) {
  if (!fb_) return;
  clipToDraw(x, y, w, h);
  for (int16_t row = 0; row < h; row++) {
    drawFastHLine(x, y + row, w, color);
  }
}

void Display::drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap,
                            int16_t w, int16_t h) {
  if (!fb_ || !bitmap) return;
  for (int16_t j = 0; j < h; j++) {
    int16_t dy = y + j;
    if (dy < 0 || dy >= panel_.height) continue;
    if (clipEnabled_ && (dy < clipY_ || dy >= clipY_ + clipH_)) continue;
    for (int16_t i = 0; i < w; i++) {
      int16_t dx = x + i;
      if (dx < 0 || dx >= panel_.width) continue;
      if (!inClip(dx, dy)) continue;
      fb_[(int32_t)dy * panel_.width + dx] = bitmap[(int32_t)j * w + i];
    }
  }
}

void Display::blitHLine(int16_t x, int16_t y, const uint16_t *src, int16_t w) {
  if (!fb_ || !src || y < 0 || y >= panel_.height) return;
  if (clipEnabled_ && (y < clipY_ || y >= clipY_ + clipH_)) return;
  if (x < 0) {
    w += x;
    src -= x;
    x = 0;
  }
  if (x + w > panel_.width) w = panel_.width - x;
  if (clipEnabled_) {
    if (x < clipX_) {
      const int16_t skip = clipX_ - x;
      w -= skip;
      src += skip;
      x = clipX_;
    }
    if (x + w > clipX_ + clipW_) w = clipX_ + clipW_ - x;
  }
  if (w <= 0) return;
  memcpy(fb_ + (int32_t)y * panel_.width + x, src, (size_t)w * sizeof(uint16_t));
}

bool Display::insideRound(int16_t x, int16_t y) const {
  return insideRound(x, y, panel_.cornerRadius);
}

bool Display::insideRound(int16_t x, int16_t y, int16_t r) const {
  if (x < 0 || y < 0 || x >= panel_.width || y >= panel_.height) return false;
  if (r <= 0) return true;

  if (x < r && y < r) {
    const int32_t dx = r - 1 - x;
    const int32_t dy = r - 1 - y;
    return dx * dx + dy * dy <= (int32_t)(r - 1) * (r - 1);
  }
  if (x >= panel_.width - r && y < r) {
    const int32_t dx = x - (panel_.width - r);
    const int32_t dy = r - 1 - y;
    return dx * dx + dy * dy <= (int32_t)(r - 1) * (r - 1);
  }
  if (x < r && y >= panel_.height - r) {
    const int32_t dx = r - 1 - x;
    const int32_t dy = y - (panel_.height - r);
    return dx * dx + dy * dy <= (int32_t)(r - 1) * (r - 1);
  }
  if (x >= panel_.width - r && y >= panel_.height - r) {
    const int32_t dx = x - (panel_.width - r);
    const int32_t dy = y - (panel_.height - r);
    return dx * dx + dy * dy <= (int32_t)(r - 1) * (r - 1);
  }
  return true;
}

void Display::drawPanelRoundRect(uint16_t color) {
  const int16_t r = panel_.cornerRadius;
  if (r <= 0) {
    drawRect(0, 0, panel_.width, panel_.height, color);
    return;
  }
  drawRoundRect(0, 0, panel_.width, panel_.height, r, color);
  drawRoundRect(1, 1, panel_.width - 2, panel_.height - 2, r > 1 ? r - 1 : 1,
                color);
}

void Display::fillPanelRoundRect(uint16_t color) {
  if (panel_.cornerRadius <= 0) {
    fillRect(0, 0, panel_.width, panel_.height, color);
    return;
  }
  fillRoundRect(0, 0, panel_.width, panel_.height, panel_.cornerRadius, color);
}
