#pragma once

#include "InputSource.h"
#include "KeyTracker.h"

/**
 * Serial → UIEvent.
 *
 * One-shot (Down only), type a letter then Enter optional:
 *   u/d/l/r   directions
 *   e/.       select  (also Enter / Space)
 *   b/x       back
 *
 * Hold / release (stick-style, for testing Hold phase):
 *   +u -u  +d -d  +l -l  +r -r  +e -e  +b -b
 * While held, KeyTracker emits Hold repeats.
 *
 * Uppercase U/D/L/R/E/B = one Hold pulse (no press state).
 */
class SerialInput : public InputSource {
public:
  explicit SerialInput(Stream &stream = Serial);

  void begin() override;
  void poll(uint32_t nowMs) override;

  KeyTracker &tracker() { return tracker_; }

private:
  Stream &stream_;
  KeyTracker tracker_;
  char pendingSign_ = 0; // '+' or '-' waiting for key letter

  static void onEmit(void *ctx, UIKey key, UIKeyPhase phase);
  void handleChar(char c, uint32_t nowMs);
  static bool mapKey(char c, UIKey &out);
};
