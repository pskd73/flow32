#include "InputSource.h"
#include "InputHub.h"

void InputSource::emit(UIKey key, UIKeyPhase phase) {
  emit(UIEvent(key, phase));
}

void InputSource::emit(const UIEvent &e) {
  if (hub_) hub_->push(e);
}
