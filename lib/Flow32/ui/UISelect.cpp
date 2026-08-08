#include "UISelect.h"
#include "../Canvas.h"
#include "../Icon.h"
#include "../IconSd.h"

namespace {
constexpr int16_t kIconDesign = 18;
constexpr int16_t kCheckDesign = 16;
constexpr int16_t kRowPadV = 6;
constexpr int16_t kRowPadH = 10;
constexpr int16_t kIconGap = 8;
/** Title stack height (Small baseline is 18; full yAdvance is ~25). */
constexpr uint8_t kTitleLineH = 18;
/** Description wrap stride (was 12; +0.5 line ≈ +6). */
constexpr uint8_t kDescLineH = 18;
/** Space between title and description (design px). */
constexpr int16_t kTitleDescGap = 0;
} // namespace

// --- UISelectOption --------------------------------------------------------

UISelectOption::UISelectOption() {
  highlightable_ = true;
  style_.width = Length::Pct(100);
  style_.padding = Edges(kRowPadV, kRowPadH);
  style_.radius = 10;
  style_.outlineWidth = 2;
  style_.outlineOutside = true;
  applyFocusChrome();
}

void UISelectOption::applyFocusChrome() {
  style_.outlineColor = Theme::active().focusRing;
}

UISelectOption &UISelectOption::style(const Style &s) {
  UINode::style(s);
  if (style_.outlineWidth == 0) style_.outlineWidth = 2;
  applyFocusChrome();
  return *this;
}

UISelectOption &UISelectOption::title(const char *s) {
  title_ = s ? s : "";
  return *this;
}

UISelectOption &UISelectOption::description(const char *s) {
  description_ = s;
  return *this;
}

UISelectOption &UISelectOption::icon(const char *lucideName) {
  iconName_ = lucideName;
  return *this;
}

UISelectOption &UISelectOption::selected(bool v) {
  selected_ = v;
  return *this;
}

void UISelectOption::layoutSelf(int16_t x, int16_t y, int16_t availW) {
  Canvas *host = layoutHost();
  const float s = layoutScale();
  int16_t w =
      host ? host->resolveLen(style_.width, availW) : style_.width.resolve(availW, s);
  if (style_.width.unit == Unit::Auto || w <= 0) w = availW;

  const Edges pad =
      host ? host->scaledPad(style_.padding) : scaleEdges(style_.padding, s);
  const int16_t innerW = static_cast<int16_t>(w - pad.left - pad.right);
  const int16_t iconSlot =
      (iconName_ && iconName_[0])
          ? (host ? host->sx(static_cast<int16_t>(kIconDesign + kIconGap))
                  : scalePx(static_cast<int16_t>(kIconDesign + kIconGap), s))
          : 0;
  const int16_t checkSlot =
      host ? host->sx(static_cast<int16_t>(kCheckDesign + kIconGap))
           : scalePx(static_cast<int16_t>(kCheckDesign + kIconGap), s);
  int16_t textW =
      static_cast<int16_t>(innerW - iconSlot - checkSlot);
  if (textW < 8) textW = 8;

  int16_t textH = 0;
  if (host) {
    TextStyle ts;
    ts.font = FontRole::Small;
    ts.lineHeight = kTitleLineH;
    ts.lineGap = 0;
    if (title_ && title_[0]) {
      textH = host->measureTextHeight(title_, textW, ts);
    }
    if (description_ && description_[0]) {
      TextStyle ds;
      ds.font = FontRole::Small;
      ds.lineHeight = kDescLineH;
      ds.lineGap = 0;
      textH = static_cast<int16_t>(
          textH + host->sx(kTitleDescGap) +
          host->measureTextHeight(description_, textW, ds));
    }
  } else {
    textH = scalePx(kTitleLineH, s);
    if (description_ && description_[0]) {
      textH = static_cast<int16_t>(textH + scalePx(kTitleDescGap + kDescLineH, s));
    }
  }

  const int16_t iconH =
      host ? host->sx(kIconDesign) : scalePx(kIconDesign, s);
  int16_t contentH = textH > iconH ? textH : iconH;
  if (contentH < iconH) contentH = iconH;

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(contentH + pad.top + pad.bottom);
  } else {
    h = host ? host->resolveLen(style_.height, 0) : style_.height.resolve(0, s);
  }
  borderBox_ = Rect(x, y, w, h);
}

