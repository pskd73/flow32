#pragma once

#include "Canvas.h"
#include "Rect.h"
#include "ui/UIArena.h"
#include "ui/UIEvent.h"

class UINode;
class UIDiv;
class UIButton;
class UIToggle;
class UIRange;
class UISelect;
class UISelectOption;
class UIText;
class UIImage;

/**
 * Scrollable content region with composable UI.
 *
 * Content is rasterized into a tall cache buffer; scrolling only memcpy's the
 * viewport into the panel framebuffer and presents — avoids re-layout/fonts
 * every animation frame (much smoother on slow SPI panels).
 */
class Page {
public:
  static constexpr uint8_t kMaxRoots = 8;
  static constexpr uint8_t kMaxFocusables = 32;
  static constexpr uint8_t kNoFocus = 0xFF;

  explicit Page(const Rect &viewport);
  ~Page();

  void setViewport(const Rect &r);
  Rect viewport() const { return viewport_; }

  void setContentHeight(int16_t h);
  int16_t contentHeight() const { return contentH_; }

  void setContentBackground(uint16_t color) { contentBg_ = color; }
  void invalidateContent() { contentDirty_ = true; }

  int16_t scrollY() const { return static_cast<int16_t>(scrollY_ + 0.5f); }
  int16_t maxScroll() const;
  bool scrollAnimating() const {
    const float d = scrollY_ - scrollTarget_;
    return d > 0.5f || d < -0.5f;
  }

  /** Focused control still playing a press/tap motion. */
  bool pressAnimating() const;

  /** Scroll or press motion — safe to skip UI rebuild while true. */
  bool uiAnimating() const {
    return scrollAnimating() || pressAnimating();
  }

  void scrollBy(int16_t dy);
  void scrollTo(int16_t y);
  void scrollToImmediate(int16_t y);

  void begin(Canvas &canvas);
  void end(Canvas &canvas, bool captureHeight = true);

  void beginUI();
  UIDiv &div();
  UIButton &button();
  UIToggle &toggle();
  UIRange &range();
  UISelect &select();
  UISelectOption &selectOption();
  UIText &text(const char *s);
  UIImage &image(const uint16_t *pixels, int16_t srcW, int16_t srcH);
  void add(UINode &node);
  void tick(float dt);
  void layoutUI(Canvas &canvas);
  /**
   * Rasterize if dirty, then push the scrolled viewport to the panel.
   * Prefer streaming from the content cache (no panel-FB copy) when possible.
   * Returns true if the panel was updated (caller may skip canvas.present()).
   */
  bool drawUI(Canvas &canvas);

  void syncFocus();
  void ensureFocusedVisible();
  void clearFocus();

  UINode *focused() const { return focused_; }
  bool hasFocus() const { return focusIndex_ != kNoFocus && focused_ != nullptr; }
  uint8_t focusIndex() const { return focusIndex_; }
  void setFocusIndex(uint8_t i);
  uint8_t focusCount() const { return focusCount_; }

  bool focusFirstInViewport();
  bool dispatch(UIEvent &e);

private:
  Rect viewport_;
  int16_t contentH_ = 0;
  float scrollY_ = 0;
  float scrollTarget_ = 0;
  float scrollSpeed_ = 18.0f;
  float uiScale_ = 1.0f;

  uint16_t *contentFb_ = nullptr;
  int16_t contentFbW_ = 0;
  int16_t contentFbH_ = 0;
  bool contentDirty_ = true;
  uint16_t contentBg_ = 0;
  uint8_t lastFocusIndex_ = kNoFocus;

  UIArena arena_{};
  UINode *roots_[kMaxRoots] = {};
  uint8_t rootCount_ = 0;

  uint8_t focusIndex_ = kNoFocus;
  uint8_t focusCount_ = 0;
  UINode *focused_ = nullptr;
  UINode *focusables_[kMaxFocusables] = {};

  void collectFocusables();
  bool handleDefault(UIEvent &e);
  bool intersectsViewport(const UINode &node) const;
  int16_t scrollStep() const;
  bool moveFocusInViewport(int8_t direction);
  bool browseScroll(int8_t direction);
  bool ensureContentBuffer(int16_t h);
  void rasterizeContent(Canvas &canvas);
  void blitViewport(Display &display);
  bool presentViewport(Display &display);
  int16_t lastPresentedScrollY_ = -1;
};
