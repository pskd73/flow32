#include "Page.h"
#include "ui/UIButton.h"
#include "ui/UIDiv.h"
#include "ui/UIImage.h"
#include "ui/UINode.h"
#include "ui/UIText.h"
#include "ui/UIToggle.h"
#include "ui/UIRange.h"

#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>

Page::Page(const Rect &viewport) : viewport_(viewport) {}

Page::~Page() {
  if (contentFb_) {
    heap_caps_free(contentFb_);
    contentFb_ = nullptr;
  }
}

void Page::setViewport(const Rect &r) {
  viewport_ = r;
  contentDirty_ = true;
}

void Page::setContentHeight(int16_t h) {
  contentH_ = h < 0 ? 0 : h;
  const int16_t m = maxScroll();
  if (scrollTarget_ > m) scrollTarget_ = m;
  if (scrollY_ > m) scrollY_ = m;
}

bool Page::pressAnimating() const {
  return focused_ && focused_->visualAnimating();
}

int16_t Page::maxScroll() const {
  const int16_t m = static_cast<int16_t>(contentH_ - viewport_.h);
  return m > 0 ? m : 0;
}

void Page::scrollBy(int16_t dy) {
  scrollTo(static_cast<int16_t>(scrollTarget_ + dy));
}

void Page::scrollTo(int16_t y) {
  if (y < 0) y = 0;
  const int16_t m = maxScroll();
  if (y > m) y = m;
  scrollTarget_ = y;
}

void Page::scrollToImmediate(int16_t y) {
  if (y < 0) y = 0;
  const int16_t m = maxScroll();
  if (y > m) y = m;
  scrollTarget_ = y;
  scrollY_ = y;
}

int16_t Page::scrollStep() const {
  int16_t step = static_cast<int16_t>(viewport_.h / 3);
  const int16_t minStep = scalePx(24, uiScale_);
  if (step < minStep) step = minStep;
  return step;
}

void Page::begin(Canvas &canvas) {
  canvas.setClip(viewport_);
  canvas.setOrigin(viewport_.x,
                   static_cast<int16_t>(viewport_.y - scrollY()));
  const int16_t h = contentH_ > 0 ? contentH_ : static_cast<int16_t>(10000);
  canvas.setBounds(Rect(0, 0, viewport_.w, h));
  canvas.resetFlow();
}

void Page::end(Canvas &canvas, bool captureHeight) {
  if (captureHeight) {
    setContentHeight(canvas.cursor().y);
  }
  canvas.clearClip();
  canvas.setOrigin(0, 0);
}

void Page::beginUI() {
  arena_.reset();
  rootCount_ = 0;
  focused_ = nullptr;
  focusCount_ = 0;
  // Keep contentFb cache; invalidate only on focus/UI changes.
}

UIDiv &Page::div() { return arena_.create<UIDiv>(); }

UIButton &Page::button() { return arena_.create<UIButton>(); }

UIToggle &Page::toggle() { return arena_.create<UIToggle>(); }

UIRange &Page::range() { return arena_.create<UIRange>(); }

UIText &Page::text(const char *s) { return arena_.create<UIText>(s); }

UIImage &Page::image(const uint16_t *pixels, int16_t srcW, int16_t srcH) {
  return arena_.create<UIImage>(pixels, srcW, srcH);
}

void Page::add(UINode &node) {
  if (rootCount_ >= kMaxRoots) return;
  node.parent_ = nullptr;
  roots_[rootCount_++] = &node;
}

void Page::tick(float dt) {
  if (dt < 0) dt = 0;
  if (dt > 0.1f) dt = 0.1f;

  const float diff = scrollTarget_ - scrollY_;
  if (diff > 0.5f || diff < -0.5f) {
    float t = 1.0f - expf(-scrollSpeed_ * dt);
    if (t > 1.0f) t = 1.0f;
    scrollY_ += diff * t;
  } else {
    scrollY_ = scrollTarget_;
  }

  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->tick(dt);
  }

  // Press dip/dim lives in node anim/chrome — must re-rasterize the cache.
  if (focused_ && focused_->visualAnimating()) {
    contentDirty_ = true;
  }
}

void Page::collectFocusables() {
  focusCount_ = 0;
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->collectHighlightable(focusables_, focusCount_, kMaxFocusables);
  }
}

