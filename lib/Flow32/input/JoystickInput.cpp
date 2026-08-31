#include "JoystickInput.h"

#include <Arduino.h>

#if CONFIG_IDF_TARGET_ESP32S3
#include <Wire.h>

#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_private/sar_periph_ctrl.h"
#include "hal/adc_hal_conf.h"
#include "hal/adc_ll.h"
#include "soc/adc_channel.h"
#endif

namespace {

#if CONFIG_IDF_TARGET_ESP32S3
bool s_adcReady = false;
int8_t s_pinX = -1;
int8_t s_pinY = -1;

void claimAdcRtc() {
  sar_periph_ctrl_init();
  if (!s_adcReady) {
    sar_periph_ctrl_adc_oneshot_power_acquire();
  }
  // Reclaim RTC ownership — SPI/I2C/WiFi path can steal DIG ctrl.
  adc_ll_set_controller(ADC_NUM_1, ADC_LL_CTRL_RTC);
  adc_ll_set_sar_clk_div(ADC_NUM_1, SOC_ADC_SAR_CLK_DIV_DEFAULT(0));
  adc1_config_width(ADC_WIDTH_BIT_12);
  if (!s_adcReady) {
    adc_ll_calibration_init(ADC_NUM_1);
    s_adcReady = true;
  }
}

void configureAnalogPin(int8_t pin) {
  const gpio_num_t gpio = static_cast<gpio_num_t>(pin);
  gpio_reset_pin(gpio);
  gpio_set_direction(gpio, GPIO_MODE_DISABLE);
  if (rtc_gpio_is_valid_gpio(gpio)) {
    rtc_gpio_init(gpio);
    rtc_gpio_set_direction(gpio, RTC_GPIO_MODE_DISABLED);
    rtc_gpio_pullup_dis(gpio);
    rtc_gpio_pulldown_dis(gpio);
  }
}

adc1_channel_t pinToAdc1Channel(int8_t pin) {
  if (pin < 1 || pin > 10) return ADC1_CHANNEL_0;
  return static_cast<adc1_channel_t>(pin - 1);
}

int readAdc1Once(int8_t pin, adc1_channel_t channel) {
  // Do not gpio_reset every sample — thrashing the pad with WiFi up yields
  // full-scale 4095 and kills Left/Up once center calibrates to the rail.
  claimAdcRtc();
  adc1_config_channel_atten(channel, ADC_ATTEN_DB_12);
  for (int attempt = 0; attempt < 2; attempt++) {
    const int raw = adc1_get_raw(channel);
    // Full stick travel often hits 0 or ~4095 — those are valid, not failures.
    if (raw >= 0 && raw <= 4095) return raw;
    delayMicroseconds(200);
  }
  return -1;
}

bool readAxes(int8_t pinX, int8_t pinY, int &outX, int &outY) {
  const int rawX = readAdc1Once(pinX, pinToAdc1Channel(pinX));
  const int rawY = readAdc1Once(pinY, pinToAdc1Channel(pinY));
  if (rawX < 0 || rawY < 0) return false;

  outX = rawX;
  outY = rawY;
  return true;
}
#endif

} // namespace

bool JoystickInput::readRawAxes(int8_t pinX, int8_t pinY, int &outX,
                                int &outY) {
#if CONFIG_IDF_TARGET_ESP32S3
  return readAxes(pinX, pinY, outX, outY);
#else
  outX = analogRead(pinX);
  outY = analogRead(pinY);
  return true;
#endif
}

bool JoystickInput::initAdcEarly(int8_t pinX, int8_t pinY) {
#if defined(FLOW32_JOYSTICK_BUTTON_ONLY)
  (void)pinX;
  (void)pinY;
  Serial.println("Joystick: button-only mode (ADC disabled)");
  return false;
#elif CONFIG_IDF_TARGET_ESP32S3
  if (pinX < 1 || pinX > 10 || pinY < 1 || pinY > 10) {
    Serial.println("Joystick: invalid ADC pins");
    return false;
  }
  Wire.end();
  s_pinX = pinX;
  s_pinY = pinY;
  claimAdcRtc();
  configureAnalogPin(pinX);
  configureAnalogPin(pinY);
  adc1_config_channel_atten(pinToAdc1Channel(pinX), ADC_ATTEN_DB_12);
  adc1_config_channel_atten(pinToAdc1Channel(pinY), ADC_ATTEN_DB_12);
  Serial.printf("Joystick: ADC ready X=GPIO%d Y=GPIO%d\n", pinX, pinY);
  return true;
#else
  (void)pinX;
  (void)pinY;
  return true;
#endif
}

