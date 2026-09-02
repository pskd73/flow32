#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define PROGMEM
#define F(value) value
#define pgm_read_byte(address) (*reinterpret_cast<const uint8_t *>(address))
#define pgm_read_word(address) (*reinterpret_cast<const uint16_t *>(address))
#define pgm_read_dword(address) (*reinterpret_cast<const uint32_t *>(address))
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

using byte = uint8_t;

class Print {
public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t) { return 1; }
  size_t print(const char *) { return 0; }
  size_t println(const char * = "") { return 0; }
  int printf(const char *, ...) { return 0; }
};

class Stream : public Print {
public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
};

class HardwareSerial : public Stream {
public:
  void begin(unsigned long) {}
};

extern HardwareSerial Serial;

namespace arduino_stub {
extern int digitalValues[64];
extern int analogValues[64];
extern uint32_t nowMs;
} // namespace arduino_stub

inline void pinMode(int, uint8_t) {}
inline void digitalWrite(int pin, int value) {
  if (pin >= 0 && pin < 64) arduino_stub::digitalValues[pin] = value;
}
inline int digitalRead(int pin) {
  return (pin >= 0 && pin < 64) ? arduino_stub::digitalValues[pin] : LOW;
}
inline int analogRead(int pin) {
  return (pin >= 0 && pin < 64) ? arduino_stub::analogValues[pin] : 0;
}
inline uint32_t millis() { return arduino_stub::nowMs; }
inline void delay(uint32_t amount) { arduino_stub::nowMs += amount; }