bool Page::intersectsViewport(const UINode &node) const {
  const Rect &b = node.borderBox();
  if (b.w <= 0 || b.h <= 0) return false;
  const int16_t viewTop = scrollY();
  const int16_t viewBottom = static_cast<int16_t>(scrollY() + viewport_.h);
  const int16_t top = b.y;
  const int16_t bottom = static_cast<int16_t>(b.y + b.h);
  return bottom > viewTop && top < viewBottom;
}

void Page::clearFocus() {
  if (focusIndex_ != kNoFocus) contentDirty_ = true;
  focusIndex_ = kNoFocus;
  focused_ = nullptr;
  for (uint8_t i = 0; i < focusCount_; i++) {
    focusables_[i]->setHighlighted(false);
  }
  lastFocusIndex_ = kNoFocus;
}

void Page::syncFocus() {
  collectFocusables();
  if (focusIndex_ != kNoFocus &&
      (focusCount_ == 0 || focusIndex_ >= focusCount_)) {
    focusIndex_ = kNoFocus;
  }

  focused_ = nullptr;
  for (uint8_t i = 0; i < focusCount_; i++) {
    const bool on = (focusIndex_ != kNoFocus && i == focusIndex_);
    focusables_[i]->setHighlighted(on);
    if (on) focused_ = focusables_[i];
  }

  if (focusIndex_ != lastFocusIndex_) {
    contentDirty_ = true;
    lastFocusIndex_ = focusIndex_;
  }

  if (focused_) ensureFocusedVisible();
}

void Page::ensureFocusedVisible() {
  if (!focused_) return;
  const Rect &b = focused_->borderBox();
  if (b.w <= 0 || b.h <= 0) return;

  const int16_t top = b.y;
  const int16_t bottom = static_cast<int16_t>(b.y + b.h);
  const int16_t viewTop = static_cast<int16_t>(scrollTarget_);
  const int16_t viewBottom =
      static_cast<int16_t>(scrollTarget_ + viewport_.h);
  const int16_t kMargin = scalePx(8, uiScale_);

  if (top < viewTop + kMargin) {
    scrollTo(static_cast<int16_t>(top - kMargin));
  } else if (bottom > viewBottom - kMargin) {
    scrollTo(static_cast<int16_t>(bottom - viewport_.h + kMargin));
  }
}

void Page::setFocusIndex(uint8_t i) {
  focusIndex_ = i;
  syncFocus();
}

bool Page::focusFirstInViewport() {
  collectFocusables();
  for (uint8_t i = 0; i < focusCount_; i++) {
    if (intersectsViewport(*focusables_[i])) {
      setFocusIndex(i);
      return true;
    }
  }
  return false;
}

bool Page::browseScroll(int8_t direction) {
  const float before = scrollTarget_;
  scrollBy(static_cast<int16_t>(direction * scrollStep()));
  return scrollTarget_ != before;
}

bool Page::moveFocusInViewport(int8_t direction) {
  collectFocusables();
  if (focusCount_ == 0 || focusIndex_ == kNoFocus) return false;

  // Move along the full focus list (skipping non-highlightable like disabled).
  // ensureFocusedVisible() (via syncFocus) scrolls the new target into view.
  if (direction > 0) {
    if (focusIndex_ + 1 < focusCount_) {
      setFocusIndex(static_cast<uint8_t>(focusIndex_ + 1));
      return true;
    }
  } else {
    if (focusIndex_ > 0) {
      setFocusIndex(static_cast<uint8_t>(focusIndex_ - 1));
      return true;
    }
  }

  // Past the first/last focusable — release focus (back to browse scroll).
  clearFocus();
  browseScroll(direction);
  return true;
}

bool Page::handleDefault(UIEvent &e) {
  if (e.phase != UIKeyPhase::Down && e.phase != UIKeyPhase::Hold) {
    return false;
  }

  switch (e.key) {
  case UIKey::Select:
    if (hasFocus()) return false;
    return focusFirstInViewport();

  case UIKey::Down:
  case UIKey::Right:
    if (hasFocus()) return moveFocusInViewport(+1);
    return browseScroll(+1);

  case UIKey::Up:
  case UIKey::Left:
    if (hasFocus()) return moveFocusInViewport(-1);
    return browseScroll(-1);

  case UIKey::Back:
    if (hasFocus()) {
      clearFocus();
      return true;
    }
    return false;

  default:
    return false;
  }
}

