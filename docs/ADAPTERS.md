# Adapter and hardware boundaries

## DisplayTransport

Generic `Display` owns RGB565 framebuffer memory and clipping. It delegates all
hardware presentation and backlight requests:

```cpp
class MyTransport : public DisplayTransport {
public:
  bool begin(const DisplayPanel &panel) override;

  void present(const uint16_t *pixels,
               int16_t x, int16_t y,
               int16_t width, int16_t height,
               int16_t stride) override;

  void setBacklight(bool enabled) override;
};
```

`pixels` addresses the first pixel of the requested rectangle. `stride` is the
number of source pixels between rows and may exceed `width`. Presentation is
synchronous today. A future asynchronous extension can be added at the
transport edge without exposing controller details to UI code.

`St77xxTransport` owns the optional Adafruit ST7735/ST7789 driver, SPI startup,
controller initialization, address-window offsets, and backlight GPIO. Its
configuration begins with every pin unset and `spi == nullptr`.

If an application uses PWM fades or a separate power manager, leave
`pinBacklight` unset. Then only the application controls that hardware.

## InputSource

An input source emits normalized `UIEvent` values into `InputHub`. The
application constructs and registers sources. Flow32 does not create devices or
initialize Serial by itself.

Digital-button debounce occurs before `KeyTracker`, so contact bounce cannot
restart hold timing or create duplicate Down events. Joystick directions have
separate enter and exit dead zones.

## AssetStore

`AssetStore` is a narrow interface over an already-mounted source. Use
`FsAssetStore` with any Arduino `fs::FS`, or implement a custom store. The
application must keep the store and underlying filesystem alive while streamed
atlases use them.

The optional `Storage` adapter can mount SD or SD_MMC only when the application
explicitly calls `begin()`. It has no valid default wiring and never runs as a
side effect of `Flow32::begin()`.

## FontPack

`FontPack` is a value containing borrowed font pointers for semantic roles.
Fonts must outlive the Display. Null roles use Adafruit GFX's built-in fallback.