JoystickInput::JoystickInput(int8_t pinX, int8_t pinY, int8_t pinSw)
    : pinX_(pinX), pinY_(pinY), pinSw_(pinSw) {
  tracker_.setEmit(onEmit, this);
}

void JoystickInput::onEmit(void *ctx, UIKey key, UIKeyPhase phase) {
  static_cast<JoystickInput *>(ctx)->emit(key, phase);
}

void JoystickInput::configure(int8_t pinX, int8_t pinY, int8_t pinSw) {
  pinX_ = pinX;
  pinY_ = pinY;
  pinSw_ = pinSw;
}

bool JoystickInput::ready() const {
  return pinX_ >= 0 && pinY_ >= 0 && pinSw_ >= 0;
}

void JoystickInput::releaseDirections(uint32_t nowMs) {
  tracker_.setPressed(UIKey::Left, false, nowMs);
  tracker_.setPressed(UIKey::Right, false, nowMs);
  tracker_.setPressed(UIKey::Up, false, nowMs);
  tracker_.setPressed(UIKey::Down, false, nowMs);
}

void JoystickInput::begin() {
  if (!ready()) return;

#if CONFIG_IDF_TARGET_ESP32S3
  Serial.println("Joystick: center stick...");
  delay(50);
  claimAdcRtc();
  configureAnalogPin(pinX_);
  configureAnalogPin(pinY_);
  int sumX = 0;
  int sumY = 0;
  int samples = 0;
  for (int i = 0; i < 8; i++) {
    int rawX = 0;
    int rawY = 0;
    if (readAxes(pinX_, pinY_, rawX, rawY)) {
      // Ignore rail-stuck samples (broken ADC path / floating) so we don't
      // calibrate center to 4095 and permanently kill Left/Up.
      if (rawX >= 4000 && rawY >= 4000) {
        delay(5);
        continue;
      }
      sumX += rawX;
      sumY += rawY;
      samples++;
    }
    delay(5);
  }
  if (samples == 0) {
    Serial.println("Joystick: ADC rail/timeout — using center 2048");
    centerX = 2048;
    centerY = 2048;
  } else {
    centerX = static_cast<uint16_t>(sumX / samples);
    centerY = static_cast<uint16_t>(sumY / samples);
  }
  vrx_ = static_cast<int>(centerX);
  vry_ = static_cast<int>(centerY);
  axisValid_ = true;
  Serial.printf("Joystick: center X=%u Y=%u (%d samples)\n", centerX, centerY,
                samples);

#if defined(FLOW32_JOY_HORIZ_ON_X) || defined(FLOW32_JOY_HORIZ_ON_Y) || \
    defined(FLOW32_JOY_SWAP_XY)
  // Mapping fixed by build flags — skip long wiggle auto-detect.
  horizOnY_ = false;
#if defined(FLOW32_JOY_HORIZ_ON_Y) || defined(FLOW32_JOY_SWAP_XY)
  horizOnY_ = true;
#endif
  Serial.printf("Joystick: horiz on %s (compile mapping)\n",
                horizOnY_ ? "VRy" : "VRx");
#else
  Serial.println("Joystick: wiggle L/R + U/D...");
  int minX = 4095, maxX = 0, minY = 4095, maxY = 0;
  for (int i = 0; i < 40; i++) {
    int rawX = 0;
    int rawY = 0;
    if (readAxes(pinX_, pinY_, rawX, rawY)) {
      if (rawX < minX) minX = rawX;
      if (rawX > maxX) maxX = rawX;
      if (rawY < minY) minY = rawY;
      if (rawY > maxY) maxY = rawY;
    }
    delay(10);
  }
  const int rangeX = maxX - minX;
  const int rangeY = maxY - minY;
  horizOnY_ = rangeY > rangeX;
  Serial.printf("Joystick: range X=%d Y=%d -> horiz on %s\n", rangeX, rangeY,
                horizOnY_ ? "VRy" : "VRx");
  if (rangeX < 80 && rangeY < 80) {
    Serial.println("Joystick: WARNING low ADC swing — check wiring");
  }
#endif
#else
  analogReadResolution(12);
  analogSetPinAttenuation(pinX_, ADC_11db);
  analogSetPinAttenuation(pinY_, ADC_11db);
#endif

  pinMode(pinSw_, INPUT_PULLUP);
}

