#include "InputHub.h"

bool InputHub::add(InputSource &source) {
  if (sourceCount_ >= kMaxSources) return false;
  sources_[sourceCount_++] = &source;
  source.attach(*this);
  return true;
}

void InputHub::begin() {
  for (uint8_t i = 0; i < sourceCount_; i++) {
    sources_[i]->begin();
  }
}

void InputHub::poll(uint32_t nowMs) {
  for (uint8_t i = 0; i < sourceCount_; i++) {
    sources_[i]->poll(nowMs);
  }
}

void InputHub::push(const UIEvent &e) {
  const uint8_t next = static_cast<uint8_t>((tail_ + 1) % kMaxQueue);
  if (next == head_) return; // drop if full
  queue_[tail_] = e;
  queue_[tail_].handled = false;
  tail_ = next;
}

bool InputHub::pop(UIEvent &out) {
  if (head_ == tail_) return false;
  out = queue_[head_];
  head_ = static_cast<uint8_t>((head_ + 1) % kMaxQueue);
  return true;
}
