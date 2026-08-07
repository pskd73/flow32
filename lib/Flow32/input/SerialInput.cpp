#include "SerialInput.h"

SerialInput::SerialInput(Stream &stream) : stream_(stream) {
  tracker_.setEmit(onEmit, this);
}

void SerialInput::onEmit(void *ctx, UIKey key, UIKeyPhase phase) {
  static_cast<SerialInput *>(ctx)->emit(key, phase);
}

void SerialInput::begin() {
  // Stream already started by sketch; print cheat-sheet once.
  stream_.println(F("Input: u/d/l/r  e/. select  b back | +u/-u hold | U Hold pulse"));
}

bool SerialInput::mapKey(char c, UIKey &out) {
  switch (c) {
  case 'u':
  case 'w':
    out = UIKey::Up;
    return true;
  case 'd':
  case 's':
    out = UIKey::Down;
    return true;
  case 'l':
  case 'a':
    out = UIKey::Left;
    return true;
  case 'r':
    out = UIKey::Right;
    return true;
  case 'e':
  case '.':
  case ' ':
    out = UIKey::Select;
    return true;
  case 'b':
  case 'x':
    out = UIKey::Back;
    return true;
  default:
    return false;
  }
}

void SerialInput::handleChar(char c, uint32_t nowMs) {
  if (c == '+' || c == '-') {
    pendingSign_ = c;
    return;
  }

  if (pendingSign_) {
    const char sign = pendingSign_;
    pendingSign_ = 0;
    const char lower = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    UIKey key;
    if (!mapKey(lower, key)) return;
    // Select mapped from \n — ignore if user meant line end after +u
    if ((c == '\r' || c == '\n') && (key == UIKey::Select)) return;
    tracker_.setPressed(key, sign == '+', nowMs);
    return;
  }

  // Uppercase = single Hold pulse (no latch)
  if (c >= 'A' && c <= 'Z') {
    const char lower = static_cast<char>(c - 'A' + 'a');
    UIKey key;
    if (!mapKey(lower, key)) return;
    emit(key, UIKeyPhase::Hold);
    return;
  }

  UIKey key;
  if (!mapKey(c, key)) return;
  // One-shot Down (and optional Up) so buttons/focus react without latch
  emit(key, UIKeyPhase::Down);
}

void SerialInput::poll(uint32_t nowMs) {
  while (stream_.available() > 0) {
    handleChar(static_cast<char>(stream_.read()), nowMs);
  }
  tracker_.poll(nowMs);
}
