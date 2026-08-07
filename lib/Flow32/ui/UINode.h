#pragma once

#include "../Rect.h"
#include "Style.h"
#include "UIEvent.h"

class Canvas;

struct UIAnim {
  float x = 0;
  float y = 0;
};

/**
 * Composable UI node. Children are laid out relative to this node's content box.
 * Highlightable nodes can show a focus outline when highlighted.
 * Events start at the focused node and bubble via parent_.
 */
class UINode {
public:
  static constexpr uint8_t kMaxChildren = 16;

  using TickFn = void (*)(UINode &self, float dt);
  /** Return true to stop bubbling. */
  using EventFn = bool (*)(UINode &self, UIEvent &e);

  virtual ~UINode() = default;

  UINode &style(const Style &s) {
    style_ = s;
    return *this;
  }
  Style &style() { return style_; }
  const Style &style() const { return style_; }

  UIAnim &anim() { return anim_; }
  const UIAnim &anim() const { return anim_; }

  UINode &onTick(TickFn fn) {
    onTick_ = fn;
    return *this;
  }

  UINode &onEvent(EventFn fn) {
    onEvent_ = fn;
    return *this;
  }

  bool highlightable() const { return highlightable_; }
  UINode &setHighlightable(bool v) {
    highlightable_ = v;
    return *this;
  }
  bool highlighted() const { return highlighted_; }
  void setHighlighted(bool v) { highlighted_ = v; }

  UINode *parent() const { return parent_; }

  /** Depth-first collect of highlightable nodes. */
  void collectHighlightable(UINode **out, uint8_t &count, uint8_t max);

  /** Container nodes accept children; leaves ignore. Sets child.parent_. */
  virtual UINode &add(UINode &child);

  uint8_t childCount() const { return childCount_; }
  UINode *child(uint8_t i) const {
    return i < childCount_ ? children_[i] : nullptr;
  }

  const Rect &borderBox() const { return borderBox_; }

  void tick(float dt);
  void layout(int16_t x, int16_t y, int16_t availW);
  void draw(Canvas &canvas);

  /**
   * Handle event on this node, then bubble to parents until handled.
   * @return true if handled somewhere in the chain.
   */
  bool dispatch(UIEvent &e);

protected:
  Style style_{};
  UIAnim anim_{};
  TickFn onTick_ = nullptr;
  EventFn onEvent_ = nullptr;
  bool highlightable_ = false;
  bool highlighted_ = false;

  UINode *parent_ = nullptr;
  UINode *children_[kMaxChildren] = {};
  uint8_t childCount_ = 0;

  Rect borderBox_{};
  bool canHaveChildren_ = false;

  virtual void layoutSelf(int16_t x, int16_t y, int16_t availW) = 0;
  virtual void paintSelf(Canvas &canvas) = 0;
  /** Subclass default handling (after onEvent_). Return true to stop bubble. */
  virtual bool handleEvent(UIEvent &e);

  Rect paintBox() const;

  friend class Page;
};