void UISelectOption::paintSelf(Canvas &canvas) {
  const Theme::ThemeTokens &th = Theme::active();
  const Rect &bb = borderBox_;
  const Rect content = canvas.contentBox(bb, style_.padding);

  if (selected_) {
    const uint16_t fill = Theme::soft(ButtonColor::Primary);
    canvas.fillRoundRect(bb, style_.radius > 0 ? style_.radius : 10, fill);
  }

  const int16_t iconPx = canvas.sx(kIconDesign);
  const int16_t checkPx = canvas.sx(kCheckDesign);
  const int16_t gap = canvas.sx(kIconGap);
  int16_t textX = content.x;
  const int16_t textRight = static_cast<int16_t>(content.x + content.w - checkPx - gap);
  int16_t textW = static_cast<int16_t>(textRight - textX);
  if (textW < 8) textW = 8;

  IconSd *icons = canvas.iconSd();
  const Point origin = canvas.origin();

  if (iconName_ && iconName_[0] && icons && icons->ready()) {
    const uint32_t cp = icons->codepoint(iconName_);
    if (cp) {
      const int16_t baselineY =
          static_cast<int16_t>(content.y + iconPx + origin.y);
      const int16_t screenX = static_cast<int16_t>(content.x + origin.x);
      const uint16_t icol =
          selected_ ? Theme::brand(ButtonColor::Primary) : th.baseContent;
      icons->draw(canvas.display(), cp, screenX, baselineY, iconPx, icol);
      textX = static_cast<int16_t>(content.x + iconPx + gap);
      textW = static_cast<int16_t>(textRight - textX);
      if (textW < 8) textW = 8;
    }
  }

  int16_t ty = content.y;
  if (title_ && title_[0]) {
    TextStyle ts;
    ts.font = FontRole::Small;
    ts.color = th.baseContent;
    ts.lineHeight = kTitleLineH;
    ts.lineGap = 0;
    const DrawResult dr =
        canvas.drawText(Rect(textX, ty, textW, 200), title_, ts, false);
    ty = static_cast<int16_t>(ty + dr.h);
  }
  if (description_ && description_[0]) {
    ty = static_cast<int16_t>(ty + canvas.sx(kTitleDescGap));
    TextStyle ds;
    ds.font = FontRole::Small;
    ds.color = Theme::lerp(th.baseContent, th.base100, 0.45f);
    ds.lineHeight = kDescLineH;
    ds.lineGap = 0;
    canvas.drawText(Rect(textX, ty, textW, 200), description_, ds, false);
  }

  if (selected_ && icons && icons->ready()) {
    const uint32_t checkCp = icons->codepoint("check");
    if (checkCp) {
      const int16_t top =
          static_cast<int16_t>(content.y + (content.h - checkPx) / 2);
      const int16_t cx =
          static_cast<int16_t>(content.x + content.w - checkPx + origin.x);
      const int16_t cy = static_cast<int16_t>(top + checkPx + origin.y);
      icons->draw(canvas.display(), checkCp, cx, cy, checkPx,
                  Theme::brand(ButtonColor::Primary));
    }
  }
}

bool UISelectOption::handleEvent(UIEvent &e) {
  if (e.key == UIKey::Select && e.phase == UIKeyPhase::Down) {
    if (group_) group_->choose(*this);
    return true;
  }
  return false;
}

// --- UISelect --------------------------------------------------------------

UISelect::UISelect() {
  canHaveChildren_ = true;
  highlightable_ = false;
  style_.width = Length::Pct(100);
  style_.columns = 1;
  style_.gap = 6;
  style_.hasBackground = false;
}

UISelect &UISelect::style(const Style &s) {
  UINode::style(s);
  return *this;
}

UISelect &UISelect::add(UISelectOption &opt) {
  if (optionCount_ >= kMaxOptions) return *this;
  opt.group_ = this;
  options_[optionCount_++] = &opt;
  UINode::add(opt);
  // Re-apply selection chrome after add.
  syncSelectedFlags();
  return *this;
}

void UISelect::syncSelectedFlags() {
  for (uint8_t i = 0; i < optionCount_; i++) {
    if (options_[i]) options_[i]->selected_ = (static_cast<int16_t>(i) == selected_);
  }
}

UISelect &UISelect::selected(int16_t index) {
  if (index < -1) index = -1;
  if (index >= static_cast<int16_t>(optionCount_) && optionCount_ > 0) {
    // Options may be added after selected() — clamp later in sync / choose.
  }
  selected_ = index;
  syncSelectedFlags();
  return *this;
}

UISelect &UISelect::selectedValue(int16_t value) {
  for (uint8_t i = 0; i < optionCount_; i++) {
    if (options_[i] && options_[i]->value_ == value) {
      return selected(static_cast<int16_t>(i));
    }
  }
  return selected(-1);
}

int16_t UISelect::selectedValue() const {
  const UISelectOption *o = optionAt(static_cast<uint8_t>(selected_ >= 0 ? selected_ : 0));
  if (selected_ < 0 || !o) return 0;
  return o->value_;
}

UISelectOption *UISelect::optionAt(uint8_t i) {
  return i < optionCount_ ? options_[i] : nullptr;
}

const UISelectOption *UISelect::optionAt(uint8_t i) const {
  return i < optionCount_ ? options_[i] : nullptr;
}

void UISelect::choose(UISelectOption &opt) {
  int16_t idx = -1;
  for (uint8_t i = 0; i < optionCount_; i++) {
    if (options_[i] == &opt) {
      idx = static_cast<int16_t>(i);
      break;
    }
  }
  if (idx < 0) return;
  if (idx == selected_) return;
  selected_ = idx;
  syncSelectedFlags();
  if (onChange_) onChange_(*this);
}

void UISelect::layoutSelf(int16_t x, int16_t y, int16_t availW) {
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
  int16_t cy = static_cast<int16_t>(y + pad.top);

  // selected() may have been called before options were added.
  if (selected_ >= static_cast<int16_t>(optionCount_)) selected_ = -1;
  syncSelectedFlags();

  for (uint8_t i = 0; i < childCount_; i++) {
    children_[i]->layout(left, cy, innerW > 0 ? innerW : 0);
    cy = static_cast<int16_t>(children_[i]->borderBox().y +
                             children_[i]->borderBox().h);
    if (i + 1 < childCount_) cy = static_cast<int16_t>(cy + gap);
  }

  int16_t h;
  if (style_.height.unit == Unit::Auto) {
    h = static_cast<int16_t>(cy - y + pad.bottom);
    if (h < pad.top + pad.bottom) h = static_cast<int16_t>(pad.top + pad.bottom);
  } else {
    h = host ? host->resolveLen(style_.height, 0) : style_.height.resolve(0, s);
  }
  borderBox_ = Rect(x, y, w, h);
}

void UISelect::paintSelf(Canvas & /*canvas*/) {
  // Children paint themselves; group chrome is optional later.
}
