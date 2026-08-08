#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST77xx.h>
#include "GoogleSans9pt7b.h"
#include "GoogleSans12pt7b.h"
#include "GoogleSans18pt7b.h"
#include "GoogleSansBold12pt7b.h"
#include "DynaPuff12pt7b.h"
#include "DynaPuff18pt7b.h"
#include "ShortStack12pt7b.h"
#include "ShortStack18pt7b.h"

#include "DisplayPanel.h"

/**
 * Smooth RGB565 display.
 *
 * Drawing goes into an off-screen framebuffer (PSRAM when available).
 * Call present() to push one SPI burst — never paint the panel mid-frame.
 *
 * pushDrawTarget() redirects drawing into an external buffer (e.g. page
 * content cache) while present() always blits the panel-sized buffer.
 */
class Display : public Adafruit_GFX {
public:
  explicit Display(const DisplayPanel &panel);

  bool begin();

  const DisplayPanel &panel() const { return panel_; }
  /** @deprecated Prefer panel() */
  const DisplayPanel &config() const { return panel_; }

  void setBacklight(bool on);

  /** Current draw target (panel FB or pushDrawTarget). */
  uint16_t *buffer() { return fb_; }
  const uint16_t *buffer() const { return fb_; }
  /** Panel front buffer used by present(). */
  uint16_t *panelBuffer() { return fbPanel_; }
  const uint16_t *panelBuffer() const { return fbPanel_; }

  size_t bufferBytes() const {
    return (size_t)panel_.width * (size_t)panel_.height * sizeof(uint16_t);
  }

  void present();
  void present(int16_t x, int16_t y, int16_t w, int16_t h);
  /** Push a contiguous RGB565 rectangle from an external buffer (one SPI burst). */
  void presentBuffer(const uint16_t *src, int16_t x, int16_t y, int16_t w,
                     int16_t h);

  int16_t width() const { return panel_.width; }
  int16_t height() const { return panel_.height; }
  int16_t drawWidth() const { return targetW_; }
  int16_t drawHeight() const { return targetH_; }

  /** Redirect drawing into an external RGB565 buffer (w×h). */
  void pushDrawTarget(uint16_t *fb, int16_t w, int16_t h);
  void popDrawTarget();

  void setClip(int16_t x, int16_t y, int16_t w, int16_t h);
  void clearClip();
  bool hasClip() const { return clipEnabled_; }
  /** Current clip in draw-target pixels (false if clip disabled). */
  bool clipRect(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const {
    if (!clipEnabled_) return false;
    x = clipX_;
    y = clipY_;
    w = clipW_;
    h = clipH_;
    return true;
  }

  void clear(uint16_t color = 0);

  bool insideRound(int16_t x, int16_t y) const;
  bool insideRound(int16_t x, int16_t y, int16_t r) const;

  void drawPanelRoundRect(uint16_t color);
  void fillPanelRoundRect(uint16_t color);

  void useFontSmall();
  void useFontBody();
  void useFontBodyBold();
  void useFontBodyLarge();
  void useFontPlayful();
  void useFontPlayfulLarge();
  void useFontChildlike();
  void useFontChildlikeLarge();
  void useFontDefault();
  const GFXfont *font() const { return gfxFont; }
  uint8_t textSize() const { return textsize_x; }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

  /**
   * Anti-aliased rounded rect fill (soft corners via coverage blend).
   * Hides Adafruit_GFX::fillRoundRect when called on Display.
   */
  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                     uint16_t color);

  /**
   * Anti-aliased rounded stroke. `outside` grows beyond the box; otherwise
   * the stroke sits inside the box edge (Adafruit drawRoundRect semantics).
   */
  void strokeRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                       uint8_t strokeW, uint16_t color, bool outside = false);

  /** 1px inside AA stroke (compatible with Adafruit_GFX::drawRoundRect). */
  void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                     uint16_t color);

  uint16_t getPixel(int16_t x, int16_t y) const;
  void blendPixel(int16_t x, int16_t y, uint16_t fg, uint8_t cover4);

  using Adafruit_GFX::drawRGBBitmap;
  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w,
                     int16_t h);

  void blitHLine(int16_t x, int16_t y, const uint16_t *src, int16_t w);

  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  bool ready() const { return fbPanel_ != nullptr && tft_ != nullptr; }

  int16_t cornerRadius() const { return panel_.cornerRadius; }

private:
  DisplayPanel panel_;
  Adafruit_ST7789 *st7789_ = nullptr;
  Adafruit_ST7735 *st7735_ = nullptr;
  Adafruit_ST77xx *tft_ = nullptr;

  uint16_t *fbPanel_ = nullptr; // presented to glass
  uint16_t *fb_ = nullptr;      // current draw target
  int16_t targetW_ = 0;
  int16_t targetH_ = 0;

  uint16_t *savedFb_ = nullptr;
  int16_t savedW_ = 0;
  int16_t savedH_ = 0;
  bool targetPushed_ = false;

  bool clipEnabled_ = false;
  int16_t clipX_ = 0;
  int16_t clipY_ = 0;
  int16_t clipW_ = 0;
  int16_t clipH_ = 0;

  void clipToTarget(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const;
  void clipToDraw(int16_t &x, int16_t &y, int16_t &w, int16_t &h) const;
  bool inClip(int16_t x, int16_t y) const;
};
