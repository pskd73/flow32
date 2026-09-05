#pragma once

#include "UINode.h"
#include "Style.h"
#include "Theme.h"
#include "UIEvent.h"

class UIGridSelect;

/**
 * One cell in a UIGridSelect: optional Lucide icon above a centered title.
 * Focusable — Select confirms this option.
 */
class UIGridSelectOption : public UINode {
public:
  UIGridSelectOption();

  UIGridSelectOption &style(const Style &s);
  UIGridSelectOption &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UIGridSelectOption &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }

  UIGridSelectOption &title(const char *s);
  /** Lucide name ("bot-message-square"); resolved via Canvas::iconSd() at paint. */
  UIGridSelectOption &icon(const char *lucideName);
  UIGridSelectOption &selected(bool v);
  bool selected() const { return selected_; }

  const char *title() const { return title_; }
  const char *iconName() const { return iconName_; }

  /** App / parent tag (not used by chrome). */
  UIGridSelectOption &value(int16_t v) {
    value_ = v;
    return *this;
  }
  int16_t value() const { return value_; }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
  bool handleEvent(UIEvent &e) override;

private:
  friend class UIGridSelect;

  const char *title_ = "";
  const char *iconName_ = nullptr;
  bool selected_ = false;
  int16_t value_ = 0;
  UIGridSelect *group_ = nullptr;

  void applyFocusChrome();
};

/**
 * Grid radio / navigation picker (2–3 columns). Same selection API as UISelect,
 * with tile cells (icon above title) instead of list rows.
 *
 *   page.gridSelect().selected(-1).onChange(onSel)
 *     .add(page.gridSelectOption().icon("bot").title("Agent").value(0));
 *
 * Column count from style().columns (default 2; clamped to 2–3).
 */
class UIGridSelect : public UINode {
public:
  using ChangeFn = void (*)(UIGridSelect &self);

  static constexpr uint8_t kMaxOptions = 12;

  UIGridSelect();

  UIGridSelect &style(const Style &s);
  UIGridSelect &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UIGridSelect &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }
  UIGridSelect &onChange(ChangeFn fn) {
    onChange_ = fn;
    return *this;
  }

  UIGridSelect &add(UIGridSelectOption &opt);

  /** Which option index is selected (−1 = none). Syncs child .selected(). */
  UIGridSelect &selected(int16_t index);
  int16_t selected() const { return selected_; }

  /** Select by option.value(); −1 if not found. */
  UIGridSelect &selectedValue(int16_t value);
  int16_t selectedValue() const;

  UIGridSelectOption *optionAt(uint8_t i);
  const UIGridSelectOption *optionAt(uint8_t i) const;
  uint8_t optionCount() const { return optionCount_; }

  /** Called by a focused option on Select. */
  void choose(UIGridSelectOption &opt);

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;

private:
  UIGridSelectOption *options_[kMaxOptions] = {};
  uint8_t optionCount_ = 0;
  int16_t selected_ = -1;
  ChangeFn onChange_ = nullptr;

  void syncSelectedFlags();
};
