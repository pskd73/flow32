#pragma once

#include <Arduino.h>

struct Rect {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;

  Rect() : x(0), y(0), w(0), h(0) {}
  Rect(int16_t x_, int16_t y_, int16_t w_, int16_t h_)
      : x(x_), y(y_), w(w_), h(h_) {}

  bool empty() const { return w <= 0 || h <= 0; }

  bool contains(int16_t px, int16_t py) const {
    return px >= x && py >= y && px < x + w && py < y + h;
  }

  Rect inset(int16_t n) const {
    return Rect(static_cast<int16_t>(x + n), static_cast<int16_t>(y + n),
                static_cast<int16_t>(w - 2 * n),
                static_cast<int16_t>(h - 2 * n));
  }

  Rect inset(int16_t l, int16_t t, int16_t r, int16_t b) const {
    return Rect(static_cast<int16_t>(x + l), static_cast<int16_t>(y + t),
                static_cast<int16_t>(w - l - r),
                static_cast<int16_t>(h - t - b));
  }

  Rect intersect(const Rect &o) const {
    const int16_t x0 = max(x, o.x);
    const int16_t y0 = max(y, o.y);
    const int16_t x1 =
        min(static_cast<int16_t>(x + w), static_cast<int16_t>(o.x + o.w));
    const int16_t y1 =
        min(static_cast<int16_t>(y + h), static_cast<int16_t>(o.y + o.h));
    return Rect(x0, y0, static_cast<int16_t>(x1 - x0),
                static_cast<int16_t>(y1 - y0));
  }

  static Rect fromSize(int16_t width, int16_t height) {
    return Rect(0, 0, width, height);
  }
};
