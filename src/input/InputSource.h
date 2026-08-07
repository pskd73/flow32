#pragma once

#include "../ui/UIEvent.h"

/**
 * One input device (serial, joystick, …). Poll produces UIEvents into InputHub.
 */
class InputHub;

class InputSource {
public:
  virtual ~InputSource() = default;

  void attach(InputHub &hub) { hub_ = &hub; }

  virtual void begin() {}
  virtual void poll(uint32_t nowMs) = 0;

protected:
  void emit(UIKey key, UIKeyPhase phase);
  void emit(const UIEvent &e);
  InputHub *hub_ = nullptr;
};
