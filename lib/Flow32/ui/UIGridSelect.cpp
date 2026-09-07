#include "UIGridSelect.h"
#include "../Canvas.h"
#include "../Icon.h"
#include "../IconSd.h"

namespace {
constexpr int16_t kIconDesign = 28;
constexpr int16_t kIconTitleGap = 6;
constexpr int16_t kCellPadV = 10;
constexpr int16_t kCellPadH = 8;
constexpr uint8_t kTitleLineH = 28;
constexpr int16_t kTitleMaxLines = 2;
constexpr int16_t kDefaultGap = 8;
constexpr uint8_t kDefaultCols = 2;

uint8_t clampGridCols(uint8_t c) {
  if (c < 2) return 2;
  if (c > 3) return 3;
  return c;
}
} // namespace

// --- UIGridSelectOption ----------------------------------------------------

UIGridSelectOption::UIGridSelectOption() {
  highlightable_ = true;
  style_.width = Length::Pct(100);
  style_.padding = Edges(kCellPadV, kCellPadH);
  style_.radius = 10;
  style_.outlineWidth = 2;
  style_.outlineOutside = true;
  applyFocusChrome();
}

void UIGridSelectOption::applyFocusChrome() {
  style_.outlineColor = Theme::active().focusRing;
}

UIGridSelectOption &UIGridSelectOption::style(const Style &s) {
  UINode::style(s);
  if (style_.outlineWidth == 0) style_.outlineWidth = 2;
  applyFocusChrome();
  return *this;
}

UIGridSelectOption &UIGridSelectOption::title(const char *s) {
  title_ = s ? s : "";
  return *this;
}

UIGridSelectOption &UIGridSelectOption::icon(const char *lucideName) {
  iconName_ = lucideName;
  return *this;
}

UIGridSelectOption &UIGridSelectOption::selected(bool v) {
  selected_ = v;
  return *this;
}

void UIGridSelectOption::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  if (style_.width.unit == Unit::Auto || w <= 0) w = availW;

  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  const int16_t innerW = static_cast<int16_t>(w - pad.left - pad.right);
  const int16_t iconH =
      (iconName_ && iconName_[0])
          ? (host ? host->sx(kIconDesign) : scalePx(kIconDesign, s))
          : 0;
  const int16_t gap =
      iconH > 0 ? (host ? host->sx(kIconTitleGap) : scalePx(kIconTitleGap, s))
                : 0;

  int16_t textH = 0;
  if (host && title_ && title_[0] && innerW > 0) {
    TextStyle ts;
    ts.font = FontRole::Title;
    ts.lineHeight = kTitleLineH;
    ts.lineGap = 0;
    textH = host->measureTextHeight(title_, innerW, ts);
    const int16_t maxH =
        host->sx(static_cast<int16_t>(kTitleLineH * kTitleMaxLines));
    if (textH > maxH) textH = maxH;
  } else if (title_ && title_[0]) {
    textH = scalePx(static_cast<int16_t>(kTitleLineH * kTitleMaxLines), s);
  }

  int16_t contentH = static_cast<int16_t>(iconH + gap + textH);
  if (contentH < iconH) contentH = iconH;

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(contentH + pad.top + pad.bottom);
  } else {
    h = host ? host->resolveLen(style_.height, 0) : style_.height.resolve(0, s);
  }
  borderBox_ = Rect(x, y, w, h);
}

void UIGridSelectOption::paintSelf(Canvas &canvas) {
  const Theme::ThemeTokens &th = Theme::active();
  const Rect &bb = borderBox_;
  const Rect content = canvas.contentBox(bb, style_.padding);

  // Soft tile fill so the grid reads as cards, not empty outlines.
  const uint16_t fill =
      selected_ ? Theme::soft(ButtonColor::Primary)
                : Theme::soft(ButtonColor::Secondary);
  canvas.fillRoundRect(bb, style_.radius > 0 ? style_.radius : 10, fill);

  const int16_t iconPx = canvas.sx(kIconDesign);
  const int16_t gap = canvas.sx(kIconTitleGap);
  IconSd *icons = canvas.iconSd();
  const Point origin = canvas.origin();

  int16_t cy = content.y;
  if (iconName_ && iconName_[0] && icons && icons->ready()) {
    const Rect iconBox(content.x, cy, content.w, iconPx);
    icons->drawInBox(canvas.display(), iconName_, iconBox, origin.x, origin.y,
                     iconPx,
                     selected_ ? Theme::brand(ButtonColor::Primary)
                               : th.baseContent,
                     IconDraw::Align::Center, IconDraw::Align::Center);
    cy = static_cast<int16_t>(cy + iconPx + gap);
  }

  if (title_ && title_[0]) {
    TextStyle ts;
    ts.font = FontRole::Title;
    ts.color = th.baseContent;
    ts.lineHeight = kTitleLineH;
    ts.lineGap = 0;
    ts.align = Align::Center;
    const int16_t maxH = canvas.sx(static_cast<int16_t>(kTitleLineH * kTitleMaxLines));
    canvas.drawText(Rect(content.x, cy, content.w, maxH), title_, ts, false);
  }
}

