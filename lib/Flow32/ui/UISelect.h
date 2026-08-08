#pragma once

#include "UINode.h"
#include "Style.h"
#include "Theme.h"
#include "UIEvent.h"

class UISelect;

/**
 * One row in a UISelect list: optional Lucide icon, title, description,
 * and selected chrome. Focusable — Select confirms this option.
 */
class UISelectOption : public UINode {
public:
  UISelectOption();

  UISelectOption &style(const Style &s);
  UISelectOption &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UISelectOption &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }

  UISelectOption &title(const char *s);
  UISelectOption &description(const char *s);
  /** Lucide name ("wifi"); resolved via Canvas::iconSd() at paint. */
  UISelectOption &icon(const char *lucideName);
  UISelectOption &selected(bool v);
  bool selected() const { return selected_; }

  const char *title() const { return title_; }
  const char *description() const { return description_; }
  const char *iconName() const { return iconName_; }

  /** App / parent tag (not used by chrome). */
  UISelectOption &value(int16_t v) {
    value_ = v;
    return *this;
  }
  int16_t value() const { return value_; }

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;
  bool handleEvent(UIEvent &e) override;

private:
  friend class UISelect;

  const char *title_ = "";
  const char *description_ = nullptr;
  const char *iconName_ = nullptr;
  bool selected_ = false;
  int16_t value_ = 0;
  UISelect *group_ = nullptr;

  void applyFocusChrome();
};

/**
 * Vertical radio list. Add UISelectOption children; persist selected index
 * (or option value) in app state across frame rebuilds.
 *
 *   page.select().selected(gIdx).onChange(onSel)
 *     .add(page.selectOption().icon("sun").title("Light").description("…"))
 *     .add(page.selectOption().icon("moon").title("Dark"));
 */
class UISelect : public UINode {
public:
  using ChangeFn = void (*)(UISelect &self);

  static constexpr uint8_t kMaxOptions = 12;

  UISelect();

  UISelect &style(const Style &s);
  UISelect &onTick(TickFn fn) {
    UINode::onTick(fn);
    return *this;
  }
  UISelect &onEvent(EventFn fn) {
    UINode::onEvent(fn);
    return *this;
  }
  UISelect &onChange(ChangeFn fn) {
    onChange_ = fn;
    return *this;
  }

  UISelect &add(UISelectOption &opt);

  /** Which option index is selected (−1 = none). Syncs child .selected(). */
  UISelect &selected(int16_t index);
  int16_t selected() const { return selected_; }

  /** Select by option.value(); −1 if not found. */
  UISelect &selectedValue(int16_t value);
  int16_t selectedValue() const;

  UISelectOption *optionAt(uint8_t i);
  const UISelectOption *optionAt(uint8_t i) const;
  uint8_t optionCount() const { return optionCount_; }

  /** Called by a focused option on Select. */
  void choose(UISelectOption &opt);

protected:
  void layoutSelf(int16_t x, int16_t y, int16_t availW) override;
  void paintSelf(Canvas &canvas) override;

private:
  UISelectOption *options_[kMaxOptions] = {};
  uint8_t optionCount_ = 0;
  int16_t selected_ = -1;
  ChangeFn onChange_ = nullptr;

  void syncSelectedFlags();
};
