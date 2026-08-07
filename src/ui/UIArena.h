#pragma once

#include <Arduino.h>
#include <new>
#include <stddef.h>
#include <stdint.h>

/** Bump allocator for UI nodes built each frame. */
class UIArena {
public:
  static constexpr size_t kCapacity = 8192;

  void reset() { used_ = 0; }

  size_t used() const { return used_; }

  template <typename T, typename... Args> T &create(Args... args) {
    const size_t align = alignof(T);
    size_t offset = (used_ + align - 1) & ~(align - 1);
    if (offset + sizeof(T) > kCapacity) {
      // Fall back: overwrite from start (should not happen in demos)
      offset = 0;
      used_ = 0;
    }
    void *mem = buf_ + offset;
    used_ = offset + sizeof(T);
    return *new (mem) T(args...);
  }

private:
  alignas(8) uint8_t buf_[kCapacity];
  size_t used_ = 0;
};
