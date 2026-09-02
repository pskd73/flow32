#pragma once

#include "InputSource.h"
#include "KeyTracker.h"

#include <stdint.h>

/**
 * Addon: analog joystick → UIKey events (Left/Right/Up/Down/Select/Back).
 *
 * Not part of FlowConfig — construct with pins and register on the runtime:
 *
 *   static JoystickInput joy(1, 2, 3);  // VRx, VRy, SW
 *   flow.apps({...}).input(joy).begin();
 *
 * Optional: call initAdcEarly(pinX, pinY) from setup() before display boot
 * on ESP32-S3 if ADC must be claimed ahead of SPI/I2C.
 *
 * Click = Select; long-press click = Back. Axes use KeyTracker hold repeats.
 */
class JoystickInput : public InputSource {
public:
  JoystickInput(int8_t pinX = -1, int8_t pinY = -1, int8_t pinSw = -1);

  /** Configure ADC GPIO + SAR block. Call before Flow32::begin() on ESP32-S3. */
  static bool initAdcEarly(int8_t pinX, int8_t pinY);

  /** One-shot 12-bit ADC read of VRx/VRy (false if read failed). */
  static bool readRawAxes(int8_t pinX, int8_t pinY, int &outX, int &outY);

  /** Rebind pins before begin() / registration if not set in the constructor. */
  void configure(int8_t pinX, int8_t pinY, int8_t pinSw);
  bool ready() const;

  void begin() override;
  void poll(uint32_t nowMs) override;

  int vrx() const { return vrx_; }
  int vry() const { return vry_; }
  bool swPressed() const { return swPressed_; }

  uint16_t deadzone = 200;
  uint16_t centerX = 2048;
  uint16_t centerY = 2048;
  uint16_t sampleIntervalMs = 16;
  uint16_t longPressMs = 600;

  KeyTracker &tracker() { return tracker_; }

private:
  int8_t pinX_ = -1;
  int8_t pinY_ = -1;
  int8_t pinSw_ = -1;
  int vrx_ = 0;
  int vry_ = 0;
  bool swPressed_ = false;
  bool swLongFired_ = false;
  bool axisValid_ = false;
  bool horizOnY_ = false;
  uint32_t swDownAt_ = 0;
  uint32_t lastSampleMs_ = 0;

  KeyTracker tracker_;

  static void onEmit(void *ctx, UIKey key, UIKeyPhase phase);

  void sample(uint32_t nowMs);
  void updateDirections(uint32_t nowMs);
  void updateButton(uint32_t nowMs);
  void releaseDirections(uint32_t nowMs);
};