bool UIGridSelectOption::handleEvent(UIEvent &e) {
  if (e.key == UIKey::Select && e.phase == UIKeyPhase::Down) {
    if (group_) group_->choose(*this);
    return true;
  }
  return false;
}

// --- UIGridSelect ----------------------------------------------------------

UIGridSelect::UIGridSelect() {
  canHaveChildren_ = true;
  highlightable_ = false;
  style_.width = Length::Pct(100);
  style_.columns = kDefaultCols;
  style_.gap = kDefaultGap;
  style_.hasBackground = false;
}

UIGridSelect &UIGridSelect::style(const Style &s) {
  UINode::style(s);
  return *this;
}

UIGridSelect &UIGridSelect::add(UIGridSelectOption &opt) {
  if (optionCount_ >= kMaxOptions) return *this;
  opt.group_ = this;
  options_[optionCount_++] = &opt;
  UINode::add(opt);
  syncSelectedFlags();
  return *this;
}

void UIGridSelect::syncSelectedFlags() {
  for (uint8_t i = 0; i < optionCount_; i++) {
    if (options_[i]) {
      options_[i]->selected_ = (static_cast<int16_t>(i) == selected_);
    }
  }
}

UIGridSelect &UIGridSelect::selected(int16_t index) {
  if (index < -1) index = -1;
  selected_ = index;
  syncSelectedFlags();
  return *this;
}

UIGridSelect &UIGridSelect::selectedValue(int16_t value) {
  for (uint8_t i = 0; i < optionCount_; i++) {
    if (options_[i] && options_[i]->value_ == value) {
      return selected(static_cast<int16_t>(i));
    }
  }
  return selected(-1);
}

int16_t UIGridSelect::selectedValue() const {
  const UIGridSelectOption *o =
      optionAt(static_cast<uint8_t>(selected_ >= 0 ? selected_ : 0));
  if (selected_ < 0 || !o) return 0;
  return o->value_;
}

UIGridSelectOption *UIGridSelect::optionAt(uint8_t i) {
  return i < optionCount_ ? options_[i] : nullptr;
}

const UIGridSelectOption *UIGridSelect::optionAt(uint8_t i) const {
  return i < optionCount_ ? options_[i] : nullptr;
}

void UIGridSelect::choose(UIGridSelectOption &opt) {
  int16_t idx = -1;
  for (uint8_t i = 0; i < optionCount_; i++) {
    if (options_[i] == &opt) {
      idx = static_cast<int16_t>(i);
      break;
    }
  }
  if (idx < 0) return;
  // Always notify for navigation pickers (selected often stays −1 across
  // rebuilds). Radio UIs that need idempotent selects can ignore repeats.
  const bool changed = (idx != selected_);
  selected_ = idx;
  syncSelectedFlags();
  if (onChange_ && changed) onChange_(*this);
}

void UIGridSelect::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  if (style_.width.unit == Unit::Auto || w <= 0) w = availW;

  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  const int16_t gap = host ? host->sx(style_.gap) : scalePx(style_.gap, s);
  const int16_t innerW = static_cast<int16_t>(w - pad.left - pad.right);
  const int16_t left = static_cast<int16_t>(x + pad.left);
  const int16_t top = static_cast<int16_t>(y + pad.top);
  const uint8_t cols = clampGridCols(style_.columns);

  if (selected_ >= static_cast<int16_t>(optionCount_)) selected_ = -1;
  syncSelectedFlags();

  const int16_t gapsW = static_cast<int16_t>(gap * (cols - 1));
  int16_t cellW =
      innerW > gapsW ? static_cast<int16_t>((innerW - gapsW) / cols) : 0;
  if (cellW < 0) cellW = 0;

  int16_t rowY = top;
  int16_t maxBottom = top;
  uint8_t i = 0;
  while (i < childCount_) {
    const uint8_t rowStart = i;
    uint8_t rowCount = 0;
    int16_t rowH = 0;

    for (uint8_t c = 0; c < cols && i < childCount_; c++, i++) {
      const int16_t cellX =
          static_cast<int16_t>(left + c * (cellW + gap));
      children_[i]->layout(cellX, rowY, cellW);
      const int16_t ch = children_[i]->borderBox().h;
      if (ch > rowH) rowH = ch;
      rowCount++;
    }

    // Re-place shorter cells at the top of the row (natural height).
    for (uint8_t c = 0; c < rowCount; c++) {
      UINode *child = children_[rowStart + c];
      const int16_t cellX =
          static_cast<int16_t>(left + c * (cellW + gap));
      const Rect &cb = child->borderBox();
      if (cb.x != cellX || cb.y != rowY) {
        child->layout(cellX, rowY, cellW);
      }
    }

    maxBottom = static_cast<int16_t>(rowY + rowH);
    rowY = static_cast<int16_t>(maxBottom + gap);
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(maxBottom - y + pad.bottom);
    if (childCount_ == 0) {
      h = static_cast<int16_t>(pad.top + pad.bottom);
    }
    if (h < pad.top + pad.bottom) {
      h = static_cast<int16_t>(pad.top + pad.bottom);
    }
  } else {
    h = host ? host->resolveLen(style_.height, 0) : style_.height.resolve(0, s);
  }
  borderBox_ = Rect(x, y, w, h);
}

void UIGridSelect::paintSelf(Canvas & /*canvas*/) {}
