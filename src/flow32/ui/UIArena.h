#pragma once

#include <Arduino.h>
#include <new>
#include <stddef.h>
#include <stdint.h>

#include "Flow32Config.h"
#include "flow32/core/FlowError.h"

/** Bump allocator for UI nodes built each frame. */
class UIArena {
public:
  static constexpr size_t kCapacity = FLOW32_UI_ARENA_BYTES;
  static_assert(kCapacity > 0, "FLOW32_UI_ARENA_BYTES must be positive");

  void reset() { used_ = 0; }

  size_t used() const { return used_; }

  template <typename T, typename... Args> T &create(Args... args) {
    const size_t align = alignof(T);
    if (used_ > kCapacity || align == 0 || align - 1 > kCapacity ||
        used_ > kCapacity - (align - 1)) {
      flowFatal(FlowError::UiArenaExhausted);
    }
    size_t offset = (used_ + align - 1) & ~(align - 1);
    if (offset > kCapacity || sizeof(T) > kCapacity - offset) {
      // Never overwrite live nodes. Tune FLOW32_UI_ARENA_BYTES for the largest
      // page if this deterministic guard fires.
      flowFatal(FlowError::UiArenaExhausted);
    }
    void *mem = buf_ + offset;
    used_ = offset + sizeof(T);
    return *new (mem) T(args...);
  }

private:
  alignas(8) uint8_t buf_[kCapacity];
  size_t used_ = 0;
};
