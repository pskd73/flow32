#pragma once

#include "Canvas.h"
#include "Rect.h"
#include "ui/UIArena.h"
#include "ui/UIEvent.h"

class UINode;
class UIDiv;
class UIButton;
class UIText;
class UIImage;

/**
 * Scrollable content region with the same composable UI API as Canvas.
 *
 * Focus model:
 * - Nothing focused by default (browse/scroll).
 * - Select (unfocused): focus first focusable intersecting the viewport.
 * - Up/Down while focused: move to next in-viewport focusable in that
 *   direction; if none, clear focus and scroll.
 * - Up/Down while unfocused: scroll only.
 *
 * focusIndex_ persists across beginUI() rebuilds; focused() is valid after
 * syncFocus() until the next beginUI().
 */
class Page {
public:
  static constexpr uint8_t kMaxRoots = 8;
  static constexpr uint8_t kMaxFocusables = 32;
  static constexpr uint8_t kNoFocus = 0xFF;

  explicit Page(const Rect &viewport);

  void setViewport(const Rect &r);
  Rect viewport() const { return viewport_; }

  void setContentHeight(int16_t h);
  int16_t contentHeight() const { return contentH_; }

  int16_t scrollY() const { return static_cast<int16_t>(scrollY_ + 0.5f); }
  int16_t maxScroll() const;
  bool scrollAnimating() const {
    const float d = scrollY_ - scrollTarget_;
    return d > 0.5f || d < -0.5f;
  }

  /** Animate scroll to y (clamped). */
  void scrollBy(int16_t dy);
  void scrollTo(int16_t y);
  /** Jump without animation (content resize / hard reset). */
  void scrollToImmediate(int16_t y);

  /** Legacy flow helpers (still used by low-level Canvas drawing). */
  void begin(Canvas &canvas);
  void end(Canvas &canvas, bool captureHeight = true);

  // --- composable UI (own arena) ---
  void beginUI();
  UIDiv &div();
  UIButton &button();
  UIText &text(const char *s);
  UIImage &image(const uint16_t *pixels, int16_t srcW, int16_t srcH);
  void add(UINode &node);
  void tick(float dt);
  /** Layout roots in content space; updates content height. Call before dispatch. */
  void layoutUI(Canvas &canvas);
  void drawUI(Canvas &canvas);

  /** Apply focusIndex_ highlights; refresh focused_ for this frame. */
  void syncFocus();
  /** Scroll so the focused node is fully inside the viewport (needs layout). */
  void ensureFocusedVisible();
  void clearFocus();

  UINode *focused() const { return focused_; }
  bool hasFocus() const { return focusIndex_ != kNoFocus && focused_ != nullptr; }
  uint8_t focusIndex() const { return focusIndex_; }
  void setFocusIndex(uint8_t i);
  uint8_t focusCount() const { return focusCount_; }

  /** Focus first focusable that intersects the viewport. */
  bool focusFirstInViewport();

  bool dispatch(UIEvent &e);

private:
  Rect viewport_;
  int16_t contentH_ = 0;
  float scrollY_ = 0;       // rendered position
  float scrollTarget_ = 0;  // animated destination
  float scrollSpeed_ = 14.0f; // higher = snappier ease

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
};