bool Page::dispatch(UIEvent &e) {
  if (focused_ && focused_->dispatch(e)) {
    return true;
  }
  if (handleDefault(e)) {
    e.handled = true;
    return true;
  }
  return false;
}

void Page::layoutUI(Canvas &canvas) {
  uiScale_ = canvas.uiScale();
  UINode::setLayoutHost(&canvas);
  int16_t maxBottom = 0;
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->layout(0, 0, viewport_.w);
    const Rect &b = roots_[i]->borderBox();
    const int16_t bottom = static_cast<int16_t>(b.y + b.h);
    if (bottom > maxBottom) maxBottom = bottom;
  }
  UINode::setLayoutHost(nullptr);
  setContentHeight(maxBottom);
}

bool Page::ensureContentBuffer(int16_t h) {
  if (h < viewport_.h) h = viewport_.h;
  if (contentFb_ && contentFbW_ == viewport_.w && contentFbH_ >= h) {
    return true;
  }
  if (contentFb_) {
    heap_caps_free(contentFb_);
    contentFb_ = nullptr;
  }
  const size_t bytes = (size_t)viewport_.w * (size_t)h * sizeof(uint16_t);
  contentFb_ =
      (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!contentFb_) {
    contentFb_ = (uint16_t *)heap_caps_malloc(
        bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (!contentFb_) return false;
  contentFbW_ = viewport_.w;
  contentFbH_ = h;
  contentDirty_ = true;
  return true;
}

void Page::rasterizeContent(Canvas &canvas) {
  Display &disp = canvas.display();
  int16_t h = contentH_;
  if (h < viewport_.h) h = viewport_.h;
  if (!ensureContentBuffer(h)) return;

  disp.pushDrawTarget(contentFb_, contentFbW_, contentFbH_);
  disp.clear(contentBg_);
  canvas.setOrigin(0, 0);
  canvas.clearClip();
  canvas.setBounds(Rect(0, 0, contentFbW_, contentFbH_));

  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->draw(canvas);
  }

  disp.popDrawTarget();
  contentDirty_ = false;
}

void Page::blitViewport(Display &display) {
  uint16_t *dst = display.panelBuffer();
  if (!dst || !contentFb_) return;

  const int16_t w = viewport_.w;
  const int16_t h = viewport_.h;
  const int16_t dstW = display.width();
  int16_t srcY = scrollY();
  if (srcY < 0) srcY = 0;
  if (srcY > maxScroll()) srcY = maxScroll();

  for (int16_t row = 0; row < h; row++) {
    const int16_t sy = static_cast<int16_t>(srcY + row);
    const int16_t dy = static_cast<int16_t>(viewport_.y + row);
    if (dy < 0 || dy >= display.height()) continue;
    uint16_t *drow = dst + (int32_t)dy * dstW + viewport_.x;
    if (sy >= 0 && sy < contentFbH_) {
      memcpy(drow, contentFb_ + (int32_t)sy * w, (size_t)w * sizeof(uint16_t));
    } else {
      for (int16_t x = 0; x < w; x++) drow[x] = contentBg_;
    }
  }
}

bool Page::presentViewport(Display &display) {
  if (!contentFb_) return false;

  const int16_t w = viewport_.w;
  const int16_t h = viewport_.h;
  int16_t srcY = scrollY();
  if (srcY < 0) srcY = 0;
  if (srcY > maxScroll()) srcY = maxScroll();

  // Copy into the panel FB then SPI-present from there. Streaming PSRAM
  // content through writePixels is unreliable on some ESP32-S3 setups.
  blitViewport(display);
  display.present(viewport_.x, viewport_.y, w, h);
  lastPresentedScrollY_ = srcY;
  return true;
}

bool Page::drawUI(Canvas &canvas) {
  uiScale_ = canvas.uiScale();
  if (focused_) ensureFocusedVisible();

  const int16_t sy = scrollY();
  if (contentDirty_) {
    rasterizeContent(canvas);
  } else if (!contentDirty_ && sy == lastPresentedScrollY_ && contentFb_) {
    return false; // identical frame already on glass
  }

  return presentViewport(canvas.display());
}
