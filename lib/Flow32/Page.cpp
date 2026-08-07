#include "Page.h"
#include "ui/UIButton.h"
#include "ui/UIDiv.h"
#include "ui/UIImage.h"
#include "ui/UINode.h"
#include "ui/UIText.h"

#include <math.h>

Page::Page(const Rect &viewport) : viewport_(viewport) {}

void Page::setViewport(const Rect &r) { viewport_ = r; }

void Page::setContentHeight(int16_t h) {
  contentH_ = h < 0 ? 0 : h;
  const int16_t m = maxScroll();
  if (scrollTarget_ > m) scrollTarget_ = m;
  if (scrollY_ > m) scrollY_ = m;
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
  if (step < 40) step = 40;
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
}

UIDiv &Page::div() { return arena_.create<UIDiv>(); }

UIButton &Page::button() { return arena_.create<UIButton>(); }

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
  focusIndex_ = kNoFocus;
  focused_ = nullptr;
  for (uint8_t i = 0; i < focusCount_; i++) {
    focusables_[i]->setHighlighted(false);
  }
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
  constexpr int16_t kMargin = 8;

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

  if (direction > 0) {
    for (uint8_t i = static_cast<uint8_t>(focusIndex_ + 1); i < focusCount_;
         i++) {
      if (intersectsViewport(*focusables_[i])) {
        setFocusIndex(i);
        return true;
      }
    }
  } else {
    for (int16_t i = static_cast<int16_t>(focusIndex_) - 1; i >= 0; i--) {
      if (intersectsViewport(*focusables_[static_cast<uint8_t>(i)])) {
        setFocusIndex(static_cast<uint8_t>(i));
        return true;
      }
    }
  }

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
  UIText::setLayoutCanvas(&canvas);
  int16_t maxBottom = 0;
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->layout(0, 0, viewport_.w);
    const Rect &b = roots_[i]->borderBox();
    const int16_t bottom = static_cast<int16_t>(b.y + b.h);
    if (bottom > maxBottom) maxBottom = bottom;
  }
  UIText::setLayoutCanvas(nullptr);
  setContentHeight(maxBottom);
}

void Page::drawUI(Canvas &canvas) {
  if (focused_) ensureFocusedVisible();

  canvas.setClip(viewport_);
  canvas.setOrigin(viewport_.x,
                   static_cast<int16_t>(viewport_.y - scrollY()));

  UIText::setLayoutCanvas(&canvas);
  for (uint8_t i = 0; i < rootCount_; i++) {
    roots_[i]->draw(canvas);
  }
  UIText::setLayoutCanvas(nullptr);

  canvas.clearClip();
  canvas.setOrigin(0, 0);
}
