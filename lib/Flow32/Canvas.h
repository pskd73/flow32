#pragma once

#include "AAFont.h"
#include "ColorEmoji.h"
#include "ColorEmojiSd.h"
#include "Display.h"
#include "Rect.h"
#include "ui/Style.h"
#include "ui/UIArena.h"

class UINode;
class UIDiv;
class UIButton;
class UIToggle;
class UIRange;
class UIText;
class UIImage;

struct TextStyle {
  FontRole font = FontRole::Body;
  uint16_t color = 0xFFFF;
  /** Absolute line box height in design px (0 = font + lineGap). */
  uint8_t lineHeight = 0;
  uint8_t lineGap = 1;
  uint8_t paragraphGap = 8;
  /**
   * Force emoji draw size in design px (0 = match font line height).
   * May exceed atlas bakedSize (upscales; softer).
   */
  uint8_t emojiSize = 0;
  Align align = Align::Start;
};

struct DrawResult {
  int16_t w = 0;
  int16_t h = 0;
  int16_t endX = 0;
  int16_t endY = 0;
};

struct Point {
  int16_t x;
  int16_t y;

  Point() : x(0), y(0) {}
  Point(int16_t x_, int16_t y_) : x(x_), y(y_) {}
};

/**
 * Content drawing layer on top of Display.
 * Also hosts a small UI arena for composable UIDiv/UIText/UIImage trees.
 */
class Canvas {
public:
  static constexpr uint8_t kMaxRoots = 8;

  explicit Canvas(Display &display);

  Display &display() { return display_; }
  const Display &display() const { return display_; }

  /** Panel UI density. Style px are design units; Canvas converts at draw/layout. */
  float uiScale() const { return display_.panel().uiScale; }
  int16_t sx(int16_t px) const { return scalePx(px, uiScale()); }
  uint8_t su8(uint8_t px) const { return scaleU8(px, uiScale()); }
  Edges scaledPad(const Edges &pad) const { return scaleEdges(pad, uiScale()); }
  int16_t resolveLen(const Length &len, int16_t parent) const {
    return len.resolve(parent, uiScale());
  }
  Rect contentBox(const Rect &border, const Edges &designPad) const {
    return contentRect(border, scaledPad(designPad));
  }

  void clear(uint16_t color = 0);
  void present();
  void present(const Rect &r);

  void setOrigin(int16_t x, int16_t y);
  Point origin() const { return Point(originX_, originY_); }

  void setClip(const Rect &r);
  void clearClip();

  void setBounds(const Rect &r);
  Rect bounds() const { return bounds_; }

  void moveTo(int16_t x, int16_t y);
  void moveBy(int16_t dx, int16_t dy);
  Point cursor() const { return Point(cx_, cy_); }

  void resetFlow();
  void newLine(int16_t extra = 0);
  void gap(int16_t dy);

  /** Color emoji atlas in flash (default null = no emoji). */
  void setEmojiAtlas(const ColorEmojiAtlas *atlas) {
    emojiAtlas_ = atlas;
    if (atlas) emojiSd_ = nullptr;
  }
  const ColorEmojiAtlas *emojiAtlas() const { return emojiAtlas_; }

  /** SD-backed emoji (preferred for large sets). Clears flash atlas pointer. */
  void setEmojiSd(ColorEmojiSd *sd) {
    emojiSd_ = sd;
    if (sd) emojiAtlas_ = nullptr;
  }
  ColorEmojiSd *emojiSd() const { return emojiSd_; }

  DrawResult drawText(const char *text, const TextStyle &style,
                      bool advance = true);
  DrawResult drawText(const Rect &box, const char *text, const TextStyle &style,
                      bool advance = false);

  /** Measure wrapped text height without drawing. */
  int16_t measureTextHeight(const char *text, int16_t maxW,
                            const TextStyle &style);

  DrawResult drawImage(const uint16_t *pixels, int16_t srcW, int16_t srcH,
                       int16_t boxW, int16_t boxH,
                       ImageFit fit = ImageFit::Cover, bool advance = true);
  DrawResult drawImage(const Rect &box, const uint16_t *pixels, int16_t srcW,
                       int16_t srcH, ImageFit fit = ImageFit::Cover,
                       bool advance = false);

  void fillRect(const Rect &box, uint16_t color);
  /** radius is design px — scaled by uiScale before rasterizing. */
  void fillRoundRect(const Rect &box, int16_t radius, uint16_t color);
  /** width/radius are design px — scaled by uiScale before rasterizing. */
  void drawOutline(const Rect &box, uint8_t width, uint16_t color,
                   bool outside = true, int16_t radius = 0);

  // --- composable UI ---
  void beginUI();
  UIDiv &div();
  UIButton &button();
  UIToggle &toggle();
  UIRange &range();
  UIText &text(const char *s);
  UIImage &image(const uint16_t *pixels, int16_t srcW, int16_t srcH);
  void add(UINode &node);
  void tick(float dt);
  /** Layout + paint all roots at (0,0) with availW = display width. */
  void drawUI();
  void drawUI(int16_t availW);

  UIArena &arena() { return arena_; }

private:
  Display &display_;
  int16_t cx_ = 0;
  int16_t cy_ = 0;
  Rect bounds_{};
  int16_t originX_ = 0;
  int16_t originY_ = 0;
  int16_t lastLineH_ = 0;
  const AAFont *aaFont_ = nullptr;
  const ColorEmojiAtlas *emojiAtlas_ = nullptr;
  ColorEmojiSd *emojiSd_ = nullptr;
  int16_t emojiDrawPx_ = 0;

  UIArena arena_{};
  UINode *roots_[kMaxRoots] = {};
  uint8_t rootCount_ = 0;

  void applyFont(FontRole role);
  int16_t fontLineHeight() const;
  int16_t fontBaseline() const;
  int16_t measureCharWidth(char c) const;
  int16_t measureCodeWidth(uint32_t cp) const;
  int16_t measureUtf8Width(const char *start, const char *end) const;
  void syncEmojiDrawSize(uint8_t overridePx = 0);
  void drawUtf8Span(int16_t baselineScreenX, int16_t baselineScreenY,
                    const char *start, const char *end, uint16_t color);

  DrawResult drawTextInBox(int16_t boxX, int16_t boxY, int16_t boxW,
                           int16_t boxH, const char *text,
                           const TextStyle &style, bool paint);

  void contentToScreen(int16_t cx, int16_t cy, int16_t &sx, int16_t &sy) const;
  void blitScaled(const Rect &dstScreen, const uint16_t *pixels, int16_t srcW,
                  int16_t srcH, int32_t srcX0, int32_t srcY0, int32_t srcX1,
                  int32_t srcY1);
};
