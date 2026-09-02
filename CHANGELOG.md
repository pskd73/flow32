# Changelog

## 0.4.0 — retained UI and explicit hardware ownership

- Made the retained App/Page/tree architecture the only supported UI model.
- Moved ST7735/ST7789, SPI, panel offsets, pins, and backlight GPIO into
  `St77xxTransport`.
- Made `Display` require an application-owned `DisplayTransport`.
- Added stable-state digital-button debounce and joystick hysteresis.
- Added a configurable fatal handler and non-wrapping UI arena checks.
- Clarified RAM-only state as the default and NVS persistence as opt-in.
- Hardened streamed icon and emoji parsing against corrupt external files.
- Avoided rebuilding and presenting unchanged Shell navigation.
- Simplified the normal entry point to `Flow32.h`; retained package export as
  optional maintainer tooling.
- Added BasicUI, Buttons, Joystick, CustomTransport, Assets, and Showcase
  examples.

Breaking changes are documented in the 0.4 migration section of the README and
release notes.
