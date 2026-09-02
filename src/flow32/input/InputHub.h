#pragma once

#include <Arduino.h>
#include "Flow32Config.h"
#include "flow32/ui/UIEvent.h"
#include "flow32/input/InputSource.h"

/**
 * Multiplexes serial, button, joystick, touch-adapter, or custom sources into
 * one allocation-free event queue.
 */
class InputHub {
public:
  static constexpr uint8_t kMaxSources = FLOW32_MAX_INPUT_SOURCES;
  static constexpr uint8_t kMaxQueue = FLOW32_INPUT_QUEUE_SIZE;
  static_assert(FLOW32_MAX_INPUT_SOURCES > 0 &&
                    FLOW32_MAX_INPUT_SOURCES <= 255,
                "FLOW32_MAX_INPUT_SOURCES must be 1..255");
  static_assert(FLOW32_INPUT_QUEUE_SIZE > 1 && FLOW32_INPUT_QUEUE_SIZE <= 255,
                "FLOW32_INPUT_QUEUE_SIZE must be 2..255");

  bool add(InputSource &source);
  void begin();
  void poll(uint32_t nowMs);

  bool empty() const { return head_ == tail_; }
  bool pop(UIEvent &out);

  /** Drain queue into target.dispatch (call after page.syncFocus). */
  template <typename Target> void dispatchTo(Target &target) {
    UIEvent e;
    while (pop(e)) {
      target.dispatch(e);
    }
  }

  void push(const UIEvent &e);

private:
  InputSource *sources_[kMaxSources] = {};
  uint8_t sourceCount_ = 0;
  UIEvent queue_[kMaxQueue];
  uint8_t head_ = 0;
  uint8_t tail_ = 0;
  bool begun_ = false;
};