void JoystickInput::poll(uint32_t nowMs) {
  if (!ready()) return;

  if (nowMs - lastSampleMs_ >= sampleIntervalMs) {
    sample(nowMs);
  }

  updateButton(nowMs);
  updateDirections(nowMs);
  tracker_.poll(nowMs);
}

void JoystickInput::updateButton(uint32_t nowMs) {
  const bool sw = digitalRead(pinSw_) == LOW;

  if (sw && !swPressed_) {
    swPressed_ = true;
    swLongFired_ = false;
    swDownAt_ = nowMs;
    return;
  }

  if (sw && swPressed_) {
    if (!swLongFired_ && nowMs - swDownAt_ >= longPressMs) {
      swLongFired_ = true;
      emit(UIKey::Back, UIKeyPhase::Down);
    }
    return;
  }

  if (!sw && swPressed_) {
    swPressed_ = false;
    if (!swLongFired_) {
      emit(UIKey::Select, UIKeyPhase::Down);
    }
  }
}

void JoystickInput::sample(uint32_t nowMs) {
  lastSampleMs_ = nowMs;
#if defined(FLOW32_JOYSTICK_BUTTON_ONLY)
  axisValid_ = false;
#elif CONFIG_IDF_TARGET_ESP32S3
  int rawX = 0;
  int rawY = 0;
  if (!readAxes(pinX_, pinY_, rawX, rawY)) {
    axisValid_ = false;
    return;
  }
  vrx_ = rawX;
  vry_ = rawY;
  axisValid_ = true;

#if defined(FLOW32_JOY_DEBUG)
  static uint32_t lastLogMs = 0;
  if (nowMs - lastLogMs >= 400) {
    lastLogMs = nowMs;
    Serial.printf("Joy raw gpio%d=%d gpio%d=%d cx=%u cy=%u\n", pinX_, vrx_,
                  pinY_, vry_, centerX, centerY);
  }
#endif
#else
  vrx_ = analogRead(pinX_);
  vry_ = analogRead(pinY_);
  axisValid_ = true;
#endif
}

void JoystickInput::updateDirections(uint32_t nowMs) {
  if (!axisValid_) {
    releaseDirections(nowMs);
    return;
  }

#if defined(FLOW32_JOY_SWAP_XY)
  const bool horizOnY = true;
#elif defined(FLOW32_JOY_HORIZ_ON_Y)
  const bool horizOnY = true;
#elif defined(FLOW32_JOY_HORIZ_ON_X)
  const bool horizOnY = false;
#else
  const bool horizOnY = horizOnY_;
#endif
  int horiz =
      horizOnY ? (vry_ - static_cast<int>(centerY))
               : (vrx_ - static_cast<int>(centerX));
  int vert =
      horizOnY ? (vrx_ - static_cast<int>(centerX))
               : (vry_ - static_cast<int>(centerY));

#if defined(FLOW32_JOY_INVERT_HORIZ)
  horiz = -horiz;
#endif
#if defined(FLOW32_JOY_INVERT_VERT)
  vert = -vert;
#endif

  const int dz = static_cast<int>(deadzone);

  tracker_.setPressed(UIKey::Left, horiz < -dz, nowMs);
  tracker_.setPressed(UIKey::Right, horiz > dz, nowMs);
  tracker_.setPressed(UIKey::Up, vert < -dz, nowMs);
  tracker_.setPressed(UIKey::Down, vert > dz, nowMs);

#if defined(FLOW32_JOY_DEBUG)
  static bool prevL = false;
  static bool prevR = false;
  static bool prevU = false;
  static bool prevD = false;
  const bool left = horiz < -dz;
  const bool right = horiz > dz;
  const bool up = vert < -dz;
  const bool down = vert > dz;
  if (left != prevL || right != prevR || up != prevU || down != prevD) {
    Serial.printf("Joy nav L=%d R=%d U=%d D=%d horiz=%d vert=%d\n", left, right,
                  up, down, horiz, vert);
    prevL = left;
    prevR = right;
    prevU = up;
    prevD = down;
  }
#endif
}
