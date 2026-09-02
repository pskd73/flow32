#pragma once

#include <stdint.h>

/** Allocation-free stable-state debounce for one digital switch. */
class ButtonDebouncer {
public:
  void reset(bool rawPressed, uint32_t nowMs) {
    initialized_ = true;
    candidate_ = rawPressed;
    stable_ = rawPressed;
    candidateSinceMs_ = nowMs;
  }

  /** Returns true only when the debounced logical state changes. */
  bool update(bool rawPressed, uint32_t nowMs, uint16_t debounceMs) {
    if (!initialized_) {
      reset(rawPressed, nowMs);
      return false;
    }
    if (rawPressed != candidate_) {
      candidate_ = rawPressed;
      candidateSinceMs_ = nowMs;
    }
    if (candidate_ == stable_ || nowMs - candidateSinceMs_ < debounceMs) {
      return false;
    }
    stable_ = candidate_;
    return true;
  }

  bool stablePressed() const { return stable_; }

private:
  bool initialized_ = false;
  bool candidate_ = false;
  bool stable_ = false;
  uint32_t candidateSinceMs_ = 0;
};

