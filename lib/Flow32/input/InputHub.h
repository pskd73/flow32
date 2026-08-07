#pragma once

#include <Arduino.h>
#include "../ui/UIEvent.h"
#include "InputSource.h"

/**
 * Multiplexes InputSources into a small event queue.
 * Add SerialInput now; JoystickInput later — same hub.
 */
class InputHub {
public:
  static constexpr uint8_t kMaxSources = 4;
  static constexpr uint8_t kMaxQueue = 24;

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
};
